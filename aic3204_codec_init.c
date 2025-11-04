
#include "aic3204_codec_init.h"
#include "ezdsp5502.h"
#include "ezdsp5502_i2c.h"
#include "ezdsp5502_i2cgpio.h"
#include "stdio.h"

// Endereço I2C do AIC3204 (copiado de aic3204_test.c)
#define AIC3204_I2C_ADDR 0x18

Int16 AIC3204_rget( Uint16 regnum, Uint16* regval )
{
    Int16  retcode = 0;
    Uint16 cmd[2];

    cmd[0] = regnum & 0x007F;     // 7-bit Device Register
    cmd[1] = 0;

    retcode |= EZDSP5502_I2C_write( AIC3204_I2C_ADDR, cmd, 1 ); // Envia o nome do registrador
    retcode |= EZDSP5502_I2C_read( AIC3204_I2C_ADDR, cmd, 1 );  // Lê o valor
    *regval = cmd[0];
    EZDSP5502_waitusec( 50 );

    return retcode;
}

/*
 * AIC3204_rset( regnum, regval )
 * Escreve um valor em um registrador do codec
 */
Int16 AIC3204_rset( Uint16 regnum, Uint16 regval )
{
    Uint16 cmd[2];
    cmd[0] = regnum & 0x007F;     // 7-bit Device Register
    cmd[1] = regval;             // 8-bit Register Data

    EZDSP5502_waitusec( 100 );

    return EZDSP5502_I2C_write( AIC3204_I2C_ADDR, cmd, 2 );
}


Int16 AIC3204_codec_init( )
{
    printf("Iniciando configuração do Codec AIC3204...\n");

    /*
     * PASSO 1: Configurar I2C GPIO para ligar o barramento McBSP
     * (Copiado de aic3204_test.c)
     */
    printf("  Configurando pinos I2C GPIO...\n");
    EZDSP5502_I2CGPIO_configLine( BSP_SEL1, OUT );
    EZDSP5502_I2CGPIO_writeLine(  BSP_SEL1, LOW );

    EZDSP5502_I2CGPIO_configLine( BSP_SEL1_ENn, OUT );
    EZDSP5502_I2CGPIO_writeLine(  BSP_SEL1_ENn, LOW );


    /*
     * PASSO 2: Sequência de Inicialização do Codec
     * (Copiado de aic3204_loop_linein.c)
     */
    printf("  Configurando registradores do Codec via I2C...\n");
    
    AIC3204_rset( 0, 0 );     // Select page 0
    AIC3204_rset( 1, 1 );     // Reset codec
    AIC3204_rset( 0, 1 );     // Select page 1
    AIC3204_rset( 1, 8 );     // Disable crude AVDD generation from DVDD
    AIC3204_rset( 2, 1 );     // Enable Analog Blocks, use LDO power
    AIC3204_rset( 0, 0 );
    
    /* PLL and Clocks config and Power Up */
    AIC3204_rset( 27, 0x0d );  // BCLK and WCLK are set as o/p; AIC3204(Master)
                              // *** NOTA: O CSL VAI MUDAR ISSO PARA SLAVE ***
    AIC3204_rset( 28, 0x00 );  // Data ofset = 0
    AIC3204_rset( 4, 3 );     // PLL setting: PLLCLK <- MCLK, CODEC_CLKIN <-PLL CLK
    AIC3204_rset( 6, 7 );     // PLL setting: J=7
    AIC3204_rset( 7, 0x06 );  // PLL setting: HI_BYTE(D=1680)
    AIC3204_rset( 8, 0x90 );  // PLL setting: LO_BYTE(D=1680)
    AIC3204_rset( 30, 0x9C );  // For 32 bit clocks per frame in Master mode ONLY
                              // BCLK=DAC_CLK/N =(12288000/8) = 1.536MHz = 32*fs
    AIC3204_rset( 5, 0x91 );  // PLL setting: Power up PLL, P=1 and R=1
    AIC3204_rset( 13, 0 );    // Hi_Byte(DOSR) for DOSR = 128 decimal or 0x0080
    AIC3204_rset( 14, 0x80 ); // Lo_Byte(DOSR) for DOSR = 128 decimal or 0x0080
    AIC3204_rset( 20, 0x80 ); // AOSR for AOSR = 128 decimal or 0x0080
    AIC3204_rset( 11, 0x82 ); // Power up NDAC and set NDAC value to 2
    AIC3204_rset( 12, 0x87 ); // Power up MDAC and set MDAC value to 7
    AIC3204_rset( 18, 0x87 ); // Power up NADC and set NADC value to 7
    AIC3204_rset( 19, 0x82 ); // Power up MADC and set MADC value to 2

    /* DAC ROUTING and Power Up */
/* DAC ROUTING and Power Up */
    AIC3204_rset(0, 1);
    AIC3204_rset(0x0c, 8);
    AIC3204_rset(0x0d, 8);
    AIC3204_rset(0, 0);
    AIC3204_rset(64, 2);
    AIC3204_rset(65, 0);
    AIC3204_rset(63, 0xd4);
    AIC3204_rset(0, 1);
    AIC3204_rset(9, 0x30);
    AIC3204_rset(0x10, 0x00);
    AIC3204_rset(0x11, 0x00);
    AIC3204_rset(0, 0);
    EZDSP5502_waitusec(1000000); // 1 segundo

    /* ADC ROUTING and Power Up */
    AIC3204_rset(0, 1);
    AIC3204_rset(0x34, 0x30);
    AIC3204_rset(0x37, 0x30);
    AIC3204_rset(0x36, 3);
    AIC3204_rset(0x39, 0xc0);
    AIC3204_rset(0x3b, 0);
    AIC3204_rset(0x3c, 0);
    AIC3204_rset(51, 0x60);
    AIC3204_rset(59, 0x28);
    AIC3204_rset(60, 0x5f);
    AIC3204_rset(0, 0);
    AIC3204_rset(0x51, 0xc0);
    AIC3204_rset(0x52, 0);
    AIC3204_rset(0, 0);
    EZDSP5502_waitusec(200);

    printf("  ...Configuração do Codec Concluída.\n");

    return 0;
}

void initAic3204(){
    /* Initialize McBSP */
    EZDSP5502_MCBSP_init( );
}
void disableAic3204(){
    EZDSP5502_MCBSP_close();// Disable McBSP
}

