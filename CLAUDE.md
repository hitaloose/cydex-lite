# CYDex Lite

Versão limitada do **CYDex** ([github.com/hitaloose/cydex](https://github.com/hitaloose/cydex), local em `~/Development/cydex`), voltada para o uso do protocolo BLE do Hardware Buddy do Claude Desktop. Repositório: [github.com/hitaloose/cydex-lite](https://github.com/hitaloose/cydex-lite) (remote `origin` pelo alias SSH `hitaloose.github.com`).

É um firmware separado para a mesma placa, o **CYD ESP32-2432S028**. Ele tem só o **modo vitrine** e um link **BLE com o Claude Desktop**, usando o protocolo Hardware Buddy (`anthropics/claude-desktop-buddy`, REFERENCE.md). Mostra o estado do Claude e o uso de tokens no rodapé da vitrine e deixa **aprovar ou negar pedidos de permissão** pelo toque.

O código é uma cópia independente do CYDex (sem submodule): não há sincronização automática. Correções feitas nas partes copiadas (HAL, `dex/`, `Showcase`, `TextFit`, widgets) precisam ser repetidas à mão no outro projeto.

**Por que um firmware separado:** no CYDex, BLE não cabe na RAM (CLAUDE.md de lá, "BLE no lugar do Wi-Fi: descartado"). Aqui, sem Wi-Fi, saves, nomes em RAM e a arena de 46 KB de sprites, sobra muito heap (ver "Memória").

Nome: "CYDex Lite". Não usar "Poké"/"Pokémon" em nomes de projeto, repositório ou binário.

## Layout (fonte da verdade da UI)

- Canvas: https://claude.ai/artifact/X6qPMmTGHUkXPwgL7YpC3C. Ele começou da `Vitrine.dc.html` do canvas do CYDex.
- Quando a UI mudar, atualizar o canvas primeiro e depois o firmware.

| # | Canvas | Tela | Firmware |
|---|---|---|---|
| L01 | `Main.dc.html` | Vitrine com o Claude | `ui/screens/showcase_screen.cpp` |
| L02 | `Permissao.dc.html` | Pedido de permissão (camada) | `ui/claude_ui.cpp` (`open_prompt`) |
| L03 | `Pareamento.dc.html` | Passkey (camada) | `ui/claude_ui.cpp` (`open_pairing`) |
| L04 | `Boot.dc.html` | Boot | `ui/screens/boot_screen.cpp` |
| L04b | `ErroSD.dc.html` | Sem cartão SD | `ui/screens/sd_error_screen.cpp` |

### L01 Vitrine

- Cartão 314×234 (x=3, y=3, padding 3/5), o mesmo do CYDex, com quatro mudanças:
  - **Cabeçalho:** `#001` (14/700 muted), o nome (16/700, "…") e os tipos abreviados à direita.
  - **Sem visto/capturado:** não há leitura de save. Toda espécie aparece completa, e o sorteio cobre as 1025.
  - **"Geração X":** fica embaixo das medidas, na coluna de status.
  - **Rodapé de 24 px (borda 1 + padding 4):** é o status do Claude.
    - À esquerda, um ponto de 8 px e o estado: Desconectado (anel faint), Ocioso (anel muted), "Rodando · n" (cheio, verde) ou Esperando (cheio, amarelo).
    - À direita, só com link: `Nv n` (12/700 muted), uma barra de 52×4 até o próximo nível e `31,2K hoje`.
- Toque em qualquer lugar mostra a próxima espécie e reinicia o timer.
- Troca a cada 60 s, em ordem aleatória (saco sem repetir, `showcase_pick`). Desloca 1–2 px a cada troca.
- O rodapé redesenha sozinho quando `claude_version()` muda, sem refazer a carta (`showcase_update_footer`).

### L02 Permissão

- Abre quando o heartbeat traz `prompt`, com um id que não foi respondido aqui.
- **Fundo:** uma camada BG a 60% em `lv_layer_top()`. Ela engole o toque, então a vitrine não troca atrás.
- **Painel:** 314×162, encostado embaixo, com borda accent.
  - Topo: escudo + `Permissão · <tool>` e o selo `+n` quando `waiting > 1`.
  - Meio: o `hint` em até 3 linhas.
  - Embaixo: os botões **Negar** e **Aprovar**, de 44 px.
- Ao abrir: chime (se "som" estiver ligado) e o LED vermelho piscando.
- Negar manda `{"cmd":"permission","id":…,"decision":"deny"}`; Aprovar manda o mesmo com `"once"`.
- Fecha sozinho quando o heartbeat vem sem `prompt` (decidido no computador), quando o link cai ou depois de 30 s sem heartbeat.
- Um heartbeat atrasado com o id já respondido não reabre o pedido (`PromptGate`).

### L03 Pareamento

Tela cheia em `lv_layer_top()` com o ícone Bluetooth, "Parear com o Claude", o passkey `482 913` (28/700) e o nome anunciado. Some quando o link fica cifrado ou cai.

## Hardware

O mesmo do CYDex. Os detalhes estão no CLAUDE.md de lá; o resumo:

- ESP32-2432S028 **sem o "R"**, com ST7789 em paisagem, `TFT_BGR`, `INVERSION_OFF` e **sem PSRAM**.
- **Barramentos:** display na HSPI; toque XPT2046 por bit-bang (25/32/39/33); microSD na VSPI. Os pinos estão só em `include/board_pins.h`.
- **Periféricos:**
  - Backlight por PWM no GPIO 21.
  - Alto-falante no GPIO 26 (canal 2 do ledc).
  - LED RGB em 4/16/17, comum-anodo (HIGH apaga), em `hal/led`.
- **Stack:** PlatformIO + Arduino, LVGL 8.3.11, TFT_eSPI 2.5.43, NimBLE-Arduino 1.4.3 e ArduinoJson 7.2.1.
- **Comandos:**
  - `pio run -e cyd -t upload`
  - `pio run -e cyd_debug -t upload` (log `[D][mem]` a cada 10 s)
  - `pio device monitor`
  - `pio test -e native_test` (só `lib/core`)
- **Print da tela:** comando `shot` na serial e `tools/cydex_screenshot.py --out <pasta> <nome>`. Abrir a porta reinicia a placa no macOS.

## Memória (medido em 2026-11-23, `cyd_debug`)

- **Ordem do setup:** `early_alloc` (só o buffer do display, 15 KB) → display/LVGL → `sprite_reserve` (18 KB, um 96×96) → SD (FATFS) → `claude_init` (NimBLE) → telas.

| Ponto | heap livre | maior bloco |
|---|---|---|
| boot | 198 KB | 110 KB |
| depois das reservas | 164 KB | 110 KB |
| SD montado | 145 KB | 110 KB |
| NimBLE anunciando | 127 KB | 106 KB |
| vitrine na tela | **122 KB** | **102 KB** |

- O CYDex com BLE ficava com 48 KB e travava no `NimBLEDevice::init`.
- O pool do LVGL é estático, de 24 KB; o pico medido foi 4,2 KB.
- A pilha da loopTask tem 12 KB (`getArduinoLoopTaskStackSize`), e a menor folga medida foi 9,3 KB.
- RAM estática: 69,8 KB.
- Com o Claude Desktop real conectado, pareado e mandando heartbeats: 122,7 KB livres, maior bloco 102 KB (2026-11-23). O desktop negociou MTU 23, então as linhas vão picadas em 20 B.
- Ainda não medido com o pedido de permissão aberto.

## Dados

- **Cartão SD (FAT32):** o mesmo do CYDex, só leitura. Precisa só de `/cydex/species.bin` e `/cydex/sprites_96.bin`, gerados pelo `tools/build_sd.py` do CYDex.
- **Sem saves, status.cache ou nomes em RAM:** `data/SpeciesRepo` lê o registro de 64 B (com o nome) na troca de espécie. `data/SpriteLoader` lê o sprite no buffer fixo.
- **NVS (namespace `cydexl`, `data/Settings`):** não existe tela de Ajustes, então os valores são os padrões ou o que já estiver na NVS.

| Chave | O que guarda | Padrão |
|---|---|---|
| `lang` | idioma | `pt-BR` |
| `sc_int` | intervalo da vitrine (30/60/120/300/900 s) | 60 s |
| `sc_rand` | ordem aleatória | ligada |
| `bright` | brilho de dia | 35% |
| `bright_n` | brilho de noite | 15% |
| `night_from` | início da noite (hora local) | 19 h |
| `night_to` | fim da noite (hora local) | 7 h |
| `sound` | som ligado | sim |
| `name` | nome vindo do desktop | — |
| `owner` | dono vindo do desktop | — |
| `appr` | pedidos aprovados | — |
| `deny` | pedidos negados | — |
| `tok_life` | total de tokens do nível | — |
| `tok_last` | último `tokens` visto | — |

- O nível e as estatísticas vão para a NVS quando o nível muda ou a cada 5 minutos com mudança, para poupar a flash.

## Protocolo (Hardware Buddy)

- **Transporte:** Nordic UART Service.
  - Serviço `6e400001-b5a3-f393-e0a9-e50e24dcca9e`.
  - RX `…0002` (write) e TX `…0003` (notify).
  - JSON UTF-8, um objeto por linha terminada em `\n`, picado no MTU nos dois sentidos.
- **Nome anunciado:** `Claude-CYDex-XXXX`, onde XXXX é o fim do MAC BT. O picker do desktop filtra por "Claude". O nome vai no scan response.
- **Segurança:**
  - LE Secure Connections com bonding e MITM; o CYD é DisplayOnly.
  - RX tem `WRITE_ENC|WRITE_AUTHEN` e TX tem `READ_ENC|READ_AUTHEN`, então o 1º acesso dispara o pareamento.
  - O NimBLE chama `onPassKeyRequest`, e o firmware sorteia um passkey de 6 dígitos para a L03.
  - Se o pareamento terminar sem cifra e autenticação, a conexão cai.
  - Os bonds ficam na NVS do NimBLE (até 3).
  - `{"cmd":"unpair"}` responde com ack, chama `deleteAllBonds()` e derruba a conexão.
- **Camadas do código:**
  - `ble/ble_link`: anel de RX de 2 KB (o callback roda na task do NimBLE), `LineAssembler` e envio.
  - `app/claude_link`: liga o link ao protocolo.
  - `lib/core/src/buddy/`: parse e estado, puros e testados em `test/test_buddy`.
- **Mensagens tratadas:**

| Mensagem | O que o firmware faz |
|---|---|
| Heartbeat (`total`, `running`, `waiting`, `msg`, `tokens`, `tokens_today`, `prompt`) | ArduinoJson com filtro: `entries` fica de fora. Textos passam por `text_fit`; o `prompt.id` fica cru e precisa caber em 79 B, senão o pedido é ignorado |
| `{"time":[epoch,tz]}` | Relógio para dia/noite (`claude_clock`) |
| `{"cmd":"status"}` | Responde `name`, `sec`, `sys.up/heap` e `stats.appr/deny/lvl` (sem `bat`: a placa não tem bateria) |
| `name`, `owner` | Ack + NVS |
| `unpair` | Ver "Segurança" |
| `evt:"turn"` | Ignorado (não há tela de transcript). Linhas maiores que 1,5 KB são descartadas inteiras |
| Pasta (`char_begin`…) | **Sem ack**: o desktop desiste e avisa o usuário |
| Outro `cmd` | Ack com `ok:false`, `error:"unsupported"` |

- **Estado (`BuddyState`):**
  - Desconectado: sem link ou 30 s sem heartbeat.
  - Esperando: `waiting > 0` ou `prompt`.
  - Rodando: `running > 0`.
  - Ocioso: o resto.
- **Nível (`TokenLedger`):**
  - Soma o que o `tokens` do heartbeat cresceu.
  - Esse valor zera quando o app do computador reinicia: um valor menor que o último conta inteiro.
  - Sobe um nível a cada 50K tokens de saída.
  - O protocolo não traz % do limite do plano.

## Regras de UI (herdadas do CYDex)

- Sem scroll e sem animação: um toque gera um redesenho. Alvos de toque ≥ 32 px (botões de 44). Tema escuro.
- Todo texto de interface passa por `tr(STR_*)` (`lib/core/src/i18n`).
  - pt-BR é a referência e en é o segundo idioma.
  - O `test_i18n` falha se faltar texto, se os `%` divergirem ou se houver glifo fora da fonte (`\n` é permitido).
- Tokens de cor em `ui/theme.h`, os mesmos do CYDex, mais `RUNNING`/`WAITING`/`BLUETOOTH` para o estado do Claude.
- **Fontes:** Montserrat 12/14/16/28, Latin-1, geradas por `tools/gen_fonts.mjs`. Os TTF ficam em `tools/fonts/`, que está no gitignore; copiar do CYDex.
- **Ícones:** A8 gerados por `tools/gen_icons.mjs` (`cd tools && npm install && npm run icons`). A lista tem só os 6 ícones usados.
- Uma tela por vez (`ui/nav`). Pedido e pareamento são camadas em `lv_layer_top()`, e `ui/claude_ui` as abre e fecha comparando versões num timer de 100 ms.

## Testar sem o Claude Desktop

`tools/buddy_fake_desktop.py` (bleak) roda este roteiro:

1. Procura o CYD, pareia (o macOS pede o passkey da tela) e manda `time`, `owner`, `status`, um `char_begin` (que não deve ter ack) e um `turn`.
2. Manda heartbeats de ocioso, depois de rodando.
3. Abre um pedido e espera Aprovar ou Negar no CYD.
4. Manda o heartbeat atrasado com o mesmo pedido, que não pode reabrir.
5. Passa de nível.

`--unpair` esquece o pareamento.

```
python3 -m venv .venv && .venv/bin/pip install bleak
.venv/bin/python tools/buddy_fake_desktop.py
```

Com o Claude Desktop real: Help → Troubleshooting → Enable Developer Mode, depois Developer → Open Hardware Buddy… → Connect e digitar o passkey.

## Futuro / em aberto

- Tela mínima de Ajustes (toque longo): intervalo, brilho, idioma, som, "esquecer pareamento".
- Apagar a tela quando o computador dorme. O protocolo não avisa; dá para usar a falta de heartbeat.
- Medir a memória com o pedido de permissão aberto.
- **Tokens:** no começo, com sessões do Claude Code no app, o desktop mandava `"tokens":0,"tokens_today":0` em todo heartbeat (log `<-` do `cyd_debug`). Mais tarde, no mesmo dia (2026-11-23), o `tokens_today` veio preenchido e apareceu no rodapé. Falta confirmar se o `tokens` também sobe e se o nível (`TokenLedger`) acompanha.
