/*
 * intrinsic_fft.c
 *
 *  Created on: May 31, 2012
 *      Author: BLEE
 *
 *  Description: Complex radix-2 decimation-in-time intrinsics FFT implementation
 *  Perform in place FFT the output overwrite the input array
 *  Using C55x C intrinsics to perform in place FFT, output overwrite input array
 *
 *  For the book "Real Time Digital Signal Processing:
 *                Fundamentals, Implementation and Application, 3rd Ed"
 *                By Sen M. Kuo, Bob H. Lee, and Wenshun Tian
 *                Publisher: John Wiley and Sons, Ltd
 */

#include "tistdtypes.h"
#include "icomplex.h"
#include <intrindefs.h> 

#define  SFT16 16

void fft(complex *, Uint16, complex *, Uint16, Uint16);

void fft(complex *X, Uint16 EXP, complex *W, Uint16 iFlag, Uint16 sFlag)
{
    complex temp;           /* Temporary storage of short complex variable */
    lcomplex ltemp;         /* Temporary storage of long complex variable */
    complex U;              /* Twiddle factor W^k */
    Uint16 i,j;
    Uint16 id;				/* Index for lower point in butterfly */
    Uint16 L;				/* FFT stage */
    Uint16 LE;				/* Number of points in sub DFT at stage L
                              and offset to next DFT in stage */
    Uint16 LE1;				/* Number of butterflies in one DFT at
                              stage L.  Also is offset to lower point
                              in butterfly at stage L */
    Int16 scale;
    Uint16 N=1<<EXP; 		/* Number of points for FFT */
    
    if (sFlag == 1)			/* SCALE_FLAG = 0, NOSCALE_FLAG=1 */
        scale = 0;
    else
        scale = 1;
     
    if (iFlag == 1)			/* FFT_FLAG=0, IFFT_FLAG=1 */
        scale = 0;

    for (L=1; L<=EXP; L++)	/* FFT butterfly */
    {
        LE=1<<L;			/* LE=2^L=points of sub DFT */
        LE1=LE>>1;			/* Number of butterflies in sub DFT */
        U.re = 0x7fff;
        U.im = 0;

        for (j=0; j<LE1;j++)
        {
            for(i=j; i<N; i+=LE) /* Do the butterflies */
            {
                id=i+LE1;                 
                ltemp.re = _lsmpy(X[id].re, U.re);
                ltemp.re = _smas(ltemp.re, X[id].im, U.im);
                temp.re = (Int16)(ltemp.re>>SFT16);
                temp.re >>= scale;
                ltemp.im = _lsmpy(X[id].im, U.re);
                ltemp.im = _smac(ltemp.im, X[id].re, U.im);
                temp.im = (Int16)(ltemp.im>>SFT16);
                temp.im >>= scale;
                X[id].re = _ssub(X[i].re>>scale, temp.re);
                X[id].im = _ssub(X[i].im>>scale, temp.im);
                X[i].re = _sadd(X[i].re>>scale, temp.re);
                X[i].im = _sadd(X[i].im>>scale, temp.im);
            }
            
            /* Recursive compute W^k as W*W^(k-1) */
            ltemp.re = _lsmpy(U.re, W[L-1].re);
            ltemp.re = _smas(ltemp.re, U.im, W[L-1].im);  
            ltemp.im = _lsmpy(U.re, W[L-1].im);
            ltemp.im = _smac(ltemp.im, U.im, W[L-1].re);
            U.re = ltemp.re>>SFT16;            
            U.im = ltemp.im>>SFT16;
        }
    }
}  
