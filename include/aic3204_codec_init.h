/*
 * aic3204_codec_init.h
 *
 * Descrição:
 * Arquivo de cabeçalho para o driver de inicialização do codec AIC3204.
 * Fornece a função de inicialização mestre para o projeto de DMA CSL.
 */

#ifndef _AIC3204_CODEC_INIT_H_
#define _AIC3204_CODEC_INIT_H_

#include "csl_types.h" // Para Uint16

/*
 * ============================================================================
 * Protótipo da Função
 * ============================================================================
 */
 
/*
 * AIC3204_codec_init
 *
 * Função mestra que:
 * 1. Configura os pinos I2CGPIO para habilitar o McBSP1.
 * 2. Configura o codec AIC3204 via I2C para áudio Line-In (48kHz).
 */
Int16 AIC3204_codec_init( );

/*
 * AIC3204_rset
 *
 * Função de baixo nível para escrever em um registrador do codec via I2C.
 * (Copiada de aic3204_test.c)
 */
Int16 AIC3204_rset( Uint16 regnum, Uint16 regval );


#endif /* _AIC3204_CODEC_INIT_H_ */
