#define CHIP_5502 1

#include <stdio.h>

#include <csl.h>
#include <csl_mcbsp.h>
#include <csl_irq.h>

#include "ezdsp5502.h"
#include "ezdsp5502_i2c.h"
#include "ezdsp5502_i2cgpio.h"
#include  "ezdsp5502_mcbsp.h"
#include "csl_dma.h"
#include "csl_mcbsp.h"
#include "aic3204_codec_init.h"

//---------Global constants---------
#define N       128
volatile Uint16 g_audio_frame_pronto = 0;
//---------Global data definition---------

/* Define transmit and receive buffers */
#pragma DATA_SECTION(xmt,"dmaMem")
Uint16 xmt[N];
#pragma DATA_SECTION(rcv,"dmaMem")
Uint16 rcv[N];


DMA_Config  dmaRcvConfig = { 
  DMA_DMACSDP_RMK(
    DMA_DMACSDP_DSTBEN_NOBURST,
    DMA_DMACSDP_DSTPACK_OFF,
    DMA_DMACSDP_DST_DARAMPORT1,
    DMA_DMACSDP_SRCBEN_NOBURST,
    DMA_DMACSDP_SRCPACK_OFF,
    DMA_DMACSDP_SRC_PERIPH,
    DMA_DMACSDP_DATATYPE_16BIT
  ),                                       /* DMACSDP  */
  DMA_DMACCR_RMK(
    DMA_DMACCR_DSTAMODE_POSTINC,
    DMA_DMACCR_SRCAMODE_CONST,
    DMA_DMACCR_ENDPROG_ON,
    DMA_DMACCR_WP_DEFAULT,
    DMA_DMACCR_REPEAT_OFF,
    DMA_DMACCR_AUTOINIT_ON,
    DMA_DMACCR_EN_STOP,
    DMA_DMACCR_PRIO_LOW,
    DMA_DMACCR_FS_DISABLE,
    DMA_DMACCR_SYNC_REVT1
  ),                                       /* DMACCR   */
  DMA_DMACICR_RMK(
    DMA_DMACICR_AERRIE_ON,
    DMA_DMACICR_BLOCKIE_OFF,
    DMA_DMACICR_LASTIE_OFF,
    DMA_DMACICR_FRAMEIE_ON,
    DMA_DMACICR_FIRSTHALFIE_OFF,
    DMA_DMACICR_DROPIE_OFF,
    DMA_DMACICR_TIMEOUTIE_OFF
  ),                                       /* DMACICR  */
    (DMA_AdrPtr)(MCBSP_ADDR(DRR11)),        /* DMACSSAL */
    0,                                     /* DMACSSAU */
    (DMA_AdrPtr)&rcv,                      /* DMACDSAL */
    0,                                     /* DMACDSAU */
    N,                                     /* DMACEN   */
    1,                                     /* DMACFN   */
    0,                                     /* DMACFI   */
    0                                      /* DMACEI   */
};

DMA_Config  dmaXmtConfig = { 
  DMA_DMACSDP_RMK(
    DMA_DMACSDP_DSTBEN_NOBURST,
    DMA_DMACSDP_DSTPACK_OFF,
    DMA_DMACSDP_DST_PERIPH,
    DMA_DMACSDP_SRCBEN_NOBURST,
    DMA_DMACSDP_SRCPACK_OFF,
    DMA_DMACSDP_SRC_DARAMPORT0,
    DMA_DMACSDP_DATATYPE_16BIT
  ),                                       /* DMACSDP  */
  DMA_DMACCR_RMK(
    DMA_DMACCR_DSTAMODE_CONST,
    DMA_DMACCR_SRCAMODE_POSTINC,
    DMA_DMACCR_ENDPROG_ON,
    DMA_DMACCR_WP_DEFAULT,
    DMA_DMACCR_REPEAT_OFF,
    DMA_DMACCR_AUTOINIT_ON,
    DMA_DMACCR_EN_STOP,
    DMA_DMACCR_PRIO_LOW,
    DMA_DMACCR_FS_DISABLE,
    DMA_DMACCR_SYNC_XEVT1
  ),                                       /* DMACCR   */
  DMA_DMACICR_RMK(
    DMA_DMACICR_AERRIE_ON,    
    DMA_DMACICR_BLOCKIE_OFF,
    DMA_DMACICR_LASTIE_OFF,
    DMA_DMACICR_FRAMEIE_ON,
    DMA_DMACICR_FIRSTHALFIE_OFF,
    DMA_DMACICR_DROPIE_OFF,
    DMA_DMACICR_TIMEOUTIE_OFF
  ),                                       /* DMACICR  */
    (DMA_AdrPtr)&xmt[1],                      /* DMACSSAL */
    0,                                     /* DMACSSAU */
    (DMA_AdrPtr)(MCBSP_ADDR(DXR11)),       /* DMACDSAL */
    0,                                     /* DMACDSAU */
    N,                                     /* DMACEN   */
    1,                                     /* DMACFN   */
    0,                                     /* DMACFI   */
    0                                      /* DMACEI   */
};

/* Define a DMA_Handle object to be used with DMA_open function */
DMA_Handle hDmaRcv, hDmaXmt;

volatile Uint16 transferComplete = FALSE;
Uint16 err = 0;
Uint16 old_intm;
Uint16 xmtEventId, rcvEventId;

//---------Function prototypes---------

/* Reference the start of the interrupt vector table */
/* This symbol is defined in file vectors.s55        */
extern void VECSTART(void);

