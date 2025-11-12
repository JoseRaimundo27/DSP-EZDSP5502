//////////////////////////////////////////////////////////////////////////////
// * File name: lcd.c
// *
// * Description:  LCD functions.
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

#include "ezdsp5502_i2c.h"
#include "ezdsp5502_gpio.h"
#include "csl_gpio.h"
#include "lcd.h"

#define OSD9616_I2C_ADDR 0x3C    // OSD9616 I2C address

/*
 *
 *  Int16 OSD9616_init( )
 *
 *      Initialize LCD display
 *
 */
Int16 osd9616_init( )
{
    Uint16 cmd[10];    // For multibyte commands

    /* Initialize LCD power */
    EZDSP5502_GPIO_init( GPIO_GPIO_PIN1 );         // Enable GPIO pin
    EZDSP5502_GPIO_setDirection( GPIO_GPIO_PIN1, GPIO_GPIO_PIN1_OUTPUT );  // Output
    EZDSP5502_GPIO_setOutput( GPIO_GPIO_PIN1, 1 ); // Enable 13V

    /* Initialize osd9616 display */
    osd9616_send(0x00,0x00); // Set low column address
    osd9616_send(0x00,0x10); // Set high column address
    osd9616_send(0x00,0x40); // Set start line address

    /* Set contrast control register */
    cmd[0] = 0x00 & 0x00FF;
    cmd[1] = 0x81;
    cmd[2] = 0x7f;
    osd9616_multiSend( cmd, 3 );

    osd9616_send(0x00,0xa1); // Set segment re-map 95 to 0
    osd9616_send(0x00,0xa6); // Set normal display

    /* Set multiplex ratio(1 to 16) */
    cmd[0] = 0x00 & 0x00FF;
    cmd[1] = 0xa8;
    cmd[2] = 0x0f;
    osd9616_multiSend( cmd, 3 );

    osd9616_send(0x00,0xd3); // Set display offset
    osd9616_send(0x00,0x00); // Not offset
    osd9616_send(0x00,0xd5); // Set display clock divide ratio/oscillator frequency
    osd9616_send(0x00,0xf0); // Set divide ratio

    /* Set pre-charge period */
    cmd[0] = 0x00 & 0x00FF;
    cmd[1] = 0xd9;
    cmd[2] = 0x22;
    osd9616_multiSend( cmd, 3 );

    /* Set com pins hardware configuration */
    cmd[0] = 0x00 & 0x00FF;
    cmd[1] = 0xda;
    cmd[2] = 0x02;
    osd9616_multiSend( cmd, 3 );

    osd9616_send(0x00,0xdb); // Set vcomh
    osd9616_send(0x00,0x49); // 0.83*vref

    /* set DC-DC enable */
    cmd[0] = 0x00 & 0x00FF;
    cmd[1] = 0x8d;
    cmd[2] = 0x14;
    osd9616_multiSend( cmd, 3 );

    osd9616_send(0x00,0xaf); // Turn on oled panel

    return 0;
}

/*
 *
 *  Int16 osd9616_send( Uint16 comdat, Uint16 data )
 *
 *      Sends 2 bytes of data to the osd9616
 *
 */
Int16 osd9616_send( Uint16 comdat, Uint16 data )
{
    Uint16 cmd[2];
    cmd[0] = comdat & 0x00FF;     // Specifies whether data is Command or Data
    cmd[1] = data;                // Command / Data

    /* Write to OSD9616 */
    return EZDSP5502_I2C_write( OSD9616_I2C_ADDR, cmd, 2 );
}

/*
 *
 *  Int16 osd9616_multiSend( Uint16 comdat, Uint16 data )
 *
 *      Sends multiple bytes of data to the osd9616
 *
 */
Int16 osd9616_multiSend( Uint16* data, Uint16 len )
{
    Uint16 x;
    Uint16 cmd[10];
    for(x=0;x<len;x++)               // Command / Data
    {
        cmd[x] = data[x];
    }

    /* Write len bytes to OSD9616 */
    return EZDSP5502_I2C_write( OSD9616_I2C_ADDR, cmd, len );
}

/*
 *
 *  Int16 printLetter(Uint16 l1,Uint16 l2,Uint16 l3,Uint16 l4)
 *
 *      Send 4 bytes representing a Character
 *
 */
Int16 printLetter(Uint16 c4,Uint16 c3,Uint16 c2,Uint16 c1)
{
    osd9616_send(0x40, c4);    // Column 4
    osd9616_send(0x40, c3);    // Column 3
    osd9616_send(0x40, c2);    // Column 2
    osd9616_send(0x40, c1);    // Column 1
    osd9616_send(0x40, 0x00);

    return 0;
}
