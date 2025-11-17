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

// =================== VARIÁVEIS EXTERNA ===================
extern volatile Uint8 currentState;
// Flag para controlar qual bloco (Ping ou Pong) está ativo
static volatile Uint16 dmaPingPongFlag = 0;
// =====================================
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
    // Resetar o flag antes de começar
    dmaPingPongFlag = 0;


    DMA_start(dmaRxHandle);
    DMA_start(dmaTxHandle);
}
// =========================================================================

void stopAudioDma (void)
{
    DMA_stop(dmaRxHandle);
    DMA_stop(dmaTxHandle);
}

void processAudioLoopback(Uint16* rxBlock, Uint16* txBlock)
{
    int i;

    // Itera sobre todas as amostras do bloco atual
    for (i = 0; i < AUDIO_BLOCK_SIZE; i++)
    {
        // Copia diretamente o valor da entrada para a saída
        txBlock[i] = rxBlock[i];
    }
}

void processAudioFlanger(Uint16* rxBlock, Uint16* txBlock)
{
    int i;
    Int32 lfoSin_Q15;     // Valor do LFO, o M(n) da Eq. 10.34
    Int32 currentDelay_L; // O atraso final L(n) em amostras
    Int16 y_n, x_n, x_n_L;  // y(n), x(n), e x[n-L(n)]
    Uint16 readIndex;

    for (i = 0; i < AUDIO_BLOCK_SIZE; i++)
    {     
        // lfoSin_Q15 = M(n), o valor do oscilador
        lfoSin_Q15 = (Int32)g_lfoTable[g_lfoIndex];

        // (Avançar o ponteiro do LFO para o próximo M(n+1))
        g_lfoIndex = (Uint16)(g_lfoIndex + (g_lfoPhaseInc * AUDIO_BUFFER_SIZE)) % LFO_SIZE;

        // Calcula L(n) = L0 + (A_samples * M(n))
        // FLANGER_L0 é o L0 (atraso médio)
        // FLANGER_A é a amplitude em amostras (A_samples)
        currentDelay_L = FLANGER_L0 + ((FLANGER_A * lfoSin_Q15) >> 16);

        /* --- 2. Ler a Amostra Atrasada x[n - L(n)] --- */
        
        // Encontra o índice de leitura no buffer circular
        readIndex = (g_flangerWriteIndex - (int)currentDelay_L) & FLANGER_DELAY_MASK;
        
        // x_n_L é a amostra atrasada x[n - L(n)]
        x_n_L = g_flangerBuffer[readIndex];

        /* --- 3. Aplicar a Equação 10.33 (Modificada) --- */

        // x_n é a amostra de entrada "seca" x(n)
        x_n = rxBlock[i];

        // y(n) = (0.5 * x(n)) + (g * x[n - L(n),(x_n >> 1) --> Corresponde a 0.5 * x(n)
        // (FLANGER_g * x_n_L) >> 16 --> Corresponde a g * x[n - L(n)]
        // pequena modificação: o sinal "seco" (x_n) é atenuado 
        // pela metade (>> 1) para evitar clipping.
        y_n = (x_n >> 1) + (Int16)(((Int32)FLANGER_g * (Int32)x_n_L) >> 16);

        // Escreve a saída final y(n)
        txBlock[i] = y_n;

        g_flangerBuffer[g_flangerWriteIndex] = x_n;

        // (Avançar o "ponteiro" de escrita do buffer circular)
        g_flangerWriteIndex = (g_flangerWriteIndex + 1) & FLANGER_DELAY_MASK;
    }
}


