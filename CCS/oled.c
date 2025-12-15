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

// Imprime "REV"
void printWord_REV_Inverted() {
    printLetter(0x00, 0x00, 0x00, 0x00); // Espaço (Último a entrar -> Fica na direita)
    printLetter(0x20, 0x40, 0x20, 0x1F); // V
    printLetter(0x41, 0x49, 0x49, 0x7F); // E
    printLetter(0x66, 0x19, 0x09, 0x7F); // R (Primeiro a entrar -> Fica na esquerda)
}

// Imprime "HAW" (Para aparecer "WAH")
void printWord_WAH_Inverted() {
    printLetter(0x7F, 0x08, 0x08, 0x7F); // H
    printLetter(0x7E, 0x11, 0x11, 0x7E); // A
    printLetter(0x40, 0x30, 0x40, 0x3F); // W
}

// Símbolo "b" (Bemol) - Mantido igual, só será chamado na ordem certa
void printSymbol_Flat() {
    printLetter(0x30, 0x48, 0x48, 0x7F); // b
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
        // ==========================================
        // ESTADO 0: LOOPBACK -> Escreve "POOL" (Para ler LOOP)
        // ==========================================
        case 0:
            printLetter(0x06, 0x09, 0x09, 0x7F); // P
            printLetter(0x3E, 0x41, 0x41, 0x3E); // O
            printLetter(0x3E, 0x41, 0x41, 0x3E); // O
            printLetter(0x40, 0x40, 0x40, 0x7F); // L
            break;

        // ==========================================
        // ESTADO 1: REVERB HALL -> Escreve "LLAH VER" (Para ler REV HALL)
        // ==========================================
        case 1:
            printLetter(0x40, 0x40, 0x40, 0x7F); // L
            printLetter(0x40, 0x40, 0x40, 0x7F); // L
            printLetter(0x7E, 0x11, 0x11, 0x7E); // A
            printLetter(0x7F, 0x08, 0x08, 0x7F); // H
            printWord_REV_Inverted();            // Chama a função invertida (V E R)
            break;

        // ==========================================
        // ESTADO 2: TOM A -> Escreve " A VER" (Para ler REV A)
        // ==========================================
        case 2:
            printLetter(0x00, 0x00, 0x00, 0x00); // Espaço
            printLetter(0x00, 0x00, 0x00, 0x00); // Espaço
            printLetter(0x7E, 0x11, 0x11, 0x7E); // A
            printLetter(0x00, 0x00, 0x00, 0x00); // Espaço
            printWord_REV_Inverted();
            break;

        // ==========================================
        // ESTADO 3: TOM Bb -> Escreve " bB VER" (Para ler REV Bb)
        // ==========================================
        case 3:
            printLetter(0x00, 0x00, 0x00, 0x00); // Espaço
            printSymbol_Flat();                  // b
            printLetter(0x36, 0x49, 0x49, 0x7F); // B
            printLetter(0x00, 0x00, 0x00, 0x00); // Espaço
            printWord_REV_Inverted();
            break;

        // ==========================================
        // ESTADO 4: TOM Db -> Escreve " bD VER" (Para ler REV Db)
        // ==========================================
        case 4:
            printLetter(0x00, 0x00, 0x00, 0x00); // Espaço
            printSymbol_Flat();                  // b
            printLetter(0x3E, 0x41, 0x41, 0x7F); // D
            printLetter(0x00, 0x00, 0x00, 0x00); // Espaço
            printWord_REV_Inverted();
            break;

        // ==========================================
        // ESTADO 5: TOM Fb -> Escreve " bF VER" (Para ler REV Fb)
        // ==========================================
        case 5:
            printLetter(0x00, 0x00, 0x00, 0x00); // Espaço
            printSymbol_Flat();                  // b
            printLetter(0x01, 0x09, 0x09, 0x7F); // F
            printLetter(0x00, 0x00, 0x00, 0x00); // Espaço
            printWord_REV_Inverted();
            break;

        // ==========================================
        // ESTADO 6: GHOST -> Escreve "TSOHG" (Para ler GHOST)
        // ==========================================
        case 6:
            printLetter(0x00, 0x01, 0x7F, 0x01); // T
            printLetter(0x31, 0x49, 0x49, 0x46); // S
            printLetter(0x3E, 0x41, 0x41, 0x3E); // O
            printLetter(0x7F, 0x08, 0x08, 0x7F); // H
            printLetter(0x2E, 0x49, 0x41, 0x3E); // G
            break;

        // ==========================================
        // ESTADO 7: PHASER -> Escreve "RSAHP" (Para ler PHASR)
        // ==========================================
        case 7:
            printLetter(0x66, 0x19, 0x09, 0x7F); // R
            printLetter(0x31, 0x49, 0x49, 0x46); // S
            printLetter(0x7E, 0x11, 0x11, 0x7E); // A
            printLetter(0x7F, 0x08, 0x08, 0x7F); // H
            printLetter(0x06, 0x09, 0x09, 0x7F); // P
            break;

        // ==========================================
        // ESTADO 8: AUTO-WAH -> Escreve "HAW-A" (Para ler A-WAH)
        // ==========================================
        case 8:
            printWord_WAH_Inverted();            // WAH (H-A-W)
            printLetter(0x08, 0x08, 0x08, 0x08); // -
            printLetter(0x7E, 0x11, 0x11, 0x7E); // A
            break;

        default:
            // Escreve "RRE" (Para ler ERR)
            printLetter(0x66, 0x19, 0x09, 0x7F); // R
            printLetter(0x66, 0x19, 0x09, 0x7F); // R
            printLetter(0x41, 0x49, 0x49, 0x7F); // E
            break;
    }
}
