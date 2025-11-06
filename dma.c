//////////////////////////////////////////////////////////////////////////////
// * File name: dma.c
// *                                                                          
// * Description:  DMA functions.
// *                                                                          
// * Copyright (C) 2011 Texas Instruments Incorporated - http://www.ti.com/ 
// * Copyright (C) 2011 Spectrum Digital, Incorporated
// *                                                                          
// *                                                                          
// *  Redistribution and use in source and binary forms, with or without      
// *  modification, are permitted provided that the following conditions      
// *  are met:                                                                
// *                                                                          
// *    Redistributions of source code must retain the above copyright        
// *    notice, this list of conditions and the following disclaimer.         
// *                                                                          
// *    Redistributions in binary form must reproduce the above copyright     
// *    notice, this list of conditions and the following disclaimer in the   
// *    documentation and/or other materials provided with the                
// *    distribution.                                                         
// *                                                                          
// *    Neither the name of Texas Instruments Incorporated nor the names of   
// *    its contributors may be used to endorse or promote products derived   
// *    from this software without specific prior written permission.         
// *                                                                          
// *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS     
// *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT       
// *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR   
// *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT    
// *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,   
// *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT        
// *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,   
// *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY   
// *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT     
// *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE   
// *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.    
// *                                                                          
//////////////////////////////////////////////////////////////////////////////
#include"ezdsp5502.h"
#include"ezdsp5502_mcbsp.h"
#include "csl_dma.h"

DMA_Handle myhDma;
Uint8 dmaState = 0;

/* -3dB 1 KHz sine sampled at 48 KHz for L/R channels*/
Uint16 Sine_1K[96] = {
0x0000,0x0000,0x0B6C,0x0B6C,0x16A6,0x16A6,0x217C,0x217C,0x2BC0,0x2BC0,0x3544,0x3544,0x3DDF,0x3DDF,0x456B,0x456B, 
0x4BC7,0x4BC7,0x50D7,0x50D7,0x5485,0x5485,0x56C0,0x56C0,0x5780,0x5780,0x56C0,0x56C0,0x5485,0x5485,0x50D7,0x50D7, 
0x4BC7,0x4BC7,0x456B,0x456B,0x3DDF,0x3DDF,0x3544,0x3544,0x2BC0,0x2BC0,0x217C,0x217C,0x16A6,0x16A6,0x0B6C,0x0B6C, 
0x0000,0x0000,0xF494,0xF494,0xE95A,0xE95A,0xDE84,0xDE84,0xD440,0xD440,0xCABC,0xCABC,0xC221,0xC221,0xBA95,0xBA95, 
0xB439,0xB439,0xAF29,0xAF29,0xAB7B,0xAB7B,0xA940,0xA940,0xA880,0xA880,0xA940,0xA940,0xAB7B,0xAB7B,0xAF29,0xAF29, 
0xB439,0xB439,0xBA95,0xBA95,0xC221,0xC221,0xCABC,0xCABC,0xD440,0xD440,0xDE84,0xDE84,0xE95A,0xE95A,0xF494,0xF494
};

/* -3 dB 2 KHz sine sampled at 48 KHz for L/R channels*/
Int16 Sine_2K[96] = {
0x0000,0x0000,0x16A6,0x16A6,0x2BC0,0x2BC0,0x3DDF,0x3DDF,0x4BC7,0x4BC7,0x5485,0x5485,0x5780,0x5780,0x5485,0x5485, 
0x4BC7,0x4BC7,0x3DDF,0x3DDF,0x2BC0,0x2BC0,0x16A6,0x16A6,0x0000,0x0000,0xE95A,0xE95A,0xD440,0xD440,0xC221,0xC221, 
0xB439,0xB439,0xAB7B,0xAB7B,0xA880,0xA880,0xAB7B,0xAB7B,0xB439,0xB439,0xC221,0xC221,0xD440,0xD440,0xE95A,0xE95A,
0x0000,0x0000,0x16A6,0x16A6,0x2BC0,0x2BC0,0x3DDF,0x3DDF,0x4BC7,0x4BC7,0x5485,0x5485,0x5780,0x5780,0x5485,0x5485, 
0x4BC7,0x4BC7,0x3DDF,0x3DDF,0x2BC0,0x2BC0,0x16A6,0x16A6,0x0000,0x0000,0xE95A,0xE95A,0xD440,0xD440,0xC221,0xC221, 
0xB439,0xB439,0xAB7B,0xAB7B,0xA880,0xA880,0xAB7B,0xAB7B,0xB439,0xB439,0xC221,0xC221,0xD440,0xD440,0xE95A,0xE95A,
};

