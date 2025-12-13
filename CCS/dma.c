#include"ezdsp5502.h"
#include"ezdsp5502_mcbsp.h"
#include "csl_dma.h"
#include "csl_irq.h"
#include <math.h>
#include "icomplex.h"

#include "pitch_params.h"
#include "reverb_params.h"
#include "phaser_params.h"
#include "auto_wah_params.h"
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
// ================= VARIÁVEIS DO PITCH SHIFT =================

#pragma DATA_SECTION(g_pitchBuffer, "dmaMem") // Ou "data_br_buf" conforme seu linker
Int16 g_pitchBuffer[PITCH_BUF_SIZE];
Uint16 g_pitchWriteIndex = 0;

// Tabela de Seno para Janelamento (0 a PI/2)
// 257 pontos para cobrir de 0 a 1.0 no ganho
// Valores Q15 (0 a 32767)
#pragma DATA_SECTION(g_sineWindow, "dmaMem")
const Int16 g_sineWindow[257] = {
    0, 201, 402, 603, 804, 1005, 1206, 1406, 1607, 1808, 2009, 2209, 2410, 2610, 2811, 3011,
    3211, 3411, 3611, 3811, 4011, 4210, 4410, 4609, 4808, 5007, 5205, 5403, 5601, 5799, 5997, 6195,
    6392, 6589, 6786, 6983, 7179, 7375, 7571, 7766, 7961, 8156, 8351, 8545, 8739, 8933, 9126, 9319,
    9511, 9703, 9895, 10087, 10278, 10469, 10659, 10849, 11038, 11227, 11416, 11604, 11792, 11980, 12167, 12353,
    12539, 12724, 12909, 13093, 13277, 13460, 13643, 13825, 14007, 14188, 14368, 14548, 14727, 14906, 15084, 15261,
    15438, 15614, 15790, 15965, 16139, 16313, 16486, 16658, 16830, 17001, 17171, 17341, 17510, 17678, 17846, 18013,
    18180, 18346, 18511, 18676, 18840, 19003, 19165, 19327, 19488, 19648, 19808, 19967, 20125, 20282, 20439, 20595,
    20750, 20905, 21059, 21212, 21364, 21516, 21667, 21817, 21966, 22115, 22263, 22410, 22556, 22701, 22846, 22990,
    23133, 23276, 23417, 23558, 23698, 23837, 23975, 24113, 24249, 24385, 24520, 24654, 24787, 24919, 25050, 25180,
    25310, 25438, 25566, 25693, 25819, 25944, 26068, 26191, 26314, 26435, 26556, 26676, 26795, 26913, 27030, 27146,
    27261, 27375, 27488, 27600, 27711, 27821, 27930, 28038, 28145, 28251, 28356, 28460, 28563, 28665, 28766, 28866,
    28965, 29063, 29160, 29256, 29351, 29445, 29538, 29630, 29721, 29811, 29900, 29988, 30075, 30161, 30246, 30330,
    30413, 30495, 30576, 30656, 30735, 30813, 30890, 30966, 31041, 31115, 31188, 31260, 31331, 31401, 31470, 31538,
    31605, 31671, 31736, 31800, 31863, 31925, 31986, 32046, 32105, 32163, 32220, 32276, 32331, 32385, 32438, 32490,
    32541, 32591, 32640, 32688, 32735, 32767 // Fim (1.0 em Q15)
};

// Variáveis de Controle (Inteiros de 32 bits para alta performance)
Uint32 g_phasor_int = 0;
Uint32 g_phasorStep_int = 0;

// ================= VARIÁVEIS DO PHASER =================
#pragma DATA_SECTION(g_phaserXPrev, "dmaMem")
Int16 g_phaserXPrev[PHASER_NUM_STAGES];

#pragma DATA_SECTION(g_phaserYPrev, "dmaMem")
Int16 g_phaserYPrev[PHASER_NUM_STAGES];

