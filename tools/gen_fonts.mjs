// Gera as fontes Montserrat do LVGL em src/ui/fonts/.
// As fontes embutidas do LVGL só têm ASCII. Estas cobrem Latin-1 (é, ã, ç...)
// e alguns símbolos usados no layout (… ‹ › • – — “ ” −).
//
// Uso: (cd tools && npm install && npm run fonts)
// Os TTF estáticos ficam em tools/fonts/ (baixados do repositório oficial
// JulietaUla/Montserrat, licença OFL).
import { execFileSync } from "node:child_process";
import { mkdirSync, writeFileSync } from "node:fs";
import { dirname, join } from "node:path";
import { fileURLToPath } from "node:url";

const here = dirname(fileURLToPath(import.meta.url));
const out = join(here, "..", "src", "ui", "fonts");
const conv = join(here, "node_modules", ".bin", "lv_font_conv");

const WEIGHTS = { 500: "Medium", 600: "SemiBold", 700: "Bold" };
// Só as combinações que o canvas usa. Adicionar aqui quando uma tela pedir outra.
const FONTS = [
  [12, 500], [12, 600], [12, 700],
  [14, 500], [14, 600], [14, 700],
  [16, 700],
  [28, 700],
];
const RANGE = "0x20-0x7E,0xA0-0xFF,0x2013,0x2014,0x201C,0x201D,0x2022,0x2026,0x2039,0x203A,0x2212";

mkdirSync(out, { recursive: true });
const decls = [];
for (const [size, weight] of FONTS) {
  const name = `font_${size}_${weight}`;
  execFileSync(conv, [
    "--font", join(here, "fonts", `Montserrat-${WEIGHTS[weight]}.ttf`),
    "-r", RANGE,
    "--size", String(size),
    "--bpp", "4",
    "--format", "lvgl",
    "--lv-include", "lvgl.h",
    "--no-compress",
    "-o", join(out, `${name}.c`),
  ], { stdio: "inherit" });
  decls.push(`LV_FONT_DECLARE(${name});`);
  console.log(`ok ${name}`);
}

writeFileSync(join(out, "fonts.h"), `#pragma once

// Gerado por tools/gen_fonts.mjs. Não editar à mão.
// Nome: font_<tamanho px>_<peso>. Montserrat 4 bpp, Latin-1.

#include <lvgl.h>

${decls.join("\n")}
`);
