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
extern void oled_updateEffectName(Uint8 state);

extern Uint16 timerFlag;
Uint8 ledNum = 3;

// =================== VARIÁVEL GLOBAL ===================

volatile Uint8 currentState = 0;
// =======================================================

// (Flags para debounce dos botões)
static Uint8 sw1Ready = 1;
static Uint8 sw2Ready = 1;

void main(void)
{
    /* Demo Initialization */
    //initPLL( );
    EZDSP5502_init( );
    //initLed( );
    configPort( );
    //initTimer0( );
    initAIC3204( );
    initAlgorithms(); // initLFO e Reverb
    
    configAudioDma( );
    
    /* Start Demo */
    startAudioDma ( );
    EZDSP5502_MCBSP_init( );

    oled_start( );
    oled_updateEffectName(currentState);
    
    while(1)
    {
        checkSwitch( ); // Verifica os botões
    }
}

/*
 * configPort( )
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
 * toggleLED( )
 */
static toggleLED(void)
{
    if(CHIP_FGET(ST1_55, XF))
        CHIP_FSET(ST1_55, XF,CHIP_ST1_55_XF_OFF);  // Turn off LED
    else
        CHIP_FSET(ST1_55, XF, CHIP_ST1_55_XF_ON);  // Turn on LED
}


/*
 * Verifica se SW1 ou SW2 foram premidos e altera o currentState.
 */
void checkSwitch(void)
{
    /* Check SW1 (Mudar Efeito: PARA A ESQUERDA) */
    if(!(EZDSP5502_I2CGPIO_readLine(SW0))) // SW1 está premido?
    {
        if(sw1Ready) //
        {
            sw1Ready = 0; // Marcar o botão como "usado"
            toggleLED();

            if(currentState != 0)
            {
                currentState--;
            }
            else
            {
                currentState = 2; // (Faz loop 0 -> 2)
            }
            oled_updateEffectName(currentState);
        }
    }
    else // SW1 NÃO está premido
    {
        sw1Ready = 1;
    }

    /* Check SW2 (Mudar Efeito: PARA A DIREITA) */
    if(!(EZDSP5502_I2CGPIO_readLine(SW1))) // SW2 está premido?
    {
        if(sw2Ready)
        {
            toggleLED();
            sw2Ready = 0; // Marcar como "usado"

            if (currentState != 2)
            {
                currentState++;
            }
            else
            {
                currentState = 0; // (Faz loop 2 -> 0)
            }
            oled_updateEffectName(currentState);
        }
    }
    else // SW2 NÃO está premido
    {
        sw2Ready = 1; // "Rearma" o botão
    }
}
