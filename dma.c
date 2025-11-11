//////////////////////////////////////////////////////////////////////////////
// * File name: dma.c
// *
// * Description:  Funções DMA. (MODIFICADO PARA Rx-Tx ÁUDIO LOOPBACK)
// * (ESTRUTURA PING-PONG IMPLEMENTADA)
// *
// * Copyright (C) 2011 Texas Instruments Incorporated - http://www.ti.com/
// * Copyright (C) 2011 Spectrum Digital, Incorporated
// *
// //////////////////////////////////////////////////////////////////////////////
#include"ezdsp5502.h"
#include"ezdsp5502_mcbsp.h"
#include "csl_dma.h"
#include "csl_irq.h" // <-- ADICIONADO: Necessário para as Interrupções

Uint8 dmaState = 0; // (Não mais usado, mas mantido por segurança)

// =================== ALTERAÇÃO PING-PONG ===================
// O tamanho total do buffer é 4096, mas vamos processá-lo
// em blocos de 2048 (metade).
#define AUDIO_BUFFER_SIZE 4096 // Tamanho TOTAL (deve ser par)
#define AUDIO_BLOCK_SIZE (AUDIO_BUFFER_SIZE / 2) // Tamanho do bloco (Ping ou Pong)
// =========================================================

#pragma DATA_SECTION(g_rxBuffer, "dmaMem")
#pragma DATA_ALIGN(g_rxBuffer, 4096)
Uint16 g_rxBuffer[AUDIO_BUFFER_SIZE]; // Buffer circular de ENTRADA

#pragma DATA_SECTION(g_txBuffer, "dmaMem")
#pragma DATA_ALIGN(g_txBuffer, 4096)
Uint16 g_txBuffer[AUDIO_BUFFER_SIZE]; // Buffer circular de SAÍDA
// =========================================================================


/* DMA configuration structure ( Tx - Transmissão) */
// O Tx não precisa de interrupções. Ele simplesmente corre em loop
// contínuo, lendo o g_txBuffer de 0 a 4095 e repetindo.
// A CPU irá escrever neste buffer "ao mesmo tempo".
DMA_Config dmaTxConfig = {
    DMA_DMACSDP_RMK(
        DMA_DMACSDP_DSTBEN_NOBURST , // Destination burst
        DMA_DMACSDP_DSTPACK_OFF,     // Destination packing
        DMA_DMACSDP_DST_PERIPH ,     // Source selection
        DMA_DMACSDP_SRCBEN_NOBURST , // Source burst
        DMA_DMACSDP_SRCPACK_OFF,     // Source packing
        DMA_DMACSDP_SRC_DARAMPORT1 , // Source selection
        DMA_DMACSDP_DATATYPE_16BIT   // Data type
    ), /* DMACSDP */
   
    DMA_DMACCR_RMK(
        DMA_DMACCR_DSTAMODE_CONST,   // Destination address mode
        DMA_DMACCR_SRCAMODE_POSTINC, // Source address mode
        DMA_DMACCR_ENDPROG_OFF,      // End of programmation bit
        DMA_DMACCR_WP_DEFAULT,
        DMA_DMACCR_REPEAT_ALWAYS,    // ( Loop Infinito)
        DMA_DMACCR_AUTOINIT_ON,      // ( Loop Infinito)
        DMA_DMACCR_EN_STOP,          // Channel enable
        DMA_DMACCR_PRIO_HI,          // Channel priority
        DMA_DMACCR_FS_ELEMENT,       // Frame\Element Sync
        DMA_DMACCR_SYNC_XEVT1        // Sincronizar com o Evento de TRANSMISSÃO (Tx)
    ), /* DMACCR */
    
    DMA_DMACICR_RMK(
        DMA_DMACICR_AERRIE_OFF,
        DMA_DMACICR_BLOCKIE_OFF ,    // Whole block interrupt enable
        DMA_DMACICR_LASTIE_OFF,      // Last frame Interrupt enable
        DMA_DMACICR_FRAMEIE_OFF,     // (Tx NÃO precisa de interrupção)
        DMA_DMACICR_FIRSTHALFIE_OFF, // (Tx NÃO precisa de interrupção)
        DMA_DMACICR_DROPIE_OFF,      // Sync event drop interrupt enable
        DMA_DMACICR_TIMEOUTIE_OFF    // Time out inetrrupt enable
    ), /* DMACICR */
    
    (DMA_AdrPtr)0x0000, // DMACSSAL - (Vamos definir isto na configAudioDma)
    0,                  // DMACSSAU
    (DMA_AdrPtr)0x5804, // DMACDSAL - DMA destination is DXR1 (Transmit)
    0,                  // DMACDSAU 
    AUDIO_BUFFER_SIZE,  // DMACEN  - Transferência de 4096 elementos
    1,                  // DMACFN  - Single frame
    0,                  // DMACSFI - Source frame index
    0,                  // DMACSEI - Source element index
    0,                  // DMACDFI - Destination frame index
    0                   // DMACDEI
};


