#include "app/claude_link.h"

#include <Arduino.h>

#include <cstring>

#include "ble/ble_link.h"
#include "buddy/BuddyProto.h"
#include "data/Settings.h"
#include "hal/led.h"
#include "hal/speaker.h"
#include "ui/power.h"
#include "util/log.h"

static const char *TAG = "claude";

static BuddyState s_state;
static PromptGate s_gate;
static TokenLedger s_ledger;
static uint32_t s_prompt_version = 1;
static uint32_t s_ledger_version = 0;
static uint32_t s_prompt_opened_at = 0;
static uint32_t s_link_version = 0;
// Hora: epoch do desktop + millis() de quando chegou.
static int64_t s_epoch = 0;
static int32_t s_tz = 0;
static uint32_t s_epoch_ms = 0;
// O total de tokens vai pra NVS quando o nível muda ou a cada 5 min (poupa a flash).
static uint32_t s_saved_at = 0;
static bool s_dirty = false;

static void send(const char *line) {
  LOG_D(TAG, "-> %s", line);
  ble_link_send_line(line);
}

static void ack(const char *cmd, bool ok = true, const char *error = nullptr) {
  char out[128];
  if (buddy_ack(out, sizeof(out), cmd, ok, 0, error)) send(out);
}

static void prompt_event(PromptEvent ev) {
  if (ev == PromptEvent::None) return;
  s_prompt_version++;
  if (ev == PromptEvent::Open) {
    s_prompt_opened_at = millis();
    power_wake();
    if (settings().sound) speaker_chime();
    LOG_I(TAG, "pedido %s: %s", s_gate.prompt().id, s_gate.prompt().tool);
  }
  led_attention(s_gate.open());
}

static void on_snapshot(const BuddySnapshot &s) {
  s_state.on_snapshot(s, millis());
  if (s_ledger.add(s.tokens)) {
    LOG_I(TAG, "nível %u", (unsigned)s_ledger.level());
    s_saved_at = 0;  // grava já
  }
  if (s_ledger.lifetime != stats().tokens_lifetime || s_ledger.last != stats().tokens_last) {
    stats().tokens_lifetime = s_ledger.lifetime;
    stats().tokens_last = s_ledger.last;
    s_ledger_version++;
    s_dirty = true;
  }
  prompt_event(s_gate.on_snapshot(s));
}

static void on_status() {
  BuddyStatusInfo info{};
  info.name = settings().name[0] ? settings().name : ble_link_name();
  info.secure = ble_link_secure();
  info.uptime_s = millis() / 1000;
  info.heap = ESP.getFreeHeap();
  info.approved = stats().approved;
  info.denied = stats().denied;
  info.level = s_ledger.level();
  char out[320];
  if (buddy_status_ack(out, sizeof(out), info)) send(out);
}

static void on_line(const char *line, size_t len) {
  LOG_D(TAG, "<- %.*s", (int)(len > 300 ? 300 : len), line);
  BuddyMsg m;
  buddy_parse(line, len, &m);
  switch (m.kind) {
    case BuddyKind::Snapshot: on_snapshot(m.snap); break;
    case BuddyKind::Time:
      s_epoch = m.epoch;
      s_tz = m.tz_s;
      s_epoch_ms = millis();
      LOG_I(TAG, "hora %lld tz=%d", (long long)m.epoch, (int)m.tz_s);
      power_clock_changed();
      break;
    case BuddyKind::CmdStatus: on_status(); break;
    case BuddyKind::CmdName:
    case BuddyKind::CmdOwner: {
      AppSettings &st = settings();
      char *dst = m.kind == BuddyKind::CmdName ? st.name : st.owner;
      snprintf(dst, sizeof(st.name), "%s", m.text);
      settings_save_names();
      ack(m.cmd);
      break;
    }
    case BuddyKind::CmdUnpair:
      ack("unpair");
      ble_link_unpair();
      break;
    case BuddyKind::CmdFolder: break;  // sem ack: o desktop desiste e avisa
    case BuddyKind::CmdOther: ack(m.cmd, false, "unsupported"); break;
    case BuddyKind::Turn:
    case BuddyKind::Other:
    case BuddyKind::Invalid: break;
  }
}

void claude_init() {
  s_ledger.lifetime = stats().tokens_lifetime;
  s_ledger.last = stats().tokens_last;
  ble_link_init();
}

void claude_tick() {
  ble_link_tick(on_line);
  uint32_t now = millis();
  uint32_t lv = ble_link_version();
  if (lv != s_link_version) {
    s_link_version = lv;
    s_state.on_link(ble_link_connected(), now);
    if (!ble_link_connected()) prompt_event(s_gate.on_disconnect());
  }
  if (s_state.tick(now)) prompt_event(s_gate.on_disconnect());  // 30 s sem heartbeat
  if (s_dirty && (s_saved_at == 0 || now - s_saved_at > 300000u)) {
    settings_save_stats();
    s_saved_at = now;
    s_dirty = false;
  }
}

ClaudeState claude_state() { return s_state.state(); }
const BuddySnapshot &claude_snapshot() { return s_state.snap(); }
const TokenLedger &claude_ledger() { return s_ledger; }
uint32_t claude_version() { return s_state.version() + s_ledger_version; }
const PromptGate &claude_prompt() { return s_gate; }
uint32_t claude_prompt_version() { return s_prompt_version; }

void claude_decide(bool approve) {
  if (!s_gate.open()) return;
  char out[200];
  const char *id = s_gate.answer();
  if (buddy_permission(out, sizeof(out), id, approve)) send(out);
  if (approve) stats().approved++;
  else stats().denied++;
  LOG_I(TAG, "%s %s em %u ms", approve ? "aprovou" : "negou", id,
        (unsigned)(millis() - s_prompt_opened_at));
  s_dirty = true;
  s_prompt_version++;
  led_attention(false);
}

bool claude_clock(int64_t *epoch, int32_t *tz_s) {
  if (s_epoch == 0) return false;
  *epoch = s_epoch + (int64_t)((millis() - s_epoch_ms) / 1000);
  *tz_s = s_tz;
  return true;
}