Int16 g_lfoTable[LFO_SIZE];
volatile Int16 g_phaserLastOutput = 0;
volatile Uint16 g_phaserLfoIndex = 0;

// ================= VARIÁVEIS DO AUTO-WAH =================
// Estado do Envelope (Volume atual)
volatile Int16 g_autoWahEnvState = 0;

// Estados do Filtro SVF (State Variable Filter)
volatile Int16 g_autoWahStateLow = 0;
volatile Int16 g_autoWahStateBand = 0;


//(ISRs):
extern void VECSTART(void);
interrupt void dmaRxIsr(void);
interrupt void dmaTxIsr(void);


// As funcoes aceitam ponteiros para os blocos de áudio (pingpong)
void processAudioLoopback(Uint16* rxBlock, Uint16* txBlock);
void processAudioPhaser(Uint16* rxBlock, Uint16* txBlock);
void processAudioAutoWah(Uint16* rxBlock, Uint16* txBlock);
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

// Variável Global que o Loop de Áudio vai ler
ReverbPreset g_currentPreset;

const ReverbPreset g_presetBank[6] = {
    // 0: Reverb Puro (Sem Pitch) - "Warm Hall"
    { 0, 0, {28180, 28180, 27850, 27100}, 19660 },

    // 1: Pitch A (Grave -5 semi) + Reverb Longo
    // Ratio A = 43738
    { 1, 43738, {29000, 28500, 28000, 27500}, 21000 },

    // 2: Pitch Bb (Rádio -4 semi) + Reverb Médio
    // Ratio Bb = 41284
    { 1, 41284, {28180, 28180, 27850, 27100}, 19660 },

    // 3: Pitch Db (Leve -1 semi) + Reverb Curto
    // Ratio Db = 34715
    { 1, 34715, {26000, 26000, 25000, 24000}, 16000 },

    // 4: Pitch Fb (Agudo +2 semi) + Reverb Denso
    // Ratio Fb = 29192
    { 1, 29192, {28500, 28500, 28500, 28500}, 22000 },
    
    // 5: Outro efeito extra (Ex: Monstro total -12 semi)
    // Ratio -12 = 65535
    { 1, 65535, {28000, 28000, 28000, 28000}, 20000 }
};


