#ifndef __PITCH_PARAMS_H__
#define __PITCH_PARAMS_H__

// --- CONFIGURAÇÃO DO PITCH SHIFTER ---

// Tamanho da Janela (Grain). 2048 é bom para voz grave.
#define PITCH_WINDOW_SIZE 1024
#define PITCH_BUF_SIZE 2048 //DOBRO
#define PITCH_MASK (PITCH_BUF_SIZE - 1)

// --- O VALOR QUE O PYTHON CALCULOU ---
// Exemplo para Si Bemol (Bb): Ratio 1.122 -> 1.122 * 32767 = 36780
// Exemplo para Lá (A): Ratio 1.259 -> 1.259 * 32767 = 41283
// COLOQUE O VALOR DO SEU PRINT PYTHON AQUI:
#define PITCH_RATIO_VAL 200000

#endif