//COnfiguraçoes da recepçao
DMA_Handle dmaRxHandle; // Handle para o nosso novo canal de Receção
DMA_Handle dmaTxHandle; // Handle para o canal de Transmissão


DMA_Config dmaRxConfig = {
    DMA_DMACSDP_RMK(
        DMA_DMACSDP_DSTBEN_NOBURST,
        DMA_DMACSDP_DSTPACK_OFF,
        DMA_DMACSDP_DST_DARAMPORT1, /* Destino: a nossa RAM (g_rxBuffer) */
        DMA_DMACSDP_SRCBEN_NOBURST,
        DMA_DMACSDP_SRCPACK_OFF,
        DMA_DMACSDP_SRC_PERIPH,     /* Origem: o Periférico (McBSP "Line In") */
        DMA_DMACSDP_DATATYPE_16BIT
    ), /* DMACSDP */

    DMA_DMACCR_RMK(
        DMA_DMACCR_DSTAMODE_POSTINC, /* Incrementar o destino (preencher o buffer) */
        DMA_DMACCR_SRCAMODE_CONST,   /* Ler sempre da mesma "porta" (McBSP) */
        DMA_DMACCR_ENDPROG_OFF,
        DMA_DMACCR_WP_DEFAULT,
        DMA_DMACCR_REPEAT_ALWAYS,    /* Lição: Loop Infinito */
        DMA_DMACCR_AUTOINIT_ON,      /* Lição: Loop Infinito */
        DMA_DMACCR_EN_STOP,
        DMA_DMACCR_PRIO_HI,          /* Receber é ALTA prioridade */
        DMA_DMACCR_FS_ELEMENT,
        DMA_DMACCR_SYNC_REVT1        /* Sincronizar com o Evento de RECEÇÃO (Rx) */
    ), /* DMACCR */

    // =================== ALTERAÇÃO PING-PONG ===================
    // Ativamos AMBAS as interrupções: Meio-Buffer e Fim-Buffer
    DMA_DMACICR_RMK(
        DMA_DMACICR_AERRIE_OFF,
        DMA_DMACICR_BLOCKIE_OFF,
        DMA_DMACICR_LASTIE_OFF,
        DMA_DMACICR_FRAMEIE_ON,      /* Interrupção de FIM (processar Bloco PONG) */
        DMA_DMACICR_FIRSTHALFIE_ON,  /* Interrupção de MEIO (processar Bloco PING) */
        DMA_DMACICR_DROPIE_OFF,
        DMA_DMACICR_TIMEOUTIE_OFF
    ), /* DMACICR */
    // =========================================================

    (DMA_AdrPtr)0x5800, // DMACSSAL - Endereço do McBSP DRR1 (Receive)
    0,                  // DMACSSAU
    (DMA_AdrPtr)0x0000, // DMACDSAL - (Vamos definir isto na configAudioDma)
    0,                  // DMACDSAU
    AUDIO_BUFFER_SIZE,  // DMACEN  - Tamanho total do buffer circular (4096)
    1,                  // DMACFN  - Single frame
    0,                  // DMACSFI
    0,                  // DMACSEI
    0,                  // DMACDFI
    0                   // DMACDEI
};

//(ISRs):
extern void VECSTART(void);
interrupt void dmaRxIsr(void);
interrupt void dmaTxIsr(void); // (Não usada, mas precisa de existir)

// =================== ALTERAÇÃO PING-PONG ===================
// Flag estática para sabermos qual bloco processar.
// 0 = Processar Bloco PING (primeira metade)
// 1 = Processar Bloco PONG (segunda metade)
static volatile Uint16 dmaPingPongFlag = 0;
// =========================================================

