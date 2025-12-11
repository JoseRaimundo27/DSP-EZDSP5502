#include"ezdsp5502.h"
#include"ezdsp5502_mcbsp.h"
#include "csl_dma.h"
#include "csl_irq.h"
#include <math.h>
#include "icomplex.h"

//#include "pitch_params.h"
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
// ========================================================
// ==================== VARIAVEIS BUFFER DE ENTRADA E SAIDA==========================
#pragma DATA_SECTION(g_rxBuffer, "dmaMem")
#pragma DATA_ALIGN(g_rxBuffer, 4096)
Uint16 g_rxBuffer[AUDIO_BUFFER_SIZE]; // Onde o "Line In" escreve (BUFFER DE ENTRADA)

#pragma DATA_SECTION(g_txBuffer, "dmaMem")
#pragma DATA_ALIGN(g_txBuffer, 4096)
Uint16 g_txBuffer[AUDIO_BUFFER_SIZE]; // De onde o "Headphone" lê (BUFFER DE SAÍDA)
// ========================================================

// ================= VARIÁVEIS DO PITCH SHIFT =================
#define PS_BUFFER_LEN 4096        // buffer circular grande
#define PS_CROSSFADE 128          // tamanho do crossfade
#define PITCH_RATIO 0.5f         // 1.0 = igual, 1.3 = +30%, 0.8 = -20%

#pragma DATA_SECTION(psBuffer, "dmaMem")
Int16 psBuffer[PS_BUFFER_LEN];

#pragma DATA_SECTION(pitchTemp, "dmaMem")
Uint16 pitchTemp[AUDIO_BLOCK_SIZE];



float readPos = 0;       // posição de leitura flutuante
Uint16 writePos = 0;     // posição de escrita
// ============================================================

// ================= VARIÁVEIS DO REVERB SCHROEDER =================

// Buffer Gigante (Pool de Memória)
#pragma DATA_SECTION(g_reverbMemory, "dmaMem") // Ou onde você guarda seus buffers
Int16 g_reverbMemory[REVERB_MEM_SIZE];

// Ponteiros para o inicio de cada buffer dentro do Pool
Int16 *pC1, *pC2, *pC3, *pC4;
Int16 *pAP1, *pAP2;

// Índices atuais (Cabeçotes de Leitura/Escrita)
int idxC1 = 0, idxC2 = 0, idxC3 = 0, idxC4 = 0;
int idxAP1 = 0, idxAP2 = 0;

//(ISRs):
extern void VECSTART(void);
interrupt void dmaRxIsr(void);
interrupt void dmaTxIsr(void);

// As funcoes aceitam ponteiros para os blocos de áudio (pingpong)
void processAudioLoopback(Uint16* rxBlock, Uint16* txBlock);
void processAudioPitchShifter(Uint16* rxBlock, Uint16* txBlock);
void processAudioReverb(Uint16* rxBlock, Uint16* txBlock);


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


void processAudioReverb(Uint16* rxBlock, Uint16* txBlock)
{
    int i;
        Int16 x_n, y_n;

        // Variáveis temporárias
        Int16 combOut1, combOut2, combOut3, combOut4;
        Int16 sumCombs;
        Int16 ap1_in, ap1_out, ap1_bufferVal;
        Int16 ap2_in, ap2_out, ap2_bufferVal;
        Int32 tempCalc; // Para contas de 32 bits

        for (i = 0; i < AUDIO_BLOCK_SIZE; i++)
        {
            x_n = rxBlock[i];

            // --- ESTÁGIO 1: 4 FILTROS PENTE EM PARALELO ---
            // Input: x_n reduzido (>>2) para evitar overflow na soma

            // Comb 1
            Int16 buffVal1 = pC1[idxC1];
            combOut1 = buffVal1; // A saída é o que estava no buffer
            // Feedback: Buffer = Input + (Output * Gain)
            tempCalc = (Int32)(x_n >> 2) + (((Int32)buffVal1 * C1_GAIN) >> 15);
            pC1[idxC1] = (Int16)tempCalc;
            // Avançar índice circular
            if (++idxC1 >= C1_LEN) idxC1 = 0;

            // Comb 2
            Int16 buffVal2 = pC2[idxC2];
            combOut2 = buffVal2;
            tempCalc = (Int32)(x_n >> 2) + (((Int32)buffVal2 * C2_GAIN) >> 15);
            pC2[idxC2] = (Int16)tempCalc;
            if (++idxC2 >= C2_LEN) idxC2 = 0;

            // Comb 3
            Int16 buffVal3 = pC3[idxC3];
            combOut3 = buffVal3;
            tempCalc = (Int32)(x_n >> 2) + (((Int32)buffVal3 * C3_GAIN) >> 15);
            pC3[idxC3] = (Int16)tempCalc;
            if (++idxC3 >= C3_LEN) idxC3 = 0;

            // Comb 4
            Int16 buffVal4 = pC4[idxC4];
            combOut4 = buffVal4;
            tempCalc = (Int32)(x_n >> 2) + (((Int32)buffVal4 * C4_GAIN) >> 15);
            pC4[idxC4] = (Int16)tempCalc;
            if (++idxC4 >= C4_LEN) idxC4 = 0;


            // --- ESTÁGIO 2: SOMA ---
            // Somar as 4 saídas
            sumCombs = combOut1 + combOut2 + combOut3 + combOut4;


            // --- ESTÁGIO 3: FILTRO ALL-PASS 1 ---
            // Input = sumCombs
            ap1_in = sumCombs;
            ap1_bufferVal = pAP1[idxAP1];

            // Equação All-Pass (Schroeder):
            // Output = -Gain*Input + Buffer
            // Buffer_New = Input + Gain*Buffer

            // 1. Calcular Buffer Novo
            tempCalc = (Int32)ap1_in + (((Int32)ap1_bufferVal * AP_GAIN) >> 15);
            pAP1[idxAP1] = (Int16)tempCalc; // Grava no delay

            // 2. Calcular Saída
            // Output = Buffer_Old - (Gain * Input)  <-- Forma canônica mais segura
            ap1_out = ap1_bufferVal - (Int16)(((Int32)ap1_in * AP_GAIN) >> 15);

            if (++idxAP1 >= AP1_LEN) idxAP1 = 0;


            // --- ESTÁGIO 4: FILTRO ALL-PASS 2 ---
            // Input = ap1_out
            ap2_in = ap1_out;
            ap2_bufferVal = pAP2[idxAP2];

            // Mesmo processo do AP1
            tempCalc = (Int32)ap2_in + (((Int32)ap2_bufferVal * AP_GAIN) >> 15);
            pAP2[idxAP2] = (Int16)tempCalc;

            ap2_out = ap2_bufferVal - (Int16)(((Int32)ap2_in * AP_GAIN) >> 15);

            if (++idxAP2 >= AP2_LEN) idxAP2 = 0;


            // --- SAÍDA FINAL ---
            y_n = ap2_out;
            txBlock[i] = y_n;
        }
}


