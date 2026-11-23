#pragma once

/* Textos da interface do CYDex Lite.
 *
 * Adicionar um TEXTO:
 *   1. Criar a chave STR_* aqui (no grupo da tela).
 *   2. Escrever o texto em CADA arquivo de lang/. O teste nativo
 *      (test/test_i18n) falha se faltar algum.
 *
 * Os arquivos de idioma são C, não C++, por causa do inicializador designado
 * ([STR_X] = "..."). Texto em UTF-8, só Latin-1 + … ‹ › • – — “ ” − (a fonte).
 * Formatos de printf (%u) têm que ser os mesmos em todos os idiomas.
 */

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  /* L04 Boot */
  STR_BOOT_READING_SD,
  STR_BOOT_STARTING_BLE,

  /* L04b Sem cartão SD */
  STR_SDERR_NO_CARD_TITLE,
  STR_SDERR_NO_CARD_BODY,
  STR_SDERR_NO_DATA_TITLE,
  STR_SDERR_NO_DATA_BODY,
  STR_SDERR_RETRY,

  /* L01 Vitrine */
  STR_SC_NUM_FMT,        /* %03u = nº */
  STR_SC_GEN_FMT,        /* %s = geração em romano */
  STR_DETAIL_HEIGHT_FMT, /* %s */
  STR_DETAIL_WEIGHT_FMT, /* %s */
  STR_DECIMAL_SEP,

  /* L01 Rodapé do Claude */
  STR_CL_DISCONNECTED,
  STR_CL_IDLE,
  STR_CL_RUNNING_FMT, /* %u = sessões rodando */
  STR_CL_WAITING,
  STR_CL_APPROVE_ON_PC, /* waiting sem prompt: só dá pra aprovar no computador */
  STR_CL_LEVEL_FMT,   /* %u = nível */
  STR_CL_TODAY,

  /* L02 Permissão */
  STR_PERM_TITLE_FMT, /* %s = ferramenta */
  STR_PERM_DENY,
  STR_PERM_APPROVE,

  /* L03 Pareamento */
  STR_PAIR_TITLE,
  STR_PAIR_HINT,

  /* Tipos (mesma ordem de SpeciesType, dex/Types.h). Testado. */
  STR_TYPE_NORMAL,
  STR_TYPE_FIRE,
  STR_TYPE_WATER,
  STR_TYPE_GRASS,
  STR_TYPE_ELECTRIC,
  STR_TYPE_ICE,
  STR_TYPE_FIGHTING,
  STR_TYPE_POISON,
  STR_TYPE_GROUND,
  STR_TYPE_FLYING,
  STR_TYPE_PSYCHIC,
  STR_TYPE_BUG,
  STR_TYPE_ROCK,
  STR_TYPE_GHOST,
  STR_TYPE_DRAGON,
  STR_TYPE_DARK,
  STR_TYPE_STEEL,
  STR_TYPE_FAIRY,

  /* Tipos abreviados (3 letras, chips da Vitrine): mesma ordem. Testado. */
  STR_TYPE_ABBR_NORMAL,
  STR_TYPE_ABBR_FIRE,
  STR_TYPE_ABBR_WATER,
  STR_TYPE_ABBR_GRASS,
  STR_TYPE_ABBR_ELECTRIC,
  STR_TYPE_ABBR_ICE,
  STR_TYPE_ABBR_FIGHTING,
  STR_TYPE_ABBR_POISON,
  STR_TYPE_ABBR_GROUND,
  STR_TYPE_ABBR_FLYING,
  STR_TYPE_ABBR_PSYCHIC,
  STR_TYPE_ABBR_BUG,
  STR_TYPE_ABBR_ROCK,
  STR_TYPE_ABBR_GHOST,
  STR_TYPE_ABBR_DRAGON,
  STR_TYPE_ABBR_DARK,
  STR_TYPE_ABBR_STEEL,
  STR_TYPE_ABBR_FAIRY,

  /* Status base, rótulo curto: mesma ordem do species.bin (PS..Vel) */
  STR_STAT_HP,
  STR_STAT_ATK,
  STR_STAT_DEF,
  STR_STAT_SPA,
  STR_STAT_SPD,
  STR_STAT_SPE,

  STR_COUNT
} StrId;

typedef enum {
  LANG_PT_BR,
  LANG_EN,
  LANG_COUNT
} LangId;

/* Texto no idioma atual. Chave sem tradução cai no pt-BR. Nunca NULL. */
const char *tr(StrId id);

void i18n_set_lang(LangId lang);
LangId i18n_lang(void);

/* "pt-BR", "en"... (salvo na NVS) */
const char *i18n_lang_code(LangId lang);
/* Nome do idioma escrito nele mesmo ("Português", "English"). */
const char *i18n_lang_name(LangId lang);
/* Código desconhecido -> LANG_PT_BR. */
LangId i18n_lang_from_code(const char *code);

/* Texto cru da tabela de um idioma (NULL se faltar). Para os testes. */
const char *i18n_raw(LangId lang, StrId id);

#ifdef __cplusplus
}
#endif
