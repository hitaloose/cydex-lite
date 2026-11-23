#include <unity.h>

#include <cstring>
#include <string>

#include "buddy/BuddyProto.h"
#include "buddy/BuddyState.h"
#include "buddy/LineAssembler.h"

void setUp() {}
void tearDown() {}

static BuddyMsg parse(const char *s) {
  BuddyMsg m;
  buddy_parse(s, strlen(s), &m);
  return m;
}

// ---------- LineAssembler ----------

void test_lines_split_across_writes() {
  LineAssembler a;
  const char *chunks[] = {"{\"a\":", "1}\r", "\n{\"b\":2}\n"};
  std::string got;
  int lines = 0;
  for (const char *c : chunks) {
    for (const char *p = c; *p; p++) {
      if (a.push((uint8_t)*p)) {
        got += a.line();
        got += "|";
        lines++;
      }
    }
  }
  TEST_ASSERT_EQUAL(2, lines);
  TEST_ASSERT_EQUAL_STRING("{\"a\":1}|{\"b\":2}|", got.c_str());
}

void test_oversized_line_is_dropped_whole() {
  LineAssembler a;
  for (size_t i = 0; i < LineAssembler::kMax + 100; i++) TEST_ASSERT_FALSE(a.push('x'));
  TEST_ASSERT_FALSE(a.push('\n'));
  TEST_ASSERT_EQUAL(1, a.dropped());
  const char *next = "{\"ok\":1}\n";
  bool done = false;
  for (const char *p = next; *p; p++) done = a.push((uint8_t)*p);
  TEST_ASSERT_TRUE(done);
  TEST_ASSERT_EQUAL_STRING("{\"ok\":1}", a.line());
}

void test_empty_lines_are_skipped() {
  LineAssembler a;
  TEST_ASSERT_FALSE(a.push('\n'));
  TEST_ASSERT_FALSE(a.push('\r'));
  TEST_ASSERT_FALSE(a.push('\n'));
  TEST_ASSERT_EQUAL(0, a.dropped());
}

// ---------- Parse ----------

void test_snapshot_with_prompt() {
  BuddyMsg m = parse(
      "{\"total\":3,\"running\":1,\"waiting\":1,\"msg\":\"approve: Bash\","
      "\"entries\":[\"10:42 git push\",\"10:41 yarn test\"],\"tokens\":184502,"
      "\"tokens_today\":31200,\"prompt\":{\"id\":\"req_abc123\",\"tool\":\"Bash\","
      "\"hint\":\"rm -rf /tmp/foo\"}}");
  TEST_ASSERT_EQUAL(BuddyKind::Snapshot, m.kind);
  TEST_ASSERT_EQUAL(3, m.snap.total);
  TEST_ASSERT_EQUAL(1, m.snap.running);
  TEST_ASSERT_EQUAL(1, m.snap.waiting);
  TEST_ASSERT_EQUAL_UINT32(184502, m.snap.tokens);
  TEST_ASSERT_EQUAL_UINT32(31200, m.snap.tokens_today);
  TEST_ASSERT_EQUAL_STRING("approve: Bash", m.snap.msg);
  TEST_ASSERT_TRUE(m.snap.has_prompt);
  TEST_ASSERT_EQUAL_STRING("req_abc123", m.snap.prompt.id);
  TEST_ASSERT_EQUAL_STRING("Bash", m.snap.prompt.tool);
  TEST_ASSERT_EQUAL_STRING("rm -rf /tmp/foo", m.snap.prompt.hint);
}

void test_snapshot_without_prompt_and_text_fit() {
  BuddyMsg m = parse("{\"total\":0,\"running\":0,\"waiting\":0,\"msg\":\"**ok** \\ud83d\\ude00\"}");
  TEST_ASSERT_EQUAL(BuddyKind::Snapshot, m.kind);
  TEST_ASSERT_FALSE(m.snap.has_prompt);
  TEST_ASSERT_EQUAL_STRING("ok", m.snap.msg);  // markdown e emoji somem
}

void test_long_hint_is_cut_and_long_id_drops_prompt() {
  std::string hint(400, 'a');
  std::string line = "{\"waiting\":1,\"prompt\":{\"id\":\"r1\",\"tool\":\"Bash\",\"hint\":\"" + hint + "\"}}";
  BuddyMsg m = parse(line.c_str());
  TEST_ASSERT_TRUE(m.snap.has_prompt);
  TEST_ASSERT_TRUE(strlen(m.snap.prompt.hint) < sizeof(m.snap.prompt.hint));
  TEST_ASSERT_NOT_NULL(strstr(m.snap.prompt.hint, "…"));

  std::string id(120, 'i');
  line = "{\"waiting\":1,\"prompt\":{\"id\":\"" + id + "\",\"tool\":\"Bash\"}}";
  m = parse(line.c_str());
  TEST_ASSERT_EQUAL(BuddyKind::Snapshot, m.kind);
  TEST_ASSERT_FALSE(m.snap.has_prompt);
}

