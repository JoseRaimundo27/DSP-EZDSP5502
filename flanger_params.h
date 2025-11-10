/*
 * ============================================================================
 * flanger_params.h
 *
 * Contém todas as macros de configuração (#define) para 
 * os parâmetros do efeito Flanger.
 * ============================================================================
 */

#ifndef __FLANGER_PARAMS_H__
#define __FLANGER_PARAMS_H__



//--------------- PARÂMETROS DO EFEITO FLANGER --------------------
// (Atraso Médio (12.5ms @ 48kHz - igual a 100 @ 8kHz))
#define FLANGER_L0    600 // 600
// (Variação (50% de L0 - igual a A_maxSwing = 0.5))
#define FLANGER_A     300 // 300
// (Profundidade (1.0 em Q15 - G_depth = 1.0))
#define FLANGER_g     32767   //16384 ou 32767
// (Velocidade do LFO (0.2 Hz))
#define FLANGER_fr    0.2


//--------------- PARÂMETROS DOS BUFFERS --------------------------
// (O "Buffer de Atraso" (Delay Line) do Flanger)
#define FLANGER_DELAY_SIZE 1024
#define FLANGER_DELAY_MASK (FLANGER_DELAY_SIZE - 1) // (Para loop rápido)

// (O "LFO" de Ponto Fixo (Tabela de Seno))
#define LFO_SIZE 256


#endif // __FLANGER_PARAMS_H__
