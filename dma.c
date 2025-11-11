#include"ezdsp5502.h"
#include"ezdsp5502_mcbsp.h"
#include "csl_dma.h"
#include "csl_irq.h"
#include <math.h>
#include "icomplex.h"

#include "flanger_params.h"
#include "tremolo_params.h"
#include "reverb_params.h"

#define AUDIO_BUFFER_SIZE 4096 // Tamanho TOTAL

// Processar em blocos de 2048 (dois blocos -> DMA PING PONG)
#define AUDIO_BLOCK_SIZE (AUDIO_BUFFER_SIZE / 2)
// =========================================================
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


// As funcoes aceitam ponteiros para os blocos de áudio (pingpong)
void processAudioFlanger(Uint16* rxBlock, Uint16* txBlock);
void processAudioTremolo(Uint16* rxBlock, Uint16* txBlock);
void processAudioReverb(Uint16* rxBlock, Uint16* txBlock);

// Flag para controlar qual bloco (Ping ou Pong) está ativo
static volatile Uint16 dmaPingPongFlag = 0;
// =========================================================


DMA_Config dmaTxConfig = { // (Configuração do Tx - Sem Alterações)
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
        DMA_DMACCR_REPEAT_ALWAYS,    // (Loop Infinito)
        DMA_DMACCR_AUTOINIT_ON,      // (Loop Infinito)
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
    
    (DMA_AdrPtr)0x0000,
    0,
    (DMA_AdrPtr)0x5804,
    0,
    AUDIO_BUFFER_SIZE,  // (Tamanho total)
    1,
    0,
    0,
    0,
    0
};


//Configurações da recepção
DMA_Handle dmaRxHandle;
DMA_Handle dmaTxHandle;


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
        DMA_DMACCR_REPEAT_ALWAYS,
        DMA_DMACCR_AUTOINIT_ON,
        DMA_DMACCR_EN_STOP,
        DMA_DMACCR_PRIO_HI,
        DMA_DMACCR_FS_ELEMENT,
        DMA_DMACCR_SYNC_REVT1
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

    (DMA_AdrPtr)0x5800,
    0,
    (DMA_AdrPtr)0x0000,
    0,
    AUDIO_BUFFER_SIZE,  // (Tamanho total)
    1,
    0,
    0,
    0,
    0
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

    IRQ_enable(rcvEventId); // (Ligar APENAS a interrupção de Rx)

    
    IRQ_globalEnable();
}

void startAudioDma (void)
{
    // =================== ALTERAÇÃO PING-PONG ===================
    // Resetar o flag antes de começar
    dmaPingPongFlag = 0;
    // =========================================================

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
}

// =================== ALTERAÇÃO PING-PONG ===================
// Função MODIFICADA
// Agora processa apenas um BLOCO (Ping ou Pong)
// =========================================================
void processAudioFlanger(Uint16* rxBlock, Uint16* txBlock)
{
    int i;
    Int32 lfoSin_Q15;
    Int32 currentDelay_L;
    Int16 y_n, x_n, x_n_L;
    Uint16 readIndex;

    // O loop agora itera apenas sobre o BLOCO (metade do buffer)
    for (i = 0; i < AUDIO_BLOCK_SIZE; i++)
    {
        /* --- 1. Calcular o Atraso L(n) (Ponto Fixo) --- */
        lfoSin_Q15 = (Int32)g_lfoTable[g_lfoIndex];

        // (Avançar o ponteiro do LFO)
        // (NOTA: A velocidade do LFO será o dobro se AUDIO_BLOCK_SIZE
        //  for usado aqui. Mantemos o incremento original
        //  baseado em AUDIO_BUFFER_SIZE para manter a freq. correta)
        g_lfoIndex = (Uint16)(g_lfoIndex + (g_lfoPhaseInc * AUDIO_BUFFER_SIZE)) % LFO_SIZE;

        currentDelay_L = FLANGER_L0 + ((FLANGER_A * lfoSin_Q15) >> 16);

        /* --- 2. Ler a Amostra Atrasada x[n - L(n)] --- */
        readIndex = (g_flangerWriteIndex - (int)currentDelay_L) & FLANGER_DELAY_MASK;
        x_n_L = g_flangerBuffer[readIndex];

        /* --- 3. Aplicar a Equação 10.33 (Ponto Fixo) --- */

        // (Ler do BLOCO de entrada, não do buffer global)
        x_n = rxBlock[i];

        y_n = (x_n >> 1) + (Int16)(((Int32)FLANGER_g * (Int32)x_n_L) >> 16);

        /* --- 4. Escrever de volta e Salvar --- */

        // (Escrever no BLOCO de saída, não no buffer global)
        txBlock[i] = y_n;

        // (Guardar o áudio "seco" no buffer de atraso para o futuro)
        g_flangerBuffer[g_flangerWriteIndex] = x_n;

        // (Avançar o "ponteiro" de escrita do buffer circular)
        g_flangerWriteIndex = (g_flangerWriteIndex + 1) & FLANGER_DELAY_MASK;
    }
}


