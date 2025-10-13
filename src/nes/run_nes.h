#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
  // Entrée du core nofrendo (bloquante)
  int nofrendo_main(int argc, char *argv[]);
}
#endif

// Lance l'émulateur NES (nofrendo) en lui passant le chemin XIP de la ROM.
// - xipPath : ex. "/xip/Super_Mario_Bros.nes"
// NOTE : fonction bloquante (retourne quand le core quitte).
void run_nes(const char* xipPath);