/*
 * configAudioDma( )
 *
 * (Reconstruída para Rx-Tx)
 * Configura AMBOS os canais de DMA (Rx e Tx) e as Interrupções
 */
void configAudioDma (void)
{
    Uint16 rcvEventId, xmtEventId;
    
    // Aponta o Tx para o nosso buffer de saída (g_txBuffer)
    dmaTxConfig.dmacssal = (DMA_AdrPtr)(((Uint32)&g_txBuffer) << 1);
    
    // Canal 0 para Tx
    dmaTxHandle = DMA_open(DMA_CHA0, 0);
    DMA_config(dmaTxHandle, &dmaTxConfig);

    /* --- Configurar o Canal de Receção (Rx - Canal 1) --- */

    // Apontar o Rx para o nosso buffer de entrada (g_rxBuffer)
    dmaRxConfig.dmacdsal = (DMA_AdrPtr)(((Uint32)&g_rxBuffer) << 1);

    //Canal 1 para Rx
    dmaRxHandle = DMA_open(DMA_CHA1, 0);
    DMA_config(dmaRxHandle, &dmaRxConfig);

    /* --- Configurar as Interrupções  --- */
    CSL_init();
    IRQ_setVecs((Uint32)(&VECSTART));

    rcvEventId = DMA_getEventId(dmaRxHandle);
    xmtEventId = DMA_getEventId(dmaTxHandle);
    
    IRQ_disable(rcvEventId);
    IRQ_disable(xmtEventId); // (Tx não terá interrupções)
    IRQ_clear(rcvEventId);
    IRQ_clear(xmtEventId);

    IRQ_plug(rcvEventId, &dmaRxIsr); // Ligar ISR de Receção
    IRQ_plug(xmtEventId, &dmaTxIsr); // Ligar ISR de Transmissão (stub)

    IRQ_enable(rcvEventId); // Ligar APENAS a interrupção de Receção
    
    IRQ_globalEnable();
}

void startAudioDma (void)
{
    // Resetar o flag PingPong antes de começar
    dmaPingPongFlag = 0;

    DMA_start(dmaRxHandle); // Ligar RECEPÇAO
    DMA_start(dmaTxHandle); // Ligar TRANSMISSAO
}
// =========================================================================


void stopAudioDma (void)
{
    DMA_stop(dmaRxHandle);  // Parar Rx
    DMA_stop(dmaTxHandle);  // Parar Tx
}


void changeTone (void)
{
    // Não fazer nada.
    // (Isto impede o projeto de "explodir" se o SW2 for premido)
}


interrupt void dmaRxIsr(void)
{
    int i;
    Uint16 *p_rxBuffer; // Ponteiro para o início do bloco de ENTRADA
    Uint16 *p_txBuffer; // Ponteiro para o início do bloco de SAÍDA

    if (dmaPingPongFlag == 0)
    {
        // --- Processar Bloco PING ---
        // O DMA acabou de encher a PRIMEIRA METADE (Ping) [0 ... 2047]
        // O DMA está AGORA a encher a SEGUNDA METADE (Pong)

        p_rxBuffer = &g_rxBuffer[0];                // Ler de g_rxBuffer[0]
        p_txBuffer = &g_txBuffer[0];                // Escrever em g_txBuffer[0]

        dmaPingPongFlag = 1; // Na próxima interrupção, processar o PONG
    }
    else
    {
        // --- Processar Bloco PONG ---
        // O DMA acabou de encher a SEGUNDA METADE (Pong) [2048 ... 4095]
        // O DMA está AGORA a encher a PRIMEIRA METADE (Ping)

        p_rxBuffer = &g_rxBuffer[AUDIO_BLOCK_SIZE]; // Ler de g_rxBuffer[2048]
        p_txBuffer = &g_txBuffer[AUDIO_BLOCK_SIZE]; // Escrever em g_txBuffer[2048]

        dmaPingPongFlag = 0; // Na próxima interrupção, processar o PING
    }


    for (i = 0; i < AUDIO_BLOCK_SIZE; i++)
    {
        // (No futuro, o seu Reverb/Phaser irá aqui)
        p_txBuffer[i] = p_rxBuffer[i];
    }
}

/* * dmaTxIsr( )
 * Transmissão. Não a usamos, mas tem de existir porque
 * foi ligada (plugged) ao vetor de interrupção.
 */
interrupt void dmaTxIsr(void)
{
    // Não faz nada
}
