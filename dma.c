#include"ezdsp5502.h"
#include"ezdsp5502_mcbsp.h"
#include "csl_dma.h"
#include "csl_irq.h"
#include <math.h>
#include "icomplex.h"

#include "flanger_params.h"
#include "tremolo_params.h"
#include "reverb_params.h"

#define AUDIO_BUFFER_SIZE 4096 // BUFFER MAIOR
#define FS 48000
#define PI 3.14159265359

//---------------   VARIÁVEIS PARA OS BUFFERS DE ENTRADA E SAIDA     -------------------------
#pragma DATA_SECTION(g_rxBuffer, "dmaMem")
#pragma DATA_ALIGN(g_rxBuffer, 4096)
Uint16 g_rxBuffer[AUDIO_BUFFER_SIZE]; // Onde o "Line In" escreve (BUFFER DE ENTRADA)

#pragma DATA_SECTION(g_txBuffer, "dmaMem")
#pragma DATA_ALIGN(g_txBuffer, 4096)
Uint16 g_txBuffer[AUDIO_BUFFER_SIZE]; // De onde o "Headphone" lê (BUFFER DE SAÍDA)

//---------------   VARIÁVEIS PARA O FLANGER     -------------------------
#pragma DATA_SECTION(g_flangerBuffer, "flangerMem")
#pragma DATA_ALIGN(g_flangerBuffer, 4)
Int16 g_flangerBuffer[FLANGER_DELAY_SIZE]; // O nosso buffer circular
volatile Uint16 g_flangerWriteIndex = 0;  // Onde escrevemos no buffer

Int16 g_lfoTable[LFO_SIZE];
volatile Uint16 g_lfoIndex = 0; 
float g_lfoPhaseInc = (LFO_SIZE * FLANGER_fr) / (float)FS; // (Calculado 1 vez)


//--------------- VARIÁVEIS PARA O REVERB (FIR) -------------------------
// (Buffer de Atraso para x(n-l))
#pragma DATA_SECTION(g_reverbBuffer, "flangerMem")
#pragma DATA_ALIGN(g_reverbBuffer, 4)
Int16 g_reverbBuffer[REVERB_DELAY_SIZE];
volatile Uint16 g_reverbWriteIndex = 0;

// (Array para a Resposta ao Impulso h(l))
#pragma DATA_SECTION(g_reverbIR, "flangerMem")
Int16 g_reverbIR[REVERB_IR_SIZE];


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

void processAudioTremolo(void)
{
    int i;
    Int16 x_n, y_n;
    Int32 lfo_val_Q15;
    Int32 m_Q15;
    Int32 env_Q15;
    Int32 x_n_scaled;

    // (O loop de processamento)
    for (i = 0; i < AUDIO_BUFFER_SIZE; i++) 
    {
        /* --- 1. Obter Amostra "Seca" e aplicar ganho --- */
        // (x_n = g_rxBuffer[i] * 0.7)
        x_n = g_rxBuffer[i];
        x_n_scaled = ((Int32)x_n * (Int32)TREMOLO_INPUT_GAIN_Q15) >> 15;

        /* --- 2. Obter valor do LFO (Bipolar Q15: -1.0 a +1.0) --- */
        lfo_val_Q15 = (Int32)g_lfoTable[g_lfoIndex];

        // (Avançar o ponteiro do LFO)
        // (NOTA: Estamos a usar o 'g_lfoPhaseInc' global.
        //  Para usar a frequência do Tremolo, g_lfoPhaseInc deve ser
        //  calculado usando TREMOLO_fr na sua inicialização)
        g_lfoIndex = (Uint16)(g_lfoIndex + (g_lfoPhaseInc * AUDIO_BUFFER_SIZE)) % LFO_SIZE;

        /* --- 3. Calcular Envelope de Modulação (Q15) --- */
        
        // (m = LFO * Depth) -> (Q15 * Q15) >> 15 = Q15
        m_Q15 = ((Int32)lfo_val_Q15 * (Int32)TREMOLO_DEPTH_Q15) >> 15;
        
        // (env = m + Offset) -> (Q15 + Q15) = Q15
        // (Isto irá variar de 0 (se LFO=-1) a 32768 (se LFO=+1))
        env_Q15 = m_Q15 + (Int32)TREMOLO_OFFSET_Q15;
        
        /* --- 4. Aplicar Envelope (Saída = Envelope * Amostra) --- */

        // (y(n) = env * x_n_scaled) -> (Q15 * Q15) >> 15 = Q15
        y_n = (Int16)( ((Int32)env_Q15 * x_n_scaled) >> 15 );


        /* --- 5. Escrever de volta --- */
        
        // (Escrever o áudio "molhado" no buffer de saída)
        g_txBuffer[i] = y_n;
        
        // (Não precisamos de salvar nada no buffer de delay como no flanger)
    }
}

