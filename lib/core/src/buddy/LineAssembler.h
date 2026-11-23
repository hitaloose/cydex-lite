#pragma once

#include <cstddef>
#include <cstdint>

// Junta os bytes do Nordic UART em linhas terminadas em '\n' (protocolo do
// Hardware Buddy: um JSON por linha; as escritas chegam picadas no MTU).
// '\r' é ignorado. Linha maior que kMax (ex.: o evt "turn", até 4 KB) é
// descartada inteira até o próximo '\n' e conta em dropped().
class LineAssembler {
 public:
  static constexpr size_t kMax = 1536;

  // Empurra um byte. true = linha completa em line() (vale até o próximo push).
  bool push(uint8_t b);
  const char *line() const { return buf_; }
  size_t length() const { return len_; }
  uint32_t dropped() const { return dropped_; }
  // Descarta a linha pela metade (conexão caiu).
  void reset();

 private:
  char buf_[kMax + 1] = {};
  size_t len_ = 0;
  bool over_ = false;
  bool ready_ = false;
  uint32_t dropped_ = 0;
};