void test_commands_and_events() {
  TEST_ASSERT_EQUAL(BuddyKind::CmdStatus, parse("{\"cmd\":\"status\"}").kind);
  TEST_ASSERT_EQUAL(BuddyKind::CmdUnpair, parse("{\"cmd\":\"unpair\"}").kind);
  BuddyMsg o = parse("{\"cmd\":\"owner\",\"name\":\"Felix\"}");
  TEST_ASSERT_EQUAL(BuddyKind::CmdOwner, o.kind);
  TEST_ASSERT_EQUAL_STRING("Felix", o.text);
  BuddyMsg n = parse("{\"cmd\":\"name\",\"name\":\"Clawd\"}");
  TEST_ASSERT_EQUAL(BuddyKind::CmdName, n.kind);
  TEST_ASSERT_EQUAL_STRING("Clawd", n.text);
  TEST_ASSERT_EQUAL(BuddyKind::CmdFolder,
                    parse("{\"cmd\":\"char_begin\",\"name\":\"bufo\",\"total\":184320}").kind);
  BuddyMsg x = parse("{\"cmd\":\"novo\"}");
  TEST_ASSERT_EQUAL(BuddyKind::CmdOther, x.kind);
  TEST_ASSERT_EQUAL_STRING("novo", x.cmd);
  TEST_ASSERT_EQUAL(BuddyKind::Turn,
                    parse("{\"evt\":\"turn\",\"role\":\"assistant\",\"content\":[{\"type\":\"text\",\"text\":\"oi\"}]}").kind);
  BuddyMsg t = parse("{\"time\":[1775731234,-25200]}");
  TEST_ASSERT_EQUAL(BuddyKind::Time, t.kind);
  TEST_ASSERT_EQUAL_INT64(1775731234, t.epoch);
  TEST_ASSERT_EQUAL_INT32(-25200, t.tz_s);
  TEST_ASSERT_EQUAL(BuddyKind::Invalid, parse("nada").kind);
  TEST_ASSERT_EQUAL(BuddyKind::Invalid, parse("[1,2]").kind);
  TEST_ASSERT_EQUAL(BuddyKind::Other, parse("{\"foo\":1}").kind);
}

// ---------- Respostas ----------

void test_replies() {
  char b[256];
  TEST_ASSERT_TRUE(buddy_ack(b, sizeof(b), "owner", true, 0) > 0);
  TEST_ASSERT_EQUAL_STRING("{\"ack\":\"owner\",\"ok\":true,\"n\":0}", b);
  buddy_ack(b, sizeof(b), "x\"y", false, 0, "unsupported");
  TEST_ASSERT_EQUAL_STRING("{\"ack\":\"x\\\"y\",\"ok\":false,\"n\":0,\"error\":\"unsupported\"}", b);
  buddy_permission(b, sizeof(b), "req_abc123", true);
  TEST_ASSERT_EQUAL_STRING("{\"cmd\":\"permission\",\"id\":\"req_abc123\",\"decision\":\"once\"}", b);
  buddy_permission(b, sizeof(b), "req_abc123", false);
  TEST_ASSERT_EQUAL_STRING("{\"cmd\":\"permission\",\"id\":\"req_abc123\",\"decision\":\"deny\"}", b);
  BuddyStatusInfo s{"Claude-CYDex-3F2A", true, 8412, 84200, 42, 3, 5};
  TEST_ASSERT_TRUE(buddy_status_ack(b, sizeof(b), s) > 0);
  TEST_ASSERT_EQUAL_STRING(
      "{\"ack\":\"status\",\"ok\":true,\"data\":{\"name\":\"Claude-CYDex-3F2A\",\"sec\":true,"
      "\"sys\":{\"up\":8412,\"heap\":84200},\"stats\":{\"appr\":42,\"deny\":3,\"lvl\":5}}}",
      b);
  TEST_ASSERT_EQUAL(0, buddy_ack(b, 10, "status", true, 0));  // não coube
}

// ---------- Estado ----------

static BuddySnapshot snap(uint16_t running, uint16_t waiting, const char *id = nullptr) {
  BuddySnapshot s;
  s.total = 2;
  s.running = running;
  s.waiting = waiting;
  if (id) {
    s.has_prompt = true;
    strcpy(s.prompt.id, id);
    strcpy(s.prompt.tool, "Bash");
  }
  return s;
}

