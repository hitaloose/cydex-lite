#include "TextFit.h"

#include <cstring>

bool font_has_glyph(uint32_t cp) {
  return (cp >= 0x20 && cp <= 0x7E) || (cp >= 0xA0 && cp <= 0xFF) || cp == 0x2013 ||
         cp == 0x2014 || cp == 0x201C || cp == 0x201D || cp == 0x2022 || cp == 0x2026 ||
         cp == 0x2039 || cp == 0x203A || cp == 0x2212;
}

namespace {

// Lê um código UTF-8 de `p`. Sequência inválida: consome 1 byte e devolve 0.
uint32_t next_cp(const unsigned char *&p) {
  unsigned char b = *p;
  if (b < 0x80) {
    p++;
    return b;
  }
  int len = (b & 0xE0) == 0xC0 ? 2 : (b & 0xF0) == 0xE0 ? 3 : (b & 0xF8) == 0xF0 ? 4 : 0;
  if (len == 0) {
    p++;
    return 0;
  }
  uint32_t cp = b & (0x7F >> len);
  for (int i = 1; i < len; i++) {
    if ((p[i] & 0xC0) != 0x80) {
      p++;
      return 0;
    }
    cp = (cp << 6) | (p[i] & 0x3F);
  }
  p += len;
  return cp;
}

size_t encode(uint32_t cp, char *out) {
  if (cp < 0x80) {
    out[0] = (char)cp;
    return 1;
  }
  if (cp < 0x800) {
    out[0] = (char)(0xC0 | (cp >> 6));
    out[1] = (char)(0x80 | (cp & 0x3F));
    return 2;
  }
  out[0] = (char)(0xE0 | (cp >> 12));
  out[1] = (char)(0x80 | ((cp >> 6) & 0x3F));
  out[2] = (char)(0x80 | (cp & 0x3F));
  return 3;
}

// Equivalente desenhável de um caractere sem glifo (nullptr = some).
const char *substitute(uint32_t cp) {
  switch (cp) {
    case 0x2018: case 0x2019: case 0x201A: case 0x2032: return "'";
    case 0x201E: case 0x2033: return "\"";
    case 0x2192: case 0x27F6: case 0x21D2: return "->";
    case 0x2190: case 0x21D0: return "<-";
    case 0x2194: return "<->";
    case 0x2264: return "<=";
    case 0x2265: return ">=";
    case 0x2260: return "!=";
    case 0x2248: return "~";
    case 0x2010: case 0x2011: case 0x2012: return "-";
    case 0x2713: case 0x2714: case 0x2705: return "ok";
    case 0x2717: case 0x2718: case 0x274C: return "x";
    default: return nullptr;  // emoji, CJK e afins somem
  }
}

}  // namespace

size_t text_fit(const char *src, char *dst, size_t cap) {
  if (cap == 0) return 0;
  dst[0] = '\0';
  if (!src) return 0;

  static const char kEllipsis[] = "\xE2\x80\xA6";  // …
  const size_t ell = sizeof(kEllipsis) - 1;
  size_t len = 0;
  bool pending_space = false;
  // Onde cortar se estourar: depois do último caractere que ainda deixa
  // espaço para o "…".
  size_t safe_cut = 0;

  const unsigned char *p = (const unsigned char *)src;
  while (*p) {
    uint32_t cp = next_cp(p);
    if (cp == 0) continue;

    char buf[8];
    size_t n = 0;
    if (cp == ' ' || cp == '\n' || cp == '\r' || cp == '\t' || cp == 0xA0) {
      if (len > 0) pending_space = true;
      continue;
    }
    if (cp == '`' || cp == '*' || cp == '#') continue;
    if (cp < 0x20) continue;
    if (font_has_glyph(cp)) {
      n = encode(cp, buf);
    } else {
      const char *sub = substitute(cp);
      if (!sub) continue;
      n = strlen(sub);
      memcpy(buf, sub, n);
    }

    size_t need = n + (pending_space ? 1 : 0);
    if (len + need + 1 > cap) {
      // Não cabe: volta pro último ponto seguro e põe o "…".
      len = safe_cut;
      while (len > 0 && dst[len - 1] == ' ') len--;
      if (len + ell + 1 <= cap) {
        memcpy(dst + len, kEllipsis, ell);
        len += ell;
      }
      dst[len] = '\0';
      return len;
    }
    if (pending_space) dst[len++] = ' ';
    pending_space = false;
    memcpy(dst + len, buf, n);
    len += n;
    if (len + ell + 1 <= cap) safe_cut = len;
  }
  dst[len] = '\0';
  return len;
}