// Agora processa apenas um BLOCO (Ping ou Pong)
void processAudioTremolo(Uint16* rxBlock, Uint16* txBlock)
{
    int i;
    Int16 x_n, y_n;
    Int32 lfo_val_Q15;
    Int32 m_Q15;
    Int32 env_Q15;
    Int32 x_n_scaled;

    // O loop agora itera apenas sobre o BLOCO (metade do buffer)
    for (i = 0; i < AUDIO_BLOCK_SIZE; i++)
    {
        /* --- 1. Obter Amostra "Seca" e aplicar ganho --- */

        // (Ler do BLOCO de entrada, não do buffer global)
        x_n = rxBlock[i];
        x_n_scaled = ((Int32)x_n * (Int32)TREMOLO_INPUT_GAIN_Q15) >> 15;

        /* --- 2. Obter valor do LFO (Bipolar Q15: -1.0 a +1.0) --- */
        lfo_val_Q15 = (Int32)g_lfoTable[g_lfoIndex];

        // (Avançar o ponteiro do LFO)
        // (Ver nota no Flanger: mantemos o incremento original)
        g_lfoIndex = (Uint16)(g_lfoIndex + (g_lfoPhaseInc * AUDIO_BUFFER_SIZE)) % LFO_SIZE;

        /* --- 3. Calcular Envelope de Modulação (Q15) --- */
        m_Q15 = ((Int32)lfo_val_Q15 * (Int32)TREMOLO_DEPTH_Q15) >> 15;
        env_Q15 = m_Q15 + (Int32)TREMOLO_OFFSET_Q15;
        
        /* --- 4. Aplicar Envelope (Saída = Envelope * Amostra) --- */
        y_n = (Int16)( ((Int32)env_Q15 * x_n_scaled) >> 15 );


        /* --- 5. Escrever de volta --- */
        
        // (Escrever no BLOCO de saída, não no buffer global)
        txBlock[i] = y_n;
    }
}

// Agora processa apenas um BLOCO (Ping ou Pong)
void processAudioReverb(Uint16* rxBlock, Uint16* txBlock)
{
    int i; // (Índice do bloco DMA)
    int l; // (Índice da Resposta ao Impulso h(l))

    Int16 x_n, y_n;
    Int32 y_n_32; // (Acumulador de 32 bits para a soma)
    Uint16 readIndex;
    Int16 x_n_l; // Amostra atrasada x(n-l)
    Int16 h_l;   // Coeficiente h(l)

    // O loop agora itera apenas sobre o BLOCO (metade do buffer)
    for (i = 0; i < AUDIO_BLOCK_SIZE; i++)
    {
        /* --- 1. Obter amostra e guardar no buffer de atraso --- */

        // (Ler do BLOCO de entrada, não do buffer global)
        x_n = rxBlock[i];
        g_reverbBuffer[g_reverbWriteIndex] = x_n;

        /* --- 2. Calcular a Convolução (Eq 10.25) --- */
        y_n_32 = 0; // Resetar o acumulador

        // (Loop interno para y(n) = SOMA( h(l) * x(n-l) ))
        for (l = 0; l < REVERB_IR_SIZE; l++)
        {
            h_l = g_reverbIR[l];
            readIndex = (g_reverbWriteIndex - l) & REVERB_DELAY_MASK;
            x_n_l = g_reverbBuffer[readIndex];

            y_n_32 += (Int32)h_l * (Int32)x_n_l;
        }

        /* --- 3. Escalar e escrever a saída --- */
        y_n = (Int16)(y_n_32 >> 15);

        // (Escrever no BLOCO de saída, não no buffer global)
        txBlock[i] = y_n;


        /* --- 4. Avançar o ponteiro de escrita --- */
        g_reverbWriteIndex = (g_reverbWriteIndex + 1) & REVERB_DELAY_MASK;
    }
}


interrupt void dmaRxIsr(void)
{
    if (dmaPingPongFlag == 0)
    {
        // --- Processar Bloco PING ---

        // Chamar a função de processamento para o Bloco PING
        //processAudioFlanger(&g_rxBuffer[0], &g_txBuffer[0]);
        processAudioTremolo(&g_rxBuffer[0], &g_txBuffer[0]);
        //processAudioReverb(&g_rxBuffer[0], &g_txBuffer[0]);

        dmaPingPongFlag = 1; // Na próxima interrupção, processar o PONG
    }
    else
    {
        // --- Processar Bloco PONG ---

        // Chamar a função de processamento para o Bloco PONG
        //processAudioFlanger(&g_rxBuffer[AUDIO_BLOCK_SIZE], &g_txBuffer[AUDIO_BLOCK_SIZE]);
        processAudioTremolo(&g_rxBuffer[AUDIO_BLOCK_SIZE], &g_txBuffer[AUDIO_BLOCK_SIZE]);
        //processAudioReverb(&g_rxBuffer[AUDIO_BLOCK_SIZE], &g_txBuffer[AUDIO_BLOCK_SIZE]);

        dmaPingPongFlag = 0; // Na próxima interrupção, processar o PING
    }
}


interrupt void dmaTxIsr(void)
{
    // Não faz nada
}

// (Funções de inicialização não precisam de alteração)
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
