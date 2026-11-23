#include "BuddyState.h"

#include <cstring>

bool TokenLedger::add(uint32_t tokens) {
  uint32_t before = level();
  uint32_t delta = tokens >= last ? tokens - last : tokens;
  lifetime += delta;
  last = tokens;
  return level() != before;
}

PromptEvent PromptGate::on_snapshot(const BuddySnapshot &s) {
  if (!s.has_prompt) {
    answered_[0] = '\0';
    if (!open_) return PromptEvent::None;
    open_ = false;
    return PromptEvent::Close;
  }
  if (strcmp(s.prompt.id, answered_) == 0) return PromptEvent::None;
  if (open_ && strcmp(s.prompt.id, prompt_.id) == 0) {
    bool changed = s.waiting != waiting_ || strcmp(s.prompt.hint, prompt_.hint) != 0 ||
                   strcmp(s.prompt.tool, prompt_.tool) != 0;
    prompt_ = s.prompt;
    waiting_ = s.waiting;
    return changed ? PromptEvent::Update : PromptEvent::None;
  }
  bool was_open = open_;
  open_ = true;
  prompt_ = s.prompt;
  waiting_ = s.waiting;
  return was_open ? PromptEvent::Update : PromptEvent::Open;
}

const char *PromptGate::answer() {
  strcpy(answered_, prompt_.id);
  open_ = false;
  return answered_;
}

PromptEvent PromptGate::on_disconnect() {
  answered_[0] = '\0';
  if (!open_) return PromptEvent::None;
  open_ = false;
  return PromptEvent::Close;
}

void BuddyState::on_link(bool connected, uint32_t now_ms) {
  if (connected == linked_) return;
  linked_ = connected;
  fresh_ = false;
  last_ms_ = now_ms;
  if (!connected) snap_ = BuddySnapshot{};
  version_++;
}

void BuddyState::on_snapshot(const BuddySnapshot &s, uint32_t now_ms) {
  last_ms_ = now_ms;
  bool changed = !fresh_ || s.total != snap_.total || s.running != snap_.running ||
                 s.waiting != snap_.waiting || s.tokens != snap_.tokens ||
                 s.tokens_today != snap_.tokens_today || s.has_prompt != snap_.has_prompt;
  fresh_ = true;
  snap_ = s;
  if (changed) version_++;
}

bool BuddyState::tick(uint32_t now_ms) {
  if (!fresh_ || now_ms - last_ms_ < BUDDY_STALE_MS) return false;
  fresh_ = false;
  version_++;
  return true;
}

ClaudeState BuddyState::state() const {
  if (!linked_ || !fresh_) return ClaudeState::Disconnected;
  if (snap_.waiting > 0 || snap_.has_prompt) return ClaudeState::Waiting;
  if (snap_.running > 0) return ClaudeState::Running;
  return ClaudeState::Idle;
}
