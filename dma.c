#include"ezdsp5502.h"
#include"ezdsp5502_mcbsp.h"
#include "csl_dma.h"
#include "csl_irq.h"
#include <math.h>
#include "icomplex.h"
#include "flanger_params.h"

#define AUDIO_BUFFER_SIZE 4096 // BUFFER MAIOR
#define FS 48000
#define PI 3.14159265359


#pragma DATA_SECTION(g_rxBuffer, "dmaMem")
#pragma DATA_ALIGN(g_rxBuffer, 4096)
Uint16 g_rxBuffer[AUDIO_BUFFER_SIZE]; // Onde o "Line In" escreve (BUFFER DE ENTRADA)

#pragma DATA_SECTION(g_txBuffer, "dmaMem")
#pragma DATA_ALIGN(g_txBuffer, 4096)
Uint16 g_txBuffer[AUDIO_BUFFER_SIZE]; // De onde o "Headphone" lê (BUFFER DE SAÍDA)

//FLANGER:
#pragma DATA_SECTION(g_flangerBuffer, "flangerMem")
#pragma DATA_ALIGN(g_flangerBuffer, 4)
Int16 g_flangerBuffer[FLANGER_DELAY_SIZE]; // O nosso buffer circular
volatile Uint16 g_flangerWriteIndex = 0;  // Onde escrevemos no buffer

Int16 g_lfoTable[LFO_SIZE];
volatile Uint16 g_lfoIndex = 0; 
float g_lfoPhaseInc = (LFO_SIZE * FLANGER_fr) / (float)FS; // (Calculado 1 vez)

//(ISRs):
extern void VECSTART(void);
interrupt void dmaRxIsr(void);
interrupt void dmaTxIsr(void);

// <-- NOVO PROTÓTIPO para a função de processamento
void processAudioFlanger(void);


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
        DMA_DMACCR_REPEAT_ALWAYS,    // (Lição: Loop Infinito)
        DMA_DMACCR_AUTOINIT_ON,      // (Lição: Loop Infinito)
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
    AUDIO_BUFFER_SIZE,  // DMACEN  - 96 elementos
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
        DMA_DMACSDP_DST_DARAMPORT1,
        DMA_DMACSDP_SRCBEN_NOBURST,
        DMA_DMACSDP_SRCPACK_OFF,
        DMA_DMACSDP_SRC_PERIPH,
        DMA_DMACSDP_DATATYPE_16BIT
    ), /* DMACSDP */

    DMA_DMACCR_RMK(
        DMA_DMACCR_DSTAMODE_POSTINC,
        DMA_DMACCR_SRCAMODE_CONST,
        DMA_DMACCR_ENDPROG_OFF,
        DMA_DMACCR_WP_DEFAULT,
        DMA_DMACCR_REPEAT_ALWAYS,    /* Lição: Loop Infinito */
        DMA_DMACCR_AUTOINIT_ON,      /* Lição: Loop Infinito */
        DMA_DMACCR_EN_STOP,
        DMA_DMACCR_PRIO_HI,
        DMA_DMACCR_FS_ELEMENT,
        DMA_DMACCR_SYNC_REVT1
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

    (DMA_AdrPtr)0x5800, // DMACSSAL
    0,                  // DMACSSAU
    (DMA_AdrPtr)0x0000, // DMACDSAL
    0,                  // DMACDSAU
    AUDIO_BUFFER_SIZE,  // DMACEN
    1,                  // DMACFN
    0,                  // DMACSFI
    0,                  // DMACSEI
    0,                  // DMACDFI
    0                   // DMACDEI
};


