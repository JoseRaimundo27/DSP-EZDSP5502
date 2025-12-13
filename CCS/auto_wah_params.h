#ifndef AUTO_WAH_PARAMS_H_
#define AUTO_WAH_PARAMS_H_

// --- PARÂMETROS DO AUTO-WAH (AGRESSIVO) ---

// 1. Agressividade (Ressonância/Q)
// Controla o pico do filtro.
// Valor BAIXO (3000) = Pico ALTO (Apito forte/Funk)
// Valor ALTO (30000) = Pico BAIXO (Som suave)
#define AUTO_WAH_DAMP_Q15 30000

// 2. Frequências Limite
// 300Hz (Fechado - Grave) -> 1300 em Q15
#define AUTO_WAH_MIN_FREQ_Q15 1300
// 3500Hz (Aberto - Agudo) -> 15000 em Q15
#define AUTO_WAH_MAX_FREQ_Q15 15000

// 3. Sensibilidade
// O quanto a força da palhetada abre o filtro.
// 28000 é bem alto para garantir que o efeito abra totalmente.
#define AUTO_WAH_SENSITIVITY_Q15 14000

// 4. Velocidade do Envelope
// Quão rápido o wah abre e fecha (Attack/Release).
// 150 é uma resposta rápida (snappy).
#define AUTO_WAH_ENV_SPEED_Q15 150

#endif /* AUTO_WAH_PARAMS_H_ */
