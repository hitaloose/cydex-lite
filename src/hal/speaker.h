#pragma once

// Alto-falante do CYD (conector P4, GPIO 26). Só um clique curto, sem travar
// o loop: speaker_click() liga o tom e speaker_update() (no loop) desliga.
void speaker_init();
void speaker_click();
// Dois bipes curtos (aviso do Claude Code). Também sem travar o loop.
void speaker_chime();
void speaker_update();
