#include "LineAssembler.h"

void LineAssembler::reset() {
  len_ = 0;
  over_ = false;
  ready_ = false;
  buf_[0] = '\0';
}

bool LineAssembler::push(uint8_t b) {
  if (ready_) reset();
  if (b == '\r') return false;
  if (b != '\n') {
    if (len_ < kMax) buf_[len_++] = (char)b;
    else over_ = true;
    return false;
  }
  if (over_) {
    dropped_++;
    reset();
    return false;
  }
  if (len_ == 0) return false;
  buf_[len_] = '\0';
  ready_ = true;
  return true;
}
