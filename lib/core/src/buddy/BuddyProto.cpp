#include "BuddyProto.h"

#include <ArduinoJson.h>

#include <cstdio>
#include <cstring>

#include "logic/TextFit.h"

namespace {

// Só os campos usados entram no documento: "entries" e o "content" do turn
// ficam de fora (memória pequena e fixa por linha).
JsonDocument &filter() {
  static JsonDocument f;
  static bool init = false;
  if (!init) {
    init = true;
    f["cmd"] = true;
    f["evt"] = true;
    f["name"] = true;
    f["time"] = true;
    f["total"] = true;
    f["running"] = true;
    f["waiting"] = true;
    f["msg"] = true;
    f["tokens"] = true;
    f["tokens_today"] = true;
    f["prompt"]["id"] = true;
    f["prompt"]["tool"] = true;
    f["prompt"]["hint"] = true;
  }
  return f;
}

void fit(JsonVariantConst v, char *dst, size_t cap) {
  const char *s = v.is<const char *>() ? v.as<const char *>() : "";
  text_fit(s, dst, cap);
}

uint32_t u32(JsonVariantConst v) {
  if (!v.is<double>()) return 0;  // qualquer número
  double d = v.as<double>();
  if (d < 0) return 0;
  if (d > 4294967295.0) return 4294967295u;
  return (uint32_t)d;
}

uint16_t u16(JsonVariantConst v) {
  uint32_t x = u32(v);
  return x > 0xFFFF ? 0xFFFF : (uint16_t)x;
}

bool is_folder_cmd(const char *c) {
  return !strcmp(c, "char_begin") || !strcmp(c, "file") || !strcmp(c, "chunk") ||
         !strcmp(c, "file_end") || !strcmp(c, "char_end");
}

// Copia `s` como string JSON (com aspas) em `out`; devolve o tamanho ou 0.
size_t json_str(char *out, size_t cap, const char *s) {
  size_t n = 0;
  auto put = [&](char ch) {
    if (n + 1 < cap) out[n] = ch;
    n++;
  };
  put('"');
  for (const unsigned char *p = (const unsigned char *)s; *p; p++) {
    unsigned char ch = *p;
    if (ch == '"' || ch == '\\') {
      put('\\');
      put((char)ch);
    } else if (ch < 0x20) {
      char esc[7];
      snprintf(esc, sizeof(esc), "\\u%04x", ch);
      for (char *e = esc; *e; e++) put(*e);
    } else {
      put((char)ch);
    }
  }
  put('"');
  if (n >= cap) return 0;
  out[n] = '\0';
  return n;
}

size_t finish(int n, size_t cap) { return (n > 0 && (size_t)n < cap) ? (size_t)n : 0; }

}  // namespace

void buddy_parse(const char *line, size_t len, BuddyMsg *out) {
  *out = BuddyMsg{};
  JsonDocument doc;
  DeserializationError err =
      deserializeJson(doc, line, len, DeserializationOption::Filter(filter()));
  if (err || !doc.is<JsonObject>()) {
    out->kind = BuddyKind::Invalid;
    return;
  }
  JsonObjectConst o = doc.as<JsonObjectConst>();

  if (o["cmd"].is<const char *>()) {
    const char *cmd = o["cmd"];
    snprintf(out->cmd, sizeof(out->cmd), "%s", cmd);
    if (!strcmp(cmd, "status")) out->kind = BuddyKind::CmdStatus;
    else if (!strcmp(cmd, "unpair")) out->kind = BuddyKind::CmdUnpair;
    else if (!strcmp(cmd, "name") || !strcmp(cmd, "owner")) {
      out->kind = cmd[0] == 'n' ? BuddyKind::CmdName : BuddyKind::CmdOwner;
      fit(o["name"], out->text, sizeof(out->text));
    } else if (is_folder_cmd(cmd)) out->kind = BuddyKind::CmdFolder;
    else out->kind = BuddyKind::CmdOther;
    return;
  }
  if (o["evt"].is<const char *>()) {
    out->kind = BuddyKind::Turn;
    return;
  }
  if (o["time"].is<JsonArrayConst>()) {
    JsonArrayConst t = o["time"];
    out->kind = BuddyKind::Time;
    out->epoch = t[0].as<long long>();
    out->tz_s = t[1].as<long>();
    return;
  }
  bool snap = !o["total"].isNull() || !o["running"].isNull() || !o["waiting"].isNull() ||
              !o["tokens"].isNull() || !o["prompt"].isNull();
  if (!snap) {
    out->kind = BuddyKind::Other;
    return;
  }
  out->kind = BuddyKind::Snapshot;
  BuddySnapshot &s = out->snap;
  s.total = u16(o["total"]);
  s.running = u16(o["running"]);
  s.waiting = u16(o["waiting"]);
  s.tokens = u32(o["tokens"]);
  s.tokens_today = u32(o["tokens_today"]);
  fit(o["msg"], s.msg, sizeof(s.msg));
  JsonObjectConst p = o["prompt"];
  const char *id = p.isNull() ? nullptr : p["id"].as<const char *>();
  // Id que não cabe inteiro não dá pra ecoar: sem pedido (decide no computador).
  if (id && id[0] && strlen(id) < sizeof(s.prompt.id)) {
    s.has_prompt = true;
    strcpy(s.prompt.id, id);
    fit(p["tool"], s.prompt.tool, sizeof(s.prompt.tool));
    fit(p["hint"], s.prompt.hint, sizeof(s.prompt.hint));
  }
}

size_t buddy_ack(char *out, size_t cap, const char *cmd, bool ok, uint32_t n, const char *error) {
  char c[48], e[64];
  if (!json_str(c, sizeof(c), cmd)) return 0;
  if (error && !json_str(e, sizeof(e), error)) return 0;
  int k = error ? snprintf(out, cap, "{\"ack\":%s,\"ok\":%s,\"n\":%lu,\"error\":%s}", c,
                           ok ? "true" : "false", (unsigned long)n, e)
                : snprintf(out, cap, "{\"ack\":%s,\"ok\":%s,\"n\":%lu}", c, ok ? "true" : "false",
                           (unsigned long)n);
  return finish(k, cap);
}

size_t buddy_status_ack(char *out, size_t cap, const BuddyStatusInfo &s) {
  char name[80];
  if (!json_str(name, sizeof(name), s.name ? s.name : "")) return 0;
  int k = snprintf(out, cap,
                   "{\"ack\":\"status\",\"ok\":true,\"data\":{\"name\":%s,\"sec\":%s,"
                   "\"sys\":{\"up\":%lu,\"heap\":%lu},"
                   "\"stats\":{\"appr\":%lu,\"deny\":%lu,\"lvl\":%lu}}}",
                   name, s.secure ? "true" : "false", (unsigned long)s.uptime_s,
                   (unsigned long)s.heap, (unsigned long)s.approved, (unsigned long)s.denied,
                   (unsigned long)s.level);
  return finish(k, cap);
}

size_t buddy_permission(char *out, size_t cap, const char *id, bool approve) {
  char i[176];
  if (!json_str(i, sizeof(i), id)) return 0;
  int k = snprintf(out, cap, "{\"cmd\":\"permission\",\"id\":%s,\"decision\":\"%s\"}", i,
                   approve ? "once" : "deny");
  return finish(k, cap);
}
