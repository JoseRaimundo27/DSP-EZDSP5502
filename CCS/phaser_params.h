#ifndef PHASER_PARAMS_H_
#define PHASER_PARAMS_H_

// --- PHASER AGRESSIVO (Configurações) ---
// (O "LFO" de Ponto Fixo (Tabela de Seno))
#define LFO_SIZE 256
// Número de filtros em cascata (8 = Som denso e profundo)
#define PHASER_NUM_STAGES 1

// Feedback (Ressonância).
// Valor Q15: 0.85 * 32768 = 27853
// Quanto maior, mais "metálico" e "assobiante" o som fica.
#define PHASER_FEEDBACK_Q15 10000

// Faixa de Varredura (Sweep Range) do efeito
// Define o quão grave e agudo o efeito chega.

// Min Alpha (0.1) -> 0.1 * 32768 = 3277
#define PHASER_MIN_ALPHA_Q15 3277

// Max Alpha (0.9) -> 0.9 * 32768 = 29491
#define PHASER_MAX_ALPHA_Q15 3277

#endif /* PHASER_PARAMS_H_ */
