/*
 * w_table.c
 *
 *  Created on: May 31, 2012
 *      Author: BLEE
 *
 *  Description: This program generates twiddle factor lookup table
 *
 *  For the book "Real Time Digital Signal Processing:
 *                Fundamentals, Implementation and Application, 3rd Ed"
 *                By Sen M. Kuo, Bob H. Lee, and Wenshun Tian
 *                Publisher: John Wiley and Sons, Ltd
 */

#include <math.h>
#include "tistdtypes.h"
#include "icomplex.h"

void w_table(complex *U, Uint16 EXP);

#define PI 3.1415926535897

#pragma CODE_SECTION(w_table,     ".text:fft_code"); 

void w_table(complex *U, Uint16 EXP)
{
    complex W;         /* Twiddle factor W^k                   */
    Uint16 i,L;
    Uint16 SL;         /* Number of points in sub DFT at stage L
                          and offset to next DFT in stage      */
    Uint16 SL1;        /* Number of butterflies in one DFT at
                          stage L.  Also is offset to lower point
                          in butterfly at stage L              */

    for (i=0, L=1; L<=EXP; L++)
    {
        SL=1<<L;               /* LE=2^L=points of sub DFT */
        SL1=SL>>1;             /* Number of twiddle factors in sub-DFT */
        W.re =  (Int16)(32768.0*cos(PI/SL1));
        W.im = -(Int16)(32768.0*sin(PI/SL1));
        U[i++] = W;
    }
}
