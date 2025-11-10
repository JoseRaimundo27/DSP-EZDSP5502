

#ifndef __REVERB_PARAMS_H__
#define __REVERB_PARAMS_H__


//--------------- PARÂMETROS DO EFEITO REVERB (FIR) ------------------
// (Tamanho da nossa Resposta ao Impulso h(l))
// (Valores maiores são mais realistas, mas MUITO mais lentos)
#define REVERB_IR_SIZE 50 //16

// (O nosso buffer de atraso precisa ser pelo menos deste tamanho)
#define REVERB_DELAY_SIZE 1024
#define REVERB_DELAY_MASK (REVERB_DELAY_SIZE - 1)


#endif // __REVERB_PARAMS_H__
