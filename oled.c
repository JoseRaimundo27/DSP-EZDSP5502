//////////////////////////////////////////////////////////////////////////////
// * File name: oled.c
// *
// * Description:  OSD9616 OLED functions.
// * (MODIFICADO para mostrar APENAS o nome do efeito)
// *
// //////////////////////////////////////////////////////////////////////////////
 
#include"ezdsp5502.h"
#include"lcd.h"

// ================================================================
// =================== NOVAS FUNÇÕES ADICIONADAS ===================
// ================================================================

/*
 * oled_setCursor(int page, int col)
 *
 * Função "Helper" para posicionar o cursor de escrita
 */
static void oled_setCursor(int page, int col)
{
    osd9616_send(0x00, (0xb0 + page));        // Set Page
    osd9616_send(0x00, (col & 0x0F));     // Set Low Column
    osd9616_send(0x00, (0x10 + (col >> 4))); // Set High Column
}

/*
 * oled_printSpaces(int count)
 *
 * Função "Helper" para escrever N espaços em branco
 */
static void oled_printSpaces(int count)
{
    int i, j;
    for(i = 0; i < count; i++)
    {
        // 4 bytes por "letra" de espaço
        for (j=0; j < 4; j++)
        {
            osd9616_send(0x40,0x00);
        }
    }
}

/*
 * oled_clearAll()
 *
 * Função "Helper" para limpar o ecrã inteiro (escreve preto)
 */
static void oled_clearAll(void)
{
    int page, i;

    // O ecrã parece ter 6 páginas (0 a 5)
    for (page = 0; page < 6; page++)
    {
        // Define a página
        osd9616_send(0x00, (0xb0 + page));
        // Define a coluna inicial (0)
        osd9616_send(0x00, 0x00); // Low
        osd9616_send(0x00, 0x10); // High

        // Escreve 128 bytes de "preto" para limpar a linha
        for(i=0; i < 128; i++)
        {
            osd9616_send(0x40, 0x00);
        }
    }
}


/*
 * Int16 oled_start()
 *
 * (MODIFICADO)
 * Apenas inicializa o ecrã e limpa-o.
 * Remove todo o "splash screen".
 */
Int16 oled_start()
{
    /* 1. Initialize Display */
    osd9616_init( );

    /* 2. Deactivate Scrolling */
    osd9616_send(0x00, 0x2e);

    /* 3. Limpa o ecrã inteiro */
    oled_clearAll();

    return 0;
}


/*
 * oled_updateEffectName(Uint8 state)
 *
 * (MODIFICADO)
 * 1. Limpa o ecrã inteiro.
 * 2. Escreve o nome do efeito na Página 0 (topo).
 */
void oled_updateEffectName(Uint8 state)
{
    /* 1. Limpa o ecrã inteiro antes de escrever */
    oled_clearAll();

    /* 2. Posiciona o cursor no topo (Página 0, Coluna 0) */
    oled_setCursor(0, 0);

    /* 3. Adiciona um pequeno espaço à esquerda (padding) */
    oled_printSpaces(2);

    /* 4. Escreve o nome do efeito */
    switch(state)
    {
        case 0: // FLANGER
            printLetter(0x46,0x29,0x19,0x7F);  // R
            printLetter(0x41,0x49,0x49,0x7F);  // E
            printLetter(0x7F,0x41,0x41,0x22);  // G (Aprox.)
            printLetter(0x7F,0x30,0x0E,0x7F);  // N
            printLetter(0x7C,0x09,0x0A,0x7C);  // A
            printLetter(0x00,0x00,0x7F,0x00);  // L (Aprox.)
            printLetter(0x01,0x09,0x7F,0x01);  // F
            break;

        case 1: // TREMOLO
            printLetter(0x3E,0x41,0x41,0x3E);  // O
            printLetter(0x00,0x00,0x7F,0x00);  // L (Aprox.)
            printLetter(0x3E,0x41,0x41,0x3E);  // O
            printLetter(0x7F,0x06,0x06,0x7F);  // M
            printLetter(0x41,0x49,0x49,0x7F);  // E
            printLetter(0x46,0x29,0x19,0x7F);  // R
            printLetter(0x01,0x7F,0x01,0x01);  // T

            break;

        case 2: // REVERB
            printLetter(0x7F,0x49,0x49,0x36);  // B (Aprox.)
            printLetter(0x46,0x29,0x19,0x7F);  // R
            printLetter(0x41,0x49,0x49,0x7F);  // E
            printLetter(0x07,0x18,0x60,0x18);  // V (Aprox.)
            printLetter(0x41,0x49,0x49,0x7F);  // E
            printLetter(0x46,0x29,0x19,0x7F);  // R

            break;

        default: // Segurança
            printLetter(0x63,0x1C,0x1C,0x63);  // X
            break;
    }
}