void test_state_derivation_and_staleness() {
  BuddyState st;
  TEST_ASSERT_EQUAL(ClaudeState::Disconnected, st.state());
  st.on_link(true, 0);
  TEST_ASSERT_EQUAL(ClaudeState::Disconnected, st.state());  // sem heartbeat ainda
  st.on_snapshot(snap(0, 0), 100);
  TEST_ASSERT_EQUAL(ClaudeState::Idle, st.state());
  uint32_t v = st.version();
  st.on_snapshot(snap(0, 0), 10100);  // keepalive igual
  TEST_ASSERT_EQUAL(v, st.version());
  st.on_snapshot(snap(1, 0), 11000);
  TEST_ASSERT_EQUAL(ClaudeState::Running, st.state());
  TEST_ASSERT_NOT_EQUAL(v, st.version());
  st.on_snapshot(snap(1, 1, "r1"), 12000);
  TEST_ASSERT_EQUAL(ClaudeState::Waiting, st.state());
  TEST_ASSERT_FALSE(st.tick(12000 + BUDDY_STALE_MS - 1));
  TEST_ASSERT_TRUE(st.tick(12000 + BUDDY_STALE_MS));
  TEST_ASSERT_EQUAL(ClaudeState::Disconnected, st.state());
  st.on_snapshot(snap(0, 0), 50000);
  TEST_ASSERT_EQUAL(ClaudeState::Idle, st.state());
  st.on_link(false, 51000);
  TEST_ASSERT_EQUAL(ClaudeState::Disconnected, st.state());
}

void test_token_ledger_survives_desktop_restart() {
  TokenLedger l;
  TEST_ASSERT_FALSE(l.add(30000));
  TEST_ASSERT_EQUAL(0, l.level());
  TEST_ASSERT_TRUE(l.add(60000));  // +30K -> 60K = nível 1
  TEST_ASSERT_EQUAL(1, l.level());
  TEST_ASSERT_EQUAL(200, l.progress_permille());
  TEST_ASSERT_FALSE(l.add(60000));  // keepalive: nada
  TEST_ASSERT_FALSE(l.add(5000));   // app reiniciou: +5K
  TEST_ASSERT_EQUAL_UINT64(65000, l.lifetime);
  TEST_ASSERT_TRUE(l.add(40000));   // +35K = 100K
  TEST_ASSERT_EQUAL(2, l.level());
}

void test_prompt_gate() {
  PromptGate g;
  TEST_ASSERT_EQUAL(PromptEvent::None, g.on_snapshot(snap(0, 0)));
  TEST_ASSERT_EQUAL(PromptEvent::Open, g.on_snapshot(snap(0, 1, "r1")));
  TEST_ASSERT_TRUE(g.open());
  TEST_ASSERT_EQUAL(PromptEvent::None, g.on_snapshot(snap(0, 1, "r1")));
  TEST_ASSERT_EQUAL(PromptEvent::Update, g.on_snapshot(snap(0, 2, "r1")));  // +1
  TEST_ASSERT_EQUAL(2, g.waiting());
  // Respondido no CYD: o heartbeat atrasado com o mesmo id não reabre.
  TEST_ASSERT_EQUAL_STRING("r1", g.answer());
  TEST_ASSERT_FALSE(g.open());
  TEST_ASSERT_EQUAL(PromptEvent::None, g.on_snapshot(snap(0, 2, "r1")));
  // O próximo pedido abre.
  TEST_ASSERT_EQUAL(PromptEvent::Open, g.on_snapshot(snap(0, 1, "r2")));
  // Decidido no computador: some sozinho.
  TEST_ASSERT_EQUAL(PromptEvent::Close, g.on_snapshot(snap(1, 0)));
  // Outro pedido aberto e o link cai.
  g.on_snapshot(snap(0, 1, "r3"));
  TEST_ASSERT_EQUAL(PromptEvent::Close, g.on_disconnect());
  TEST_ASSERT_EQUAL(PromptEvent::None, g.on_disconnect());
  // Trocou de pedido sem passar por "sem prompt": atualiza o aberto.
  g.on_snapshot(snap(0, 1, "r4"));
  TEST_ASSERT_EQUAL(PromptEvent::Update, g.on_snapshot(snap(0, 1, "r5")));
  TEST_ASSERT_EQUAL_STRING("r5", g.prompt().id);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_lines_split_across_writes);
  RUN_TEST(test_oversized_line_is_dropped_whole);
  RUN_TEST(test_empty_lines_are_skipped);
  RUN_TEST(test_snapshot_with_prompt);
  RUN_TEST(test_snapshot_without_prompt_and_text_fit);
  RUN_TEST(test_long_hint_is_cut_and_long_id_drops_prompt);
  RUN_TEST(test_commands_and_events);
  RUN_TEST(test_replies);
  RUN_TEST(test_state_derivation_and_staleness);
  RUN_TEST(test_token_ledger_survives_desktop_restart);
  RUN_TEST(test_prompt_gate);
  return UNITY_END();
}