/* Protoype for interrupt functions */
interrupt void dmaXmtIsr(void);
interrupt void dmaRcvIsr(void);
void taskFxn(void);

//---------main routine---------
void main(void)
{
    Uint16 i;

    /* Initialize CSL library - This is REQUIRED !!! */
    CSL_init();

    /* Set IVPH/IVPD to start of interrupt vector table */
    IRQ_setVecs((Uint32)(&VECSTART));

    for (i = 0; i <= N - 1; i++) {  
        xmt[i] =  i + 1;
        rcv[i] = 0;
    }

    /* Call function to effect transfer */
    taskFxn();
}

void taskFxn(void)
{
    Uint16 srcAddrHi, srcAddrLo;
    Uint16 dstAddrHi, dstAddrLo;
    Uint16 i;

    Uint16 reg_val;

 
    /* By default, the TMS320C55xx compiler assigns all data symbols word */
    /* addresses. The DMA however, expects all addresses to be byte       */
    /* addresses. Therefore, we must shift the address by 2 in order to   */
    /* change the word address to a byte address for the DMA transfer.    */ 
    srcAddrHi = (Uint16)(((Uint32)(MCBSP_ADDR(DRR11))) >> 15) & 0xFFFFu;
    srcAddrLo = (Uint16)(((Uint32)(MCBSP_ADDR(DRR11))) << 1) & 0xFFFFu;
    dstAddrHi = (Uint16)(((Uint32)(&rcv)) >> 15) & 0xFFFFu;
    dstAddrLo = (Uint16)(((Uint32)(&rcv)) << 1) & 0xFFFFu;

    dmaRcvConfig.dmacssal = (DMA_AdrPtr)srcAddrLo;
    dmaRcvConfig.dmacssau = srcAddrHi;
    dmaRcvConfig.dmacdsal = (DMA_AdrPtr)dstAddrLo;
    dmaRcvConfig.dmacdsau = dstAddrHi;

    srcAddrHi = (Uint16)(((Uint32)(&xmt[0])) >> 15) & 0xFFFFu;
    srcAddrLo = (Uint16)(((Uint32)(&xmt[0])) << 1) & 0xFFFFu;
    dstAddrHi = (Uint16)(((Uint32)(MCBSP_ADDR(DXR11))) >> 15) & 0xFFFFu;
    dstAddrLo = (Uint16)(((Uint32)(MCBSP_ADDR(DXR11))) << 1) & 0xFFFFu;

    dmaXmtConfig.dmacssal = (DMA_AdrPtr)srcAddrLo;
    dmaXmtConfig.dmacssau = srcAddrHi;
    dmaXmtConfig.dmacdsal = (DMA_AdrPtr)dstAddrLo;
    dmaXmtConfig.dmacdsau = dstAddrHi;

        /* Inicializa o barramento I2C */
    EZDSP5502_I2C_init();

    /* Chama nossa nova função para ligar o Codec */
    AIC3204_codec_init();

    printf("Codec AIC3204 Inicializado.\n");
    

    /* Open DMA channels 1 & 2 and set regs to power on defaults */
    /* Usando os canais corretos (padrão) para o McBSP1 */
    hDmaRcv = DMA_open(DMA_CHA1, DMA_OPEN_RESET); // Canal 1 para Receber
    hDmaXmt = DMA_open(DMA_CHA2, DMA_OPEN_RESET); // Canal 2 para Enviar

    /* Get interrupt event associated with DMA receive and transmit */
    xmtEventId = DMA_getEventId(hDmaXmt);
    rcvEventId = DMA_getEventId(hDmaRcv);
    
    /* Temporarily disable interrupts and clear any pending */
    /* interrupts for MCBSP transmit */
    old_intm = IRQ_globalDisable();
    
    /* Clear any pending interrupts for DMA channels */
    IRQ_clear(xmtEventId);
    IRQ_clear(rcvEventId);

    /* Enable DMA interrupt in IER register */
    IRQ_enable(xmtEventId);
    IRQ_enable(rcvEventId);

    /* Place DMA interrupt service addresses at associate vector */
    IRQ_plug(xmtEventId,&dmaXmtIsr);
    IRQ_plug(rcvEventId,&dmaRcvIsr);

    
    /* Write values from configuration structure to DMA control regs */
    DMA_config(hDmaRcv,&dmaRcvConfig);
    DMA_config(hDmaXmt,&dmaXmtConfig);
  

    /* Ela liga a energia (clocks) e configura o McBSP */
    EZDSP5502_MCBSP_init();

    /* 1. Habilita todas as interrupções (CPU) */
    IRQ_globalEnable();

    
    DMA_start(hDmaRcv);
    DMA_start(hDmaXmt);
  
    printf("Motor de áudio em tempo real iniciado!\n");

    while (1)
    {
        if (g_audio_frame_pronto) // A flag será setada pela ISR
        {
            g_audio_frame_pronto = 0; // Abaixa a flag

            /* ESTE É O SEU LOOPBACK DE ÁUDIO! */
            for(i = 0; i < N; i++)
            {
                xmt[i] = rcv[i]; // Copia o que entrou para o buffer de saída
            }
        }
    }
}

interrupt void dmaXmtIsr(void) {
   // Não precisamos mais parar o DMA
   // Apenas limpa a interrupção (se necessário, mas o AUTOINIT cuida)
}

interrupt void dmaRcvIsr(void) {
   g_audio_frame_pronto = 1; // AVISA O MAIN() QUE O ÁUDIO CHEGOU!
   // Não paramos mais o DMA. O AUTOINIT já o reiniciou.
}
