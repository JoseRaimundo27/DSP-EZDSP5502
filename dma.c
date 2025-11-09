//////////////////////////////////////////////////////////////////////////////
// * File name: dma.c
// * // * Description:  DMA functions. (MODIFICADO PARA Rx-Tx ÁUDIO LOOPBACK)
// * // * Copyright (C) 2011 Texas Instruments Incorporated - http://www.ti.com/ 
// * Copyright (C) 2011 Spectrum Digital, Incorporated
// * //////////////////////////////////////////////////////////////////////////////
#include"ezdsp5502.h"
#include"ezdsp5502_mcbsp.h"
#include "csl_dma.h"
#include "csl_irq.h" // <-- ADICIONADO: Necessário para as Interrupções

Uint8 dmaState = 0; // (Não mais usado, mas mantido por segurança)


#define AUDIO_BUFFER_SIZE 4096 // Manter o mesmo tamanho do demo

#pragma DATA_SECTION(g_rxBuffer, "dmaMem")
#pragma DATA_ALIGN(g_rxBuffer, 4096)
Uint16 g_rxBuffer[AUDIO_BUFFER_SIZE]; // Onde o "Line In" escreve (BUFFER DE ENTRADA)

#pragma DATA_SECTION(g_txBuffer, "dmaMem")
#pragma DATA_ALIGN(g_txBuffer, 4096)
Uint16 g_txBuffer[AUDIO_BUFFER_SIZE]; // De onde o "Headphone" lê (BUFFER DE SAÍDA)
// =========================================================================


/* DMA configuration structure ( Tx - Transmissão) */
DMA_Config dmaTxConfig = { // (Renomeado de 'myconfig' para 'dmaTxConfig')
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
        DMA_DMACICR_FRAMEIE_OFF,     // (Lição: Interrupção de Tx DESLIGADA)
        DMA_DMACICR_FIRSTHALFIE_OFF, // HAlf frame interrupt enable
        DMA_DMACICR_DROPIE_OFF,      // Sync event drop interrupt enable
        DMA_DMACICR_TIMEOUTIE_OFF    // Time out inetrrupt enable
    ), /* DMACICR */
    
    (DMA_AdrPtr)0x0000, // DMACSSAL - (Vamos definir isto na configAudioDma)
    0,                  // DMACSSAU
    (DMA_AdrPtr)0x5804, // DMACDSAL - DMA destination is DXR1 (Transmit)
    0,                  // DMACDSAU 
    AUDIO_BUFFER_SIZE,  // DMACEN  - 1024 elementos
    1,                  // DMACFN  - Single frame
    0,                  // DMACSFI - Source frame index
    0,                  // DMACSEI - Source element index
    0,                  // DMACDFI - Destination frame index
    0                   // DMACDEI - (Demo usava 2, mas 0 é mais seguro)
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

    DMA_DMACICR_RMK(
        DMA_DMACICR_AERRIE_OFF,
        DMA_DMACICR_BLOCKIE_OFF,
        DMA_DMACICR_LASTIE_OFF,
        DMA_DMACICR_FRAMEIE_ON,
        DMA_DMACICR_FIRSTHALFIE_OFF,
        DMA_DMACICR_DROPIE_OFF,
        DMA_DMACICR_TIMEOUTIE_OFF
    ), /* DMACICR */

    (DMA_AdrPtr)0x5800, // DMACSSAL - Endereço do McBSP DRR1 (Receive)
    0,                  // DMACSSAU
    (DMA_AdrPtr)0x0000, // DMACDSAL - (Vamos definir isto na configAudioDma)
    0,                  // DMACDSAU
    AUDIO_BUFFER_SIZE,  // DMACEN  - 1024 elementos
    1,                  // DMACFN  - Single frame
    0,                  // DMACSFI
    0,                  // DMACSEI
    0,                  // DMACDFI
    0                   // DMACDEI
};

//(ISRs):
extern void VECSTART(void);
interrupt void dmaRxIsr(void);
interrupt void dmaTxIsr(void);

/*
 * configAudioDma( )
 *
 * (Reconstruída para Rx-Tx)
 * Configura AMBOS os canais de DMA (Rx e Tx) e as Interrupções
 */
void configAudioDma (void)
{
    Uint16 rcvEventId, xmtEventId;

    
    // Aponta o Tx para o nosso NOVO buffer de saída (g_txBuffer)
    dmaTxConfig.dmacssal = (DMA_AdrPtr)(((Uint32)&g_txBuffer) << 1);
    
    // Canal 0 para Tx
    dmaTxHandle = DMA_open(DMA_CHA0, 0);
    DMA_config(dmaTxHandle, &dmaTxConfig);

    /* --- Configurar o Canal de Receção (Rx - Canal 1) --- */

    //  Apontar o Rx para o nosso NOVO buffer de entrada (g_rxBuffer)
    // E traduzir o endereço para Byte (<< 1)
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
    IRQ_disable(xmtEventId);
    IRQ_clear(rcvEventId);
    IRQ_clear(xmtEventId);

    IRQ_plug(rcvEventId, &dmaRxIsr); // Ligar
    IRQ_plug(xmtEventId, &dmaTxIsr); // Ligar

    IRQ_enable(rcvEventId); // Ligar APENAS a interrupção de Receção

    
    IRQ_globalEnable();
}

void startAudioDma (void)
{
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

/*
 * dmaRxIsr( )
 *
 * O DMA chama esta função QUANDO o g_rxBuffer está CHEIO.
 * Este é o "Cérebro" do nosso projeto.
 */
interrupt void dmaRxIsr(void)
{
    int i;

    /* O "Cérebro" (Processamento) */
    // Por agora, vamos apenas fazer um loopback (bypass)
    // O áudio que acabou de chegar (g_rxBuffer)
    // é copiado para o áudio que vai sair (g_txBuffer)

    for (i = 0; i < AUDIO_BUFFER_SIZE; i++)
    {
        // (No futuro, o seu Reverb/Phaser irá aqui)
        g_txBuffer[i] = g_rxBuffer[i];
    }


}

/* Transmissão. Não a usamos, mas tem de existir. */
interrupt void dmaTxIsr(void)
{
    // Não faz nada
}
