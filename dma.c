#include"ezdsp5502.h"
#include"ezdsp5502_mcbsp.h"
#include "csl_dma.h"
#include "csl_irq.h" // <-- ADICIONADO: Necessário para as Interrupções
#include <math.h>         // (Necessário para a tabela Twiddle)
#include "icomplex.h"     // (Necessário para a FFT intrínseca)

#define N_FFT   128     // (É o nosso AUDIO_BUFFER_SIZE)
#define EXP_FFT 7       // (Porque 2^7 = 128)

/* Definições da FFT */
#define FFT_FLAG        0
#define IFFT_FLAG       1
#define SCALE_FLAG      0
#define NOSCALE_FLAG    1
#define PI 3.1415926

Uint8 dmaState = 0; // (Não mais usado, mas mantido por segurança)


#define AUDIO_BUFFER_SIZE 96 // Manter o mesmo tamanho do demo

#pragma DATA_SECTION(g_rxBuffer, "dmaMem")
#pragma DATA_ALIGN(g_rxBuffer, 4)
Uint16 g_rxBuffer[AUDIO_BUFFER_SIZE]; // Onde o "Line In" escreve (BUFFER DE ENTRADA)

#pragma DATA_SECTION(g_txBuffer, "dmaMem")
#pragma DATA_ALIGN(g_txBuffer, 4)
Uint16 g_txBuffer[AUDIO_BUFFER_SIZE]; // De onde o "Headphone" lê (BUFFER DE SAÍDA)

/* Buffers Globais para a FFT */

#pragma DATA_SECTION(g_fftBuffer, "fftMem")
#pragma DATA_ALIGN(g_fftBuffer, 4)
complex g_fftBuffer[N_FFT];     // O buffer de trabalho "X" da FFT

#pragma DATA_SECTION(g_twiddleTable, "fftMem")
#pragma DATA_ALIGN(g_twiddleTable, 4)
complex g_twiddleTable[EXP_FFT]; // A nossa tabela "W" (Twiddle)
// =========================================================================

extern void bit_rev(complex *, Uint16);
extern void fft(complex *, Uint16, complex *, Uint16, Uint16);

//(ISRs):
extern void VECSTART(void);
interrupt void dmaRxIsr(void);
interrupt void dmaTxIsr(void);



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
        DMA_DMACICR_FRAMEIE_ON,      /* <-- A "CAMPAINHA"! Ligar Interrupção! */
        DMA_DMACICR_FIRSTHALFIE_OFF,
        DMA_DMACICR_DROPIE_OFF,
        DMA_DMACICR_TIMEOUTIE_OFF
    ), /* DMACICR */

    (DMA_AdrPtr)0x5800, // DMACSSAL - Endereço do McBSP DRR1 (Receive)
    0,                  // DMACSSAU
    (DMA_AdrPtr)0x0000, // DMACDSAL - (Vamos definir isto na configAudioDma)
    0,                  // DMACDSAU
    AUDIO_BUFFER_SIZE,  // DMACEN  - 96 elementos
    1,                  // DMACFN  - Single frame
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

    /*
 Preparar os Dados (Copiar Int -> Complex)
     */
    for (i = 0; i < N_FFT; i++)
    {
        g_fftBuffer[i].re = g_rxBuffer[i]; // (Parte Real é o áudio)
        g_fftBuffer[i].im = 0;             // (Parte Imaginária é 0)
    }


     
    // (Re-ordenar)
    bit_rev(g_fftBuffer, EXP_FFT);
    
    // (FFT)
    fft(g_fftBuffer, EXP_FFT, g_twiddleTable, FFT_FLAG, SCALE_FLAG);

    // -----------------------------------------------------
    // <-- O "PROCESSAMENTO" NO DOMÍNIO DA FREQUÊNCIA IRIA AQUI
    // (Por agora, não fazemos nada, só testamos a "canalização")
    // -----------------------------------------------------

    // (FFT Inversa)
    // fft(g_fftBuffer, EXP_FFT, g_twiddleTable, IFFT_FLAG, SCALE_FLAG);

    //Devolve os dados para o buffer de saida
    for (i = 0; i < N_FFT; i++)
    {
        // (A FFT Inversa devolve o áudio na Parte Real)
        g_txBuffer[i] = g_fftBuffer[i].re;
    }
}

/* Transmissão. Não a usamos, mas tem de existir. */
interrupt void dmaTxIsr(void)
{
    // Não faz nada
}

void initFFT(void)
{
    Uint16 L, LE, LE1;
    
    // (Este loop é 100% "roubado" do intrinsic_fftTest.c)
    for (L=1; L<=EXP_FFT; L++)
    {
        LE=1<<L;
        LE1=LE>>1;
        g_twiddleTable[L-1].re =  (Int16)(32768.0*cos(PI/LE1));
        g_twiddleTable[L-1].im = -(Int16)(32768.0*sin(PI/LE1));
    }
}