/* DMA configuration structure */
DMA_Config myconfig = { 
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
        DMA_DMACCR_REPEAT_ALWAYS,    // Repeat conditioncsl_dma.h
        DMA_DMACCR_AUTOINIT_ON,      // Auto initialization bit
        DMA_DMACCR_EN_STOP,          // Channel enable
        DMA_DMACCR_PRIO_HI,          // Channel priority
        DMA_DMACCR_FS_ELEMENT,       // Frame\Element Sync
        DMA_DMACCR_SYNC_XEVT1        // Synchronization control
    ), /* DMACCR */
    
    DMA_DMACICR_RMK(
        DMA_DMACICR_AERRIE_OFF,
        DMA_DMACICR_BLOCKIE_OFF ,    // Whole block interrupt enable
        DMA_DMACICR_LASTIE_OFF,      // Last frame Interrupt enable
        DMA_DMACICR_FRAMEIE_OFF,     // Whole frame interrupt enable
        DMA_DMACICR_FIRSTHALFIE_OFF, // HAlf frame interrupt enable
        DMA_DMACICR_DROPIE_OFF,      // Sync event drop interrupt enable
        DMA_DMACICR_TIMEOUTIE_OFF    // Time out inetrrupt enable
    ), /* DMACICR */
    
    (DMA_AdrPtr)0x5000, // DMACSSAL - DMA source set to random location (Set to sine array later)
    0,                  // DMACSSAU
    (DMA_AdrPtr)0x5804, // DMACDSAL - DMA destination is DXR1
    0,                  // DMACDSAU 
    96,                 // DMACEN  - 96 elements (48 for each channel)
    1,                  // DMACFN  - Single frame
    0,                  // DMACSFI - Source frame index
    0,                  // DMACSEI - Source element index
    0,                  // DMACDFI - Destination frame index
    2                   // DMACDEI - Destination element index
};

/*
 *  configAudioDma( )
 *
 *    Configure DMA for Audio
 */
void configAudioDma (void)
{
    /* Set source address to Sine_1K */
    myconfig.dmacssal = (DMA_AdrPtr)(((Uint32)&Sine_1K) << 1);

    myhDma = DMA_open(DMA_CHA0, 0);  // Open DMA Channel 0
    DMA_config(myhDma, &myconfig);   // Configure Channel
}

/*
 *  startAudioDma( )
 *
 *    Start DMA transfer for Audio
 */
void startAudioDma (void)
{
    DMA_start(myhDma); // Begin Transfer
}

/*
 *  stopAudioDma( )
 *
 *    Stop DMA transfer for Audio
 */
void stopAudioDma (void)
{
    DMA_stop(myhDma);  // Stop Transfer
}

/*
 *  changeTone( )
 *
 *    Swap between 1KHz and 2KHz audio tones
 */
void changeTone (void)
{
    if(dmaState == 0)
    {
        DMA_close(myhDma);  // Close DMA
        
        /* Set source address to Sine_2K */
        myconfig.dmacssal = (DMA_AdrPtr)(((Uint32)&Sine_2K) << 1);
        
        myhDma = DMA_open(DMA_CHA0, 0);  // Open DMA Channel 0
        DMA_config(myhDma, &myconfig);   // Configure Channel
        DMA_start(myhDma);               // Start DMA
        EZDSP5502_MCBSP_init( );         // Reinitalize McBSP
        dmaState = 1;                    // Swap state
    }
    else
    {
        DMA_close(myhDma);  // Close DMA
        
        /* Set source address to Sine_1K */
        myconfig.dmacssal = (DMA_AdrPtr)(((Uint32)&Sine_1K) << 1);
        
        myhDma = DMA_open(DMA_CHA0, 0);  // Open DMA Channel 0
        DMA_config(myhDma, &myconfig);   // Configure Channel
        DMA_start(myhDma);               // Start DMA
        EZDSP5502_MCBSP_init( );         // Reinitalize McBSP
        dmaState = 0;                    // Swap state
    }
}
