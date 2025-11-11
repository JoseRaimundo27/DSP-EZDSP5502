//////////////////////////////////////////////////////////////////////////////
// * File name: main.c
// *
// * Description:  Main function.
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
#include "ezdsp5502.h"
#include "ezdsp5502_mcbsp.h"
#include "ezdsp5502_i2cgpio.h"
#include "dma.h"
#include "timer.h"
#include "lcd.h"
#include "i2cgpio.h"
#include "csl_chip.h"
#include "stdio.h"

void configPort(void);
void checkTimer(void);
void checkSwitch(void);

extern void initPLL(void);
extern void initAIC3204( );
extern Int16 oled_start( );

extern Uint16 timerFlag;  // Timer interrupt flag
Uint8 ledNum = 3;         // I2C GPIO LED number
Uint8 sw1State = 0;       // SW1 state
Uint8 sw2State = 0;       // SW2 state

/*
 *  main( )
 *
 *    The following functions are performed by this demo
 *    1. Outputs audio tone from STEREO OUT jack
 *    2. Uses DMA to service McBSP for audio
 *    3. Uses timer0 to create time base
 *    4. Use SW1 to swap between two timer periods
 *    5. Use SW2 to swap between two audio tones
 *    6. Uses timer0 interrupt to trigger LED state change
 *    7. Generates output on LCD screen
 */
void main(void)
 {
    /* Demo Initialization */
    //initPLL( );         // Initialize PLL
    EZDSP5502_init( );  // Board Initialization
    //initLed( );         // Init I2C GPIO for LEDs
    configPort( );      // Configure MUXs
    //initTimer0( );      // Initialize timer
    initAIC3204( );     // Initialize AIC3204
    configAudioDma( );  // Configure DMA for Audio tone
    
    /* Start Demo */
    startAudioDma ( );        // Start DMA to service McBSP
    EZDSP5502_MCBSP_init( );  // Configure and start McBSP
    //startTimer0( );           // Start timer
    //oled_start( );            // Configure and start output on display
    
    while(1)
    {
        //checkTimer( );  // Check if timer interrupt was triggered
        checkSwitch( ); // Check if a switch was pressed
    }
}

/*
 *  configPort( )
 *
 *   Configure MUXs for AIC3204
 */
void configPort(void)
{
    /* Set to McBSP1 mode */
    EZDSP5502_I2CGPIO_configLine( BSP_SEL1, OUT );
    EZDSP5502_I2CGPIO_writeLine(  BSP_SEL1, LOW );
    
    /* Enable McBSP1 */
    EZDSP5502_I2CGPIO_configLine( BSP_SEL1_ENn, OUT );
    EZDSP5502_I2CGPIO_writeLine(  BSP_SEL1_ENn, LOW );
}

/*
 *  toggleLED( )
 *
 *   Toggle XF LED to opposite state
 */
static toggleLED(void)
{
    if(CHIP_FGET(ST1_55, XF))
        CHIP_FSET(ST1_55, XF,CHIP_ST1_55_XF_OFF);  // Turn off LED
    else
        CHIP_FSET(ST1_55, XF, CHIP_ST1_55_XF_ON);  // Turn on LED
}

/*
 *  checkTimer( )
 *
 *   See if timer interrupt was triggered. ISR sets timerFlag to 1.
 *
 *
 *
 *   void checkTimer(void)
{
    if(timerFlag ==1)
    {
        //timerFlag =0;    // Set flag back to 0
        toggleLED();     // Toggle XF LED
        EZDSP5502_I2CGPIO_writeLine( ledNum,   // Toggle User LED ledNum
                 (~EZDSP5502_I2CGPIO_readLine(ledNum) & 0x01) );
        if( ledNum > 6)  // I2C GPIOs 3 to 6 control LEDs
            ledNum = 3;  // Go back to first LED
        ledNum++;        // Go to next LED
    }
}
 */


/*
 *  checkSwitch( )
 *
 *   Check if SW1 or SW2 was pressed.
 *     SW1 controls Timer period
 *     SW2 controls Audio Tone
 */
void checkSwitch(void)
{
    /* Check SW1 */
    if(!(EZDSP5502_I2CGPIO_readLine(SW0))) // Is SW1 pressed?
    {
        if(sw1State)          // Was previous state not pressed?
        {
            //changeTimer();    // Change timer period
            sw1State = 0;     // Set state to 0 to allow only single press
        }
    }
    else                      // SW1 not pressed
        sw1State = 1;         // Set state to 1 to allow timer change
        
    /* Check SW2 */
    if(!(EZDSP5502_I2CGPIO_readLine(SW1))) // Is SW2 pressed?
    {
        if(sw2State)          // Was previous state not pressed?
        {
            changeTone();     // Change audio tone
            sw2State = 0;     // Set state to 0 to allow only single press
        }
    }
    else                      // SW2 not pressed
        sw2State = 1;         // Set state to 1 to allow tone change
}

