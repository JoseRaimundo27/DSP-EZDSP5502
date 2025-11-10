/*
 * ============================================================================
 * flanger_params.h
 *
 * Contém todas as macros de configuração (#define) para 
 * os parâmetros dos efeitos de áudio.
 * ============================================================================
 */

#ifndef __TREMOLO_PARAMS_H__
#define __TREMOLO_PARAMS_H__


//--------------- PARÂMETROS DO EFEITO TREMOLO ------------------
// (Velocidade do LFO (6 Hz é um bom valor para tremolo))
#define TREMOLO_fr    6

// (Profundidade: 0.5 em Q15, conforme realtime_tremolo.c)
#define TREMOLO_DEPTH_Q15  16384 
// (Offset: 1.0 - 0.5 = 0.5 em Q15)
#define TREMOLO_OFFSET_Q15 16384 
// (Ganho de entrada: 0.7 em Q15, conforme realtime_tremolo.c)
#define TREMOLO_INPUT_GAIN_Q15 22936 // (0.7 * 32767) 22936


#endif // __TREMOLO_PARAMS_H__
