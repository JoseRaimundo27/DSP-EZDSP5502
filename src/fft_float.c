/*
 * float_fft.c
 *
 *  Created on: May 29, 2012
 *      Author: BLEE
 *
 *  Description: Floating-point complex radix-2 decimation-in-time FFT
 *               Perform in place FFT the output overwrite the input array
 *
 *  For the book "Real Time Digital Signal Processing:
 *                Fundamentals, Implementation and Application, 3rd Ed"
 *                By Sen M. Kuo, Bob H. Lee, and Wenshun Tian
 *                Publisher: John Wiley and Sons, Ltd
 */

#include "tistdtypes.h"
#include "fcomplex.h"       /* Floating-point complex.h header file */

void fft(complex *, Uint16, complex *, Uint16, Uint16);

void fft(complex *X, Uint16 EXP, complex *W, Uint16 iFlag, Uint16 sFlag)
{
    complex  temp;  /* Temporary storage of complex variable */
    complex  U;     /* Twiddle factor W^k */
    Uint16 i,j;
    Uint16 id;      /* Index for lower point in butterfly */
    Uint16 L;       /* FFT stage */
    Uint16 LE;      /* Number of points in sub DFT at stage L
                       and offset to next DFT in stage */
    Uint16 LE1;     /* Number of butterflies in one DFT at
                       stage L.  Also is offset to lower point
                       in butterfly at stage L */
    float scale;
    Uint16 N=1<<EXP;/* Number of points for FFT */
    
    if (sFlag == 1) /* NOSCALE_FLAG=1 */
        scale = 1.0;
    else            /* SCALE_FLAG = 0 */
        scale = 0.5;/* Scaling of 0.5 at each stage */
     
    if (iFlag == 1) /* FFT_FLAG=0, IFFT_FLAG=1 */
        scale = 1.0;/* Without scaling for IFFT */

    
    for (L=1; L<=EXP; L++) 	/* FFT of length 2^EXP */
    {
        LE=1<<L;        	/* LE=2^L=points of sub DFT */
        LE1=LE>>1;      	/* Number of butterflies in sub-DFT */
        U.re = 1.0;
        U.im = 0.0;

        for (j=0; j<LE1;j++)
        {
            for(i=j; i<N; i+=LE) /* Butterfly computations */
            {
                id=i+LE1;
                temp.re = (X[id].re*U.re - X[id].im*U.im)*scale;
                temp.im = (X[id].im*U.re + X[id].re*U.im)*scale;

                X[id].re = X[i].re*scale - temp.re;
                X[id].im = X[i].im*scale - temp.im;

                X[i].re = X[i].re*scale + temp.re;
                X[i].im = X[i].im*scale + temp.im;
            }
            
            /* Recursive compute W^k as U*W^(k-1) */
            temp.re = U.re*W[L-1].re - U.im*W[L-1].im;
            U.im = U.re*W[L-1].im + U.im*W[L-1].re;
            U.re = temp.re;
        }
    }
}
