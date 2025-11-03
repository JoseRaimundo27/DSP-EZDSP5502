//////////////////////////////////////////////////////////////////////////////
// * File name: ezdsp5502_i2cgpio.h
// *                                                                          
// * Description:  I2C GPIO header file.
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

#define I2CGPIO_I2C_ADDR 0x21

/* I2C GPIO lines */
#define I2C_P01       0
#define I2C_P00       1
#define SW1           2
#define SW0           3
#define LED3          4
#define LED2          5
#define LED1          6
#define LED0          7
#define I2C_P10       8
#define I2C_P11       9
#define I2C_P12       10
#define FTDI_UARTn    11
#define BSP_SEL1_ENn  12
#define BSP_SEL1      13
#define BSP_SEL0_ENn  14
#define BSP_SEL0      15

/* I2C GPIO direction */
#define OUT           0
#define IN            1

/* I2C GPIO STATE */
#define LOW           0
#define HIGH          1 

/* ------------------------------------------------------------------------ *
 *  Prototypes                                                              *
 * ------------------------------------------------------------------------ */
Int16 EZDSP5502_I2CGPIO_configLine(  Uint16 line, Uint16 dir );
Int16 EZDSP5502_I2CGPIO_readLine(  Uint16 line );
Int16 EZDSP5502_I2CGPIO_writeLine(  Uint16 line, Uint16 val );
Int16 EZDSP5502_I2CGPIO_read(  Uint16 num, Uint16* val );
Int16 EZDSP5502_I2CGPIO_write( Uint16 num, Uint16 val );