void loadPreset(int index) {
    if (index < 0 || index > 5) return;
    
    g_currentPreset = g_presetBank[index];

    if (g_currentPreset.enablePitch) {
        float ratio = (float)g_currentPreset.pitchRatio / 32767.0f;
        float stepFloat = (1.0f - ratio) / (float)PITCH_WINDOW_SIZE;
        g_phasorStep_int = (Uint32)(stepFloat * 4294967296.0f);
    } else {
        g_phasorStep_int = 0;
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

        Int16 curCGain0 = g_currentPreset.cGains[0];
        Int16 curCGain1 = g_currentPreset.cGains[1];
        Int16 curCGain2 = g_currentPreset.cGains[2];
        Int16 curCGain3 = g_currentPreset.cGains[3];
        Int16 curAPGain = g_currentPreset.apGain;

        for (i = 0; i < AUDIO_BLOCK_SIZE; i++)
        {
            x_n = rxBlock[i];

            // --- ESTÁGIO 1: 4 FILTROS PENTE EM PARALELO ---
            // Input: x_n reduzido (>>2) para evitar overflow na soma

            // Comb 1
            Int16 buffVal1 = pC1[idxC1];
            combOut1 = buffVal1; // A saída é o que estava no buffer
            // Feedback: Buffer = Input + (Output * Gain)
            tempCalc = (Int32)(x_n >> 2) + (((Int32)buffVal1 * curCGain0) >> 15);
            pC1[idxC1] = (Int16)tempCalc;
            // Avançar índice circular
            if (++idxC1 >= C1_LEN) idxC1 = 0;

            // Comb 2
            Int16 buffVal2 = pC2[idxC2];
            combOut2 = buffVal2;
            tempCalc = (Int32)(x_n >> 2) + (((Int32)buffVal2 * curCGain1) >> 15);
            pC2[idxC2] = (Int16)tempCalc;
            if (++idxC2 >= C2_LEN) idxC2 = 0;

            // Comb 3
            Int16 buffVal3 = pC3[idxC3];
            combOut3 = buffVal3;
            tempCalc = (Int32)(x_n >> 2) + (((Int32)buffVal3 * curCGain2) >> 15);
            pC3[idxC3] = (Int16)tempCalc;
            if (++idxC3 >= C3_LEN) idxC3 = 0;

            // Comb 4
            Int16 buffVal4 = pC4[idxC4];
            combOut4 = buffVal4;
            tempCalc = (Int32)(x_n >> 2) + (((Int32)buffVal4 * curCGain3) >> 15);
            pC4[idxC4] = (Int16)tempCalc;
            if (++idxC4 >= C4_LEN) idxC4 = 0;


            // --- ESTÁGIO 2: SOMA ---
            // Somar as 4 saídas
            sumCombs = combOut1 + combOut2 + combOut3 + combOut4;


            // --- ESTÁGIO 3: FILTRO ALL-PASS 1 ---
            // Input = sumCombs
            ap1_in = sumCombs;
            ap1_bufferVal = pAP1[idxAP1];
            tempCalc = (Int32)ap1_in + (((Int32)ap1_bufferVal * curAPGain) >> 15);
            pAP1[idxAP1] = (Int16)tempCalc; // Grava no delay
            ap1_out = ap1_bufferVal - (Int16)(((Int32)ap1_in * curAPGain) >> 15);
            if (++idxAP1 >= AP1_LEN) idxAP1 = 0;


            // --- ESTÁGIO 4: FILTRO ALL-PASS 2 ---
            ap2_in = ap1_out;
            ap2_bufferVal = pAP2[idxAP2];
            tempCalc = (Int32)ap2_in + (((Int32)ap2_bufferVal * curAPGain) >> 15);
            pAP2[idxAP2] = (Int16)tempCalc;
            ap2_out = ap2_bufferVal - (Int16)(((Int32)ap2_in * curAPGain) >> 15);
            if (++idxAP2 >= AP2_LEN) idxAP2 = 0;


            // --- SAÍDA FINAL ---
            y_n = ap2_out;
            txBlock[i] = y_n;
        }
}

// Pitch Shifter com Interpolação Linear e Janela Senoidal (Sem Float)
void processAudioPitchShifter(Uint16* rxBlock, Uint16* txBlock)
{
    int i;

    if (g_currentPreset.enablePitch == 0) {
        for(i=0; i<AUDIO_BLOCK_SIZE; i++) txBlock[i] = rxBlock[i];
        return;
    }

    Int16 x_n, y_n;
    Uint32 phA, phB;
    Uint16 normPosA;
    Uint32 delayFixedA, delayFixedB;
    int idxA_0, idxA_1, idxB_0, idxB_1;
    Uint16 fracA, fracB;
    Int16 sA, sB;
    Int16 gainA, gainB;
    Int32 mix32;

    for (i = 0; i < AUDIO_BLOCK_SIZE; i++)
    {
        x_n = rxBlock[i];

        // Grava Buffer
        g_pitchBuffer[g_pitchWriteIndex] = x_n;

        // Calcula Phasors
        phA = g_phasor_int;
        phB = g_phasor_int + 2147483648UL;

        // Calcula Delay
        normPosA = (Uint16)(phA >> 16);
        Uint16 normPosB = (Uint16)(phB >> 16);
        delayFixedA = (Uint32)normPosA * PITCH_WINDOW_SIZE;
        delayFixedB = (Uint32)normPosB * PITCH_WINDOW_SIZE;

        // Ponteiros de Leitura
        Uint32 rdPtrA = ((Uint32)g_pitchWriteIndex << 16) - delayFixedA;
        Uint32 rdPtrB = ((Uint32)g_pitchWriteIndex << 16) - delayFixedB;

        // Índices Inteiros e Fração
        idxA_0 = (int)(rdPtrA >> 16) & PITCH_MASK;
        fracA  = (Uint16)(rdPtrA & 0xFFFF);
        idxA_1 = (idxA_0 + 1) & PITCH_MASK;

        idxB_0 = (int)(rdPtrB >> 16) & PITCH_MASK;
        fracB  = (Uint16)(rdPtrB & 0xFFFF);
        idxB_1 = (idxB_0 + 1) & PITCH_MASK;

        // Interpolação Linear (Amostras)
        Int16 vA0 = g_pitchBuffer[idxA_0];
        Int16 vA1 = g_pitchBuffer[idxA_1];
        sA = vA0 + (Int16)(((Int32)(vA1 - vA0) * fracA) >> 16);

        Int16 vB0 = g_pitchBuffer[idxB_0];
        Int16 vB1 = g_pitchBuffer[idxB_1];
        sB = vB0 + (Int16)(((Int32)(vB1 - vB0) * fracB) >> 16);

        // Ganhos (Janela triangular)
         // Canal A
        if (normPosA < 32768) {
            // Primeira metade: crescente de 0 a 32767
            // normPosA >> 1 converte 0-32767 para 0-16383, depois <<1 mantém paridade
            gainA = (Int16)(normPosA);  // 0 a 32767
        } else {
            // Segunda metade: decrescente de 32767 a 0
            // 65535 - normPosA: 32767 a 0 quando normPosA: 32768 a 65535
            gainA = (Int16)(65535 - normPosA);
        }
        
        // Canal B
        if (normPosB < 32768) {
            gainB = (Int16)(normPosB);
        } else {
            gainB = (Int16)(65535 - normPosB);
        }
        
        // Mix
        mix32 = ((Int32)sA * gainA) + ((Int32)sB * gainB);
        y_n = (Int16)(mix32 >> 15);

        // Saída e Atualização
        txBlock[i] = y_n;
        g_pitchWriteIndex = (g_pitchWriteIndex + 1) & PITCH_MASK;
        g_phasor_int += g_phasorStep_int;
    }
}

void processAudioPitchReverb(Uint16* rxBlock, Uint16* txBlock)
{
    processAudioPitchShifter(rxBlock, txBlock);
    processAudioReverb(txBlock, txBlock);
}


void processAudioPhaser(Uint16* rxBlock, Uint16* txBlock)
{
    int i, j;
    Int16 in_sample, out_sample;
    Int32 lfo_raw, lfo_norm;
    Int16 alpha_Q15;
    Int32 alpha_range = (Int32)PHASER_MAX_ALPHA_Q15 - (Int32)PHASER_MIN_ALPHA_Q15;
    Int32 term1, term2, term3, y_long;
    Int16 x_n, y_n, x_prev, y_prev;
    Int32 fb_val, input_with_fb;

    for (i = 0; i < AUDIO_BLOCK_SIZE; i++)
    {
        in_sample = (Int16)rxBlock[i];

        // LFO
        lfo_raw = (Int32)g_lfoTable[g_phaserLfoIndex];
        lfo_norm = (lfo_raw + 32768) >> 1;
        alpha_Q15 = PHASER_MIN_ALPHA_Q15 + (Int16)((lfo_norm * alpha_range) >> 15);
        g_phaserLfoIndex = (g_phaserLfoIndex + 1) % LFO_SIZE;

        // Feedback com proteção
        fb_val = ((Int32)g_phaserLastOutput * (Int32)PHASER_FEEDBACK_Q15) >> 15;
        if (fb_val > 16000) fb_val = 16000;
        else if (fb_val < -16000) fb_val = -16000;

        input_with_fb = (Int32)in_sample + fb_val;
        if (input_with_fb > 32767) input_with_fb = 32767;
        else if (input_with_fb < -32768) input_with_fb = -32768;
        x_n = (Int16)input_with_fb;

        // Cadeia de Filtros
        for (j = 0; j < PHASER_NUM_STAGES; j++)
        {
            x_prev = g_phaserXPrev[j];
            y_prev = g_phaserYPrev[j];
            term1 = -((Int32)alpha_Q15 * (Int32)x_n) >> 15;
            term2 = (Int32)x_prev;
            term3 = ((Int32)alpha_Q15 * (Int32)y_prev) >> 15;
            y_long = term1 + term2 + term3;
            if (y_long > 32767) y_long = 32767;
            else if (y_long < -32768) y_long = -32768;
            y_n = (Int16)y_long;
            g_phaserXPrev[j] = x_n;
            g_phaserYPrev[j] = y_n;
            x_n = y_n;
        }
        g_phaserLastOutput = x_n;

        // Mix
        out_sample = (in_sample >> 1) + (x_n >> 1);
        txBlock[i] = out_sample;
    }
}

// -------------------------------------------------------------
// [NOVO] PROCESSAMENTO AUTO-WAH (Envelope Follower + SVF)
// -------------------------------------------------------------
void processAudioAutoWah(Uint16* rxBlock, Uint16* txBlock)
{
    int i;
    Int16 in_sample;
    Int16 abs_in;
    Int32 env_diff, env_calc;
    Int16 freq_f_Q15;
    Int32 freq_range;
    Int16 low, band;
    Int32 term_damp_band, term_f_band, term_f_high;
    Int32 high_long, low_long, band_long;
    Int32 out_scaled;

    // Calcula range de frequência (Max - Min)
    freq_range = (Int32)AUTO_WAH_MAX_FREQ_Q15 - (Int32)AUTO_WAH_MIN_FREQ_Q15;

    // Carrega estados para registradores
    low = g_autoWahStateLow;
    band = g_autoWahStateBand;

    for (i = 0; i < AUDIO_BLOCK_SIZE; i++)
    {
        in_sample = (Int16)rxBlock[i];

        // 1. Envelope Follower
        abs_in = (in_sample < 0) ? -in_sample : in_sample;
        env_diff = (Int32)abs_in - (Int32)g_autoWahEnvState;
        env_calc = (Int32)g_autoWahEnvState + ((env_diff * AUTO_WAH_ENV_SPEED_Q15) >> 15);
        g_autoWahEnvState = (Int16)env_calc;

        // 2. Mapeamento de Frequência
        Int32 mod_amount = ((Int32)g_autoWahEnvState * (Int32)AUTO_WAH_SENSITIVITY_Q15) >> 15;
        if (mod_amount > 32767) mod_amount = 32767;

        freq_f_Q15 = AUTO_WAH_MIN_FREQ_Q15 + (Int16)((mod_amount * freq_range) >> 15);

        // 3. Filtro SVF (Chamberlin)
        Int32 input_reduced = (Int32)in_sample >> 1; // Reduz entrada para evitar distorção

        // --- High Pass ---
        term_damp_band = ((Int32)AUTO_WAH_DAMP_Q15 * (Int32)band) >> 15;
        high_long = input_reduced - (Int32)low - term_damp_band;
        if (high_long > 32767) high_long = 32767;
        else if (high_long < -32768) high_long = -32768;
        Int16 high = (Int16)high_long;

        // --- Low Pass ---
        term_f_band = ((Int32)freq_f_Q15 * (Int32)band) >> 15;
        low_long = (Int32)low + term_f_band;
        if (low_long > 32767) low_long = 32767;
        else if (low_long < -32768) low_long = -32768;
        low = (Int16)low_long;

        // --- Band Pass (Saída do Wah) ---
        term_f_high = ((Int32)freq_f_Q15 * (Int32)high) >> 15;
        band_long = (Int32)band + term_f_high;
        if (band_long > 32767) band_long = 32767;
        else if (band_long < -32768) band_long = -32768;
        band = (Int16)band_long;

        // 4. Saída
        // Compensa a redução da entrada (x2)
        out_scaled = (Int32)band << 1;
        if (out_scaled > 32767) out_scaled = 32767;
        else if (out_scaled < -32768) out_scaled = -32768;

        txBlock[i] = (Int16)out_scaled;
    }

    // Salva estados
    g_autoWahStateLow = low;
    g_autoWahStateBand = band;
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

    // Dentro da sua função principal de processamento ou interrupção:

    switch (currentState)
    {
        case 0:
            processAudioLoopback(pRx, pTx);
            break;

        case 1: // Reverb Puro (Preset 0)
            loadPreset(0); 
            processAudioPitchReverb(pRx, pTx); // Chama Pitch (Bypass) -> Reverb
            break;

        case 2: // Pitch A + Reverb (Preset 1)
            loadPreset(1);
            processAudioPitchReverb(pRx, pTx);
            break;

        case 3: // Pitch Bb + Reverb (Preset 2)
            loadPreset(2);
            processAudioPitchReverb(pRx, pTx);
            break;
            
        case 4: // Pitch Db + Reverb (Preset 3)
            loadPreset(3);
            processAudioPitchReverb(pRx, pTx);
            break;

        case 5: // Pitch Fb + Reverb (Preset 4)
            loadPreset(4);
            processAudioPitchReverb(pRx, pTx);
            break;
        case 6: // Flanger
            processAudioPhaser(pRx, pTx);
            break;

        case 7: // Tremolo / AutoWah
            processAudioAutoWah(pRx, pTx);
            break;

        default:
            processAudioLoopback(pRx, pTx);
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

void initLFO(void)
{
    int i;
    float rad;
    for (i = 0; i < 256; i++) {
        rad = (float)i / 256.0 * (2.0 * PI);
        g_lfoTable[i] = (Int16)(sinf(rad) * 32767.0);
    }
}

void initPhaser(void)
{
    int i;
    for (i = 0; i < PHASER_NUM_STAGES; i++) {
        g_phaserXPrev[i] = 0;
        g_phaserYPrev[i] = 0;
    }
    g_phaserLastOutput = 0;
    g_phaserLfoIndex = 0;
}

// [NOVO] Inicializa Auto Wah
void initAutoWah(void)
{
    g_autoWahEnvState = 0;
    g_autoWahStateLow = 0;
    g_autoWahStateBand = 0;
}

void initPitchShift(void) {
    int i;
    // 1. Limpar Buffer
    for(i=0; i<PITCH_BUF_SIZE; i++) g_pitchBuffer[i] = 0;
    g_pitchWriteIndex = 0;
    g_phasor_int = 0;

    // 2. Calcular o Passo (Step) baseado no Ratio definido no .h
    // Fazemos a conta com float AQUI (na inicialização) para não gastar CPU depois.
    
    // Converte o define Q15 para float (Ex: 36780 -> 1.122)
    float ratio = (float)PITCH_RATIO_VAL / 32767.0f;
    
    // Fórmula: (1.0 - ratio) / WindowSize
    // Nota: Se ratio > 1 (Voz Grossa), o resultado é negativo.
    // O Uint32 vai tratar o negativo como um número gigante (Wrap Around), o que funciona perfeitamente.
    float stepFloat = (1.0f - ratio) / (float)PITCH_WINDOW_SIZE;
    
    // Converter para Q32 (Escala total de 32 bits)
    // 4294967296.0f é 2^32
    g_phasorStep_int = (Uint32)(stepFloat * 4294967296.0f);
}

void initAlgorithms(void) {
    initLFO();
    initPitchShift();
    initReverb();
    initPhaser();
    initAutoWah();
}