void processAudioTremolo(Uint16* rxBlock, Uint16* txBlock)
{
    int i;
    Int16 x_n, y_n;      // x_n é a entrada x(n), y_n é a saída y(n)
    Int32 lfo_val_Q15; // Representa M(n), o valor do oscilador
    Int32 m_Q15;       // Termo intermediário: A * M(n)
    Int32 env_Q15;     // O envelope [Offset + A*M(n)]
    Int32 x_n_scaled;  // A entrada x(n) *com* um ganho (desvio da fórmula)

    // O loop agora itera apenas sobre o BLOCO (metade do buffer)
    for (i = 0; i < AUDIO_BLOCK_SIZE; i++)
    {        
        // x_n é a amostra de entrada original, o x(n) da fórmula
        x_n = rxBlock[i];

        // O código aplica um ganho de entrada (InputGain) antes.
        // x_n_scaled = InputGain * x(n) mas definimos o inputGain como 1, não faz diferença
        x_n_scaled = ((Int32)x_n * (Int32)TREMOLO_INPUT_GAIN_Q15) >> 15;


        // lfo_val_Q15 é o M(n) da fórmula: o valor atual do LFO
        lfo_val_Q15 = (Int32)g_lfoTable[g_lfoIndex];

        // (Avançar o ponteiro do LFO para a próxima amostra, M(n+1))
        g_lfoIndex = (Uint16)(g_lfoIndex + (g_lfoPhaseInc * AUDIO_BUFFER_SIZE)) % LFO_SIZE;

        /* Calcular Envelope de Modulação [Offset + A*M(n)] --- */
        // m_Q15 = A * M(n)
        // 'A' (Amplitude/Depth) é TREMOLO_DEPTH_Q15
        // 'M(n)' (Oscilador) é lfo_val_Q15
        m_Q15 = ((Int32)lfo_val_Q15 * (Int32)TREMOLO_DEPTH_Q15) >> 15;

        // env_Q15 = [Offset + A*M(n)]
        // TREMOLO_OFFSET_Q15 tambem vale 1
        env_Q15 = m_Q15 + (Int32)TREMOLO_OFFSET_Q15;
        
        /* (Saída = Envelope * Amostra) --- */
        // y_n = env_Q15 * x_n_scaled
        // Fórmula final do CÓDIGO:
        // y(n) = [1 + A*M(n)] * [1 * x(n)]
        y_n = (Int16)( ((Int32)env_Q15 * x_n_scaled) >> 15 );
        
        // (Escrever no BLOCO de saída, não no buffer global)
        txBlock[i] = y_n; // Armazena o y(n) final
    }
}

// Agora processa apenas um BLOCO (Ping ou Pong)
void processAudioReverb(Uint16* rxBlock, Uint16* txBlock)
{
    int i; 
    int l; // (Índice da Resposta ao Impulso h(l)) - Representa 'l'
    Int16 x_n, y_n; // x_n é x(n), y_n é y(n)
    Int32 y_n_32; // Acumulador para o Somatório (Σ)
    Uint16 readIndex;
    Int16 x_n_l; // Amostra atrasada x(n-l)
    Int16 h_l;   // Coeficiente h(l)

    for (i = 0; i < AUDIO_BLOCK_SIZE; i++)
    {
        // x_n = x(n)
        x_n = rxBlock[i];
        // Guarda x(n) no buffer circular (delay line)
        g_reverbBuffer[g_reverbWriteIndex] = x_n;

        // Inicia o acumulador do Somatório (Σ) em zero
        y_n_32 = 0; 

        // Este loop 'l' implementa o Somatório: Σ de l=0 até L-1
        // (REVERB_IR_SIZE é o 'L' da fórmula)
        for (l = 0; l < REVERB_IR_SIZE; l++)
        {
            // h_l = h(l)
            // Pega o coeficiente da Resposta ao Impulso
            h_l = g_reverbIR[l];

            // Encontra a posição de x(n-l) no buffer circular
            readIndex = (g_reverbWriteIndex - l) & REVERB_DELAY_MASK;
            // x_n_l = x(n-l)
            x_n_l = g_reverbBuffer[readIndex];

            // Implementa a multiplicação e soma: h(l) * x(n-l)
            y_n_32 += (Int32)h_l * (Int32)x_n_l;
        }

        /* Escalar e escrever a saída y(n) --- */
        y_n = (Int16)(y_n_32 >> 15);

        // (Escrever no BLOCO de saída, não no buffer global)
        txBlock[i] = y_n;

        // Avança o ponteiro 'n' para 'n+1' no buffer circular
        g_reverbWriteIndex = (g_reverbWriteIndex + 1) & REVERB_DELAY_MASK;
    }
}


interrupt void dmaRxIsr(void)
{
    Uint16* pRx; // Ponteiro para o bloco de Rx
    Uint16* pTx; // Ponteiro para o bloco de Tx

    if (dmaPingPongFlag == 0)
    {
        // --- Bloco PING ---
        pRx = &g_rxBuffer[0];
        pTx = &g_txBuffer[0];
        dmaPingPongFlag = 1; // Próxima interrupção será o Pong
    }
    else
    {
        // --- Bloco PONG ---
        pRx = &g_rxBuffer[AUDIO_BLOCK_SIZE];
        pTx = &g_txBuffer[AUDIO_BLOCK_SIZE];
        dmaPingPongFlag = 0; // Próxima interrupção será o Ping
    }

    // --- SELETOR DE EFEITO ---

    switch (currentState)
    {
        case 0:
            processAudioLoopback(pRx,pTx);
            break;
        case 1: // Flanger
            processAudioFlanger(pRx, pTx);
            break;

        case 2: // Tremolo
            processAudioTremolo(pRx, pTx);
            break;

        case 3: // Reverb
            processAudioReverb(pRx, pTx);
            break;

        default: // Segurança (caso currentState seja corrompido)
            processAudioLoopback(pRx, pTx); // Default para Loopback
            break;
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
