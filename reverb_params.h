

#ifndef __REVERB_PARAMS_H__
#define __REVERB_PARAMS_H__


// --- BUFFER TOTAL ---
// Precisamos de um "Pool" de memória. 8192 deve ser suficiente (aprox 170ms total)
#define REVERB_MEM_SIZE 8192

// --- TAMANHOS DOS ATRASOS (Delay Lengths) ---
// Escolhidos para serem números primos (Prime Numbers) para evitar metalização.
// Baseado em ~48kHz.
#define C1_LEN 1687 // ~35ms
#define C2_LEN 1601 // ~33ms
#define C3_LEN 2053 // ~42ms
#define C4_LEN 2251 // ~47ms

#define AP1_LEN 225  // ~4.6ms
#define AP2_LEN 341  // ~7.1ms

// --- GANHOS (Q15) ---
// Feedback dos Combs (Controla o tempo de reverberação RT60)
// 0.8 a 0.85 é um bom valor. (0.84 * 32767 = 27524)
#define C1_GAIN 27524
#define C2_GAIN 27524
#define C3_GAIN 27524
#define C4_GAIN 27524

// Ganho dos All-Pass (Controla a difusão/densidade)
// 0.5 a 0.7 é padrão. (0.5 * 32767 = 16384)
#define AP_GAIN   16384


#endif // __REVERB_PARAMS_H__
