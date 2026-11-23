#pragma once

#include <cstdint>

// Lógica da Vitrine (L01), sem LVGL. Sem visto/capturado: entram todas as espécies.

// Opções em degraus ("−" valor "+"). Retorna o índice de `value` em `steps`
// (ou o mais próximo abaixo; 0 se for menor que todos).
uint8_t option_index(const uint16_t *steps, uint8_t count, uint16_t value);
// Próximo valor andando `dir` (-1/+1), parando nas pontas.
uint16_t option_step(const uint16_t *steps, uint8_t count, uint16_t value, int8_t dir);

// Troca a cada: 30 s, 1, 2, 5, 15 min (CLAUDE.md, "Modo vitrine").
constexpr uint8_t SHOWCASE_INTERVAL_COUNT = 5;
extern const uint16_t SHOWCASE_INTERVALS_S[SHOWCASE_INTERVAL_COUNT];
// Próxima posição na lista de espécies da vitrine (`count` itens).
// Sequencial: current+1, dando a volta. Aleatória: usa `rnd` e nunca repete a
// atual quando há mais de uma. `current` = -1 na primeira escolha.
uint16_t showcase_next(int32_t current, uint16_t count, bool random, uint32_t rnd);

// Aleatória sem repetição ("saco"): escolhe em `list` (números de espécie) uma
// que ainda não saiu na rodada. `shown` são bits por nº − 1 (DEX_BITS_BYTES),
// guardados por quem chama. Se todas já saíram, limpa os bits da lista e começa
// outra rodada sem repetir `last` (nº da última mostrada; 0 = nenhuma). Marca a
// escolhida e devolve a posição dela em `list`. Como os bits são por número, a
// rodada sobrevive à lista ser refeita, e uma espécie nova entra como não saída.
uint16_t showcase_pick(const uint16_t *list, uint16_t count, uint8_t *shown, uint16_t last,
                       uint32_t rnd);

// Deslocamento anti-retenção (1–2 px) da troca `k` (canvas: 4 posições).
void showcase_shift(uint32_t k, int8_t *dx, int8_t *dy);

// Caixa opaca (colunas x0..x1, linhas y0..y1) de um sprite quadrado RGB565 com
// fundo chroma key `key`. Falso se não houver pixel opaco.
bool showcase_opaque_box(const uint16_t *px, uint16_t size, uint16_t key, uint8_t *x0,
                         uint8_t *x1, uint8_t *y0, uint8_t *y1);

// Vitrine: o sprite 96 em 2x numa janela de 144 px mostra 72 px do original.
constexpr uint8_t SHOWCASE_CROP = 72;
// Início da janela de `win` px num eixo de `size` px cuja parte opaca vai de
// `lo` a `hi`: centrada na parte opaca e presa em [0, size - win]. Parte opaca
// maior que a janela com `anchor_end` (eixo vertical): o fim da janela fica no
// `hi` (mantém os pés, corta o topo).
uint8_t showcase_crop_origin(uint8_t lo, uint8_t hi, uint8_t size, uint8_t win, bool anchor_end);