void processAudioReverb(void)
{
    int i; // (Índice do buffer DMA)
    int l; // (Índice da Resposta ao Impulso h(l))

    Int16 x_n, y_n;
    Int32 y_n_32; // (Acumulador de 32 bits para a soma)
    Uint16 readIndex;
    Int16 x_n_l; // Amostra atrasada x(n-l)
    Int16 h_l;   // Coeficiente h(l)

    // (Loop principal para cada amostra no buffer DMA)
    for (i = 0; i < AUDIO_BUFFER_SIZE; i++)
    {
        /* --- 1. Obter amostra e guardar no buffer de atraso --- */
        x_n = g_rxBuffer[i];
        g_reverbBuffer[g_reverbWriteIndex] = x_n;

        /* --- 2. Calcular a Convolução (Eq 10.25) --- */
        y_n_32 = 0; // Resetar o acumulador

        // (Loop interno para y(n) = SOMA( h(l) * x(n-l) ))
        for (l = 0; l < REVERB_IR_SIZE; l++)
        {
            // Obter o coeficiente h(l)
            h_l = g_reverbIR[l];

            // Obter a amostra atrasada x(n-l)
            readIndex = (g_reverbWriteIndex - l) & REVERB_DELAY_MASK;
            x_n_l = g_reverbBuffer[readIndex];

            // Acumular: (Q15 * Q15)
            y_n_32 += (Int32)h_l * (Int32)x_n_l;
        }

        /* --- 3. Escalar e escrever a saída --- */

        // (Escalar o resultado de volta para 16 bits)
        // (A soma Q15*Q15 resultou em Q30, voltamos para Q15)
        y_n = (Int16)(y_n_32 >> 15);
        g_txBuffer[i] = y_n;


        /* --- 4. Avançar o ponteiro de escrita --- */
        g_reverbWriteIndex = (g_reverbWriteIndex + 1) & REVERB_DELAY_MASK;
    }
}

// interrupção receiver
interrupt void dmaRxIsr(void)
{
    //processAudioFlanger();
    processAudioTremolo();
    //processAudioReverb();
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

void initReverb(void)
{
    int i;

    // 1. Zerar todo o buffer
    for (i = 0; i < REVERB_IR_SIZE; i++) {
        g_reverbIR[i] = 0;
    }

    // 2. Criar h(l) - (Valores Q15)
    // (Som Direto)
    g_reverbIR[0] = 16384; // 0.5 (Som seco)

    // (Reflexões Iniciais - alguns ecos fortes)
    g_reverbIR[4] = 8192; // 0.25
    g_reverbIR[7] = 6144; // ~0.18

    // (Reverberação Tardia - ecos mais fracos e próximos)
    g_reverbIR[11] = 2048; // ~0.06
    g_reverbIR[13] = 1024; // ~0.03
    g_reverbIR[15] = 512;  // ~0.01
}

void initAlgorithms(void) {
    initLFO();
    initReverb();
}