void processAudioPitchShifter(Uint16* rxBlock, Uint16* txBlock)
{
    int i;
    float localRead = readPos;

    for(i = 0; i < AUDIO_BLOCK_SIZE; i++)
    {
        Int16 x = rxBlock[i];

        // =======================
        // 1) GRAVA NO BUFFER
        // =======================
        psBuffer[writePos] = x;
        writePos++;
        if(writePos >= PS_BUFFER_LEN) writePos = 0;

        // =======================
        // 2) LÊ COM POSIÇÃO FRACIONÁRIA (interpolação linear)
        // =======================
        int p0 = (int)localRead;
        int p1 = p0 + 1;
        if(p1 >= PS_BUFFER_LEN) p1 = 0;

        float frac = localRead - p0;
        float y = (1.0f - frac)*psBuffer[p0] + frac*psBuffer[p1];

        // =======================
        // 3) CROSSFADE AUTOMÁTICO PARA EVITAR CLICKS
        // =======================
        float dist = fabs((float)writePos - localRead);
        if(dist < PS_CROSSFADE)
        {
            float g = dist / PS_CROSSFADE;   // 1 → 0
            y *= g;
        }

        // salvar saída
        txBlock[i] = (Int16)y;

        // =======================
        // 4) AVANÇA POSIÇÃO DE LEITURA
        // =======================
        localRead += PITCH_RATIO;

        if(localRead >= PS_BUFFER_LEN)
            localRead -= PS_BUFFER_LEN;
    }

    readPos = localRead;
}

void processAudioPitchPlusReverb(Uint16* rxBlock, Uint16* txBlock)
{
    int i;

    // 1) APLICA PITCH NO BLOCO DE ENTRADA → vai para pitchTemp
    for(i = 0; i < AUDIO_BLOCK_SIZE; i++)
        pitchTemp[i] = rxBlock[i]; // só cópia temporária

    processAudioPitchShifter(pitchTemp, pitchTemp);

    // 2) APLICA REVERB NA SAÍDA DO PITCH
    processAudioReverb(pitchTemp, txBlock);
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
            processAudioPitchPlusReverb(pRx, pTx);
            break;
        case 1: // Flanger
            processAudioReverb(pRx, pTx);
            break;

        case 2: // Tremolo
            processAudioPitchShifter(pRx, pTx);
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

void initReverb(void)
{
    int i;
        // 1. Limpar memória
        for(i=0; i<REVERB_MEM_SIZE; i++) g_reverbMemory[i] = 0;

        // 2. Configurar os ponteiros (Mapeamento de Memória)
        // Cada ponteiro começa onde o anterior terminou
        pC1 = &g_reverbMemory[0];
        pC2 = &g_reverbMemory[C1_LEN];
        pC3 = &g_reverbMemory[C1_LEN + C2_LEN];
        pC4 = &g_reverbMemory[C1_LEN + C2_LEN + C3_LEN];

        pAP1 = &g_reverbMemory[C1_LEN + C2_LEN + C3_LEN + C4_LEN];
        pAP2 = &g_reverbMemory[C1_LEN + C2_LEN + C3_LEN + C4_LEN + AP1_LEN];

        // 3. Zerar índices
        idxC1 = 0; idxC2 = 0; idxC3 = 0; idxC4 = 0;
        idxAP1 = 0; idxAP2 = 0;
}

void initPitchShift(void) {
    int i;
    for(i=0; i<PS_BUFFER_LEN; i++)
        psBuffer[i] = 0;

    readPos = 0;
    writePos = 0;
}


void initAlgorithms(void) {
    initPitchShift();
    initReverb();
}