/*
 * configAudioDma( )
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

    dmaRxConfig.dmacdsal = (DMA_AdrPtr)(((Uint32)&g_rxBuffer) << 1);

    dmaRxHandle = DMA_open(DMA_CHA1, 0);
    DMA_config(dmaRxHandle, &dmaRxConfig);

    CSL_init();
    IRQ_setVecs((Uint32)(&VECSTART));

    rcvEventId = DMA_getEventId(dmaRxHandle);
    xmtEventId = DMA_getEventId(dmaTxHandle);
    
    IRQ_disable(rcvEventId);
    IRQ_disable(xmtEventId);
    IRQ_clear(rcvEventId);
    IRQ_clear(xmtEventId);

    IRQ_plug(rcvEventId, &dmaRxIsr);
    IRQ_plug(xmtEventId, &dmaTxIsr);

    IRQ_enable(rcvEventId);

    
    IRQ_globalEnable();
}

void startAudioDma (void)
{
    DMA_start(dmaRxHandle);
    DMA_start(dmaTxHandle);
}
// =========================================================================

void stopAudioDma (void)
{
    DMA_stop(dmaRxHandle);
    DMA_stop(dmaTxHandle);
}


void changeTone (void)
{
    // Não fazer nada.
    // ( se o SW2 for premido)
}


void processAudioFlanger(void)
{
    // (Todas as variáveis locais que estavam na ISR)
    int i;
    Int32 lfoSin_Q15;
    Int32 currentDelay_L; // (Q15)
    Int16 y_n, x_n, x_n_L;
    Uint16 readIndex;

    // (O loop de processamento que estava na ISR)
    for (i = 0; i < AUDIO_BUFFER_SIZE; i++) // (Lembre-se, AUDIO_BUFFER_SIZE = 128)
    {
        /* --- 1. Calcular o Atraso L(n) (Ponto Fixo) --- */

        // (Ler o valor do LFO da nossa tabela)
        lfoSin_Q15 = (Int32)g_lfoTable[g_lfoIndex]; // (Q15: -32767 a +32767)

        // (Avançar o ponteiro do LFO)
        // (Usamos o g_lfoPhaseInc (float) para evitar "drift" de fase)
        g_lfoIndex = (Uint16)(g_lfoIndex + (g_lfoPhaseInc * AUDIO_BUFFER_SIZE)) % LFO_SIZE;

        // (L(n) = L0 + A * LFO)
        // (A * lfoSin_Q15) -> (300 * Q15) >> 15 = (Q0 * Q15) >> 15 = Q0
        currentDelay_L = FLANGER_L0 + ((FLANGER_A * lfoSin_Q15) >> 16);

        /* --- 2. Ler a Amostra Atrasada x[n - L(n)] --- */

        // (Encontrar o índice de leitura no buffer circular)
        readIndex = (g_flangerWriteIndex - (int)currentDelay_L) & FLANGER_DELAY_MASK;

        // (Ler a amostra atrasada)
        x_n_L = g_flangerBuffer[readIndex];

        /* --- 3. Aplicar a Equação 10.33 (Ponto Fixo) --- */

        // (Pegar a amostra "seca" que acabou de chegar)
        x_n = g_rxBuffer[i];

        // (y(n) = x(n) + g * x[n - L(n)])
        // (g * x_n_L) -> (Q15 * Q0) >> 15 = (Q15 * Q15) >> 15 = Q15
        // (y_n = (x_n / 2) + (g*x_n_L / 2))
        y_n = (x_n >> 1) + (Int16)(((Int32)FLANGER_g * (Int32)x_n_L) >> 16);

        /* --- 4. Escrever de volta e Salvar --- */

        // (Escrever o áudio "molhado" no buffer de saída)
        g_txBuffer[i] = y_n;

        // (Guardar o áudio "seco" no buffer de atraso para o futuro)
        g_flangerBuffer[g_flangerWriteIndex] = x_n;

        // (Avançar o "ponteiro" de escrita do buffer circular)
        g_flangerWriteIndex = (g_flangerWriteIndex + 1) & FLANGER_DELAY_MASK;
    }
}


// interrupção receiver
interrupt void dmaRxIsr(void)
{
    processAudioFlanger();
}


interrupt void dmaTxIsr(void)
{
    // Não faz nada
}

void initLFO(void)
{
    int i;
    float rad;
    for (i = 0; i < 256; i++)
    {
        // (Gerar um seno de 0.0 a 1.0 e escalar para Q15)
        rad = (float)i / 256.0 * (2.0 * PI);
        g_lfoTable[i] = (Int16)(sinf(rad) * 32767.0);
    }
}
