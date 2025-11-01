/*
 * float_fftTest.c
 *
 *  Created on: May 29, 2012
 *      Author: BLEE
 *
 *  Description: This is the 128-point floating-point FFT experiment test program
 *
 *  For the book "Real Time Digital Signal Processing:
 *                Fundamentals, Implementation and Application, 3rd Ed"
 *                By Sen M. Kuo, Bob H. Lee, and Wenshun Tian
 *                Publisher: John Wiley and Sons, Ltd
 */

#include <stdio.h>
#include "tistdtypes.h"
#include <math.h>   
#include "fcomplex.h"	/* Floating-point complex.h header file */
#include "float_fft.h"

#include "input_f.dat"	/* Test data file */
                      
complex X[N];           /* Declare input array  */
complex W[EXP];         /* Twiddle e^(-j2pi/N) table */
complex temp;
Int16   spectrum[N/2];

#define FFT_FLAG        0        /* Perform FFT */
#define IFFT_FLAG       1        /* Perform IFFT */
#define SCALE_FLAG      0        /* Scale FFT butterfly output */
#define NOSCALE_FLAG    1        /* Not to scale FFT butterfly output */


void main(void)
{
    Uint16 i,j,L,LE,LE1,k,n;
    FILE *fp;

    fp = fopen("..\\data\\FFT_spectrum.xls","wt");
    fprintf(fp, "Bin (at each 128-FFT frame)\tFFT spectrum\n");
    n = 0;
    printf("Exp --- started\n");
    for (L=1; L<=EXP; L++)				/* Create twiddle factor table */
    {
        LE=1<<L;						/* LE=2^L=points of sub DFT */
        LE1=LE>>1;						/* Number of butterflies in sub-DFT */
        W[L-1].re = cos(pi/LE1);
        W[L-1].im = -sin(pi/LE1);
    }    
    
    for (k=1, j=0; j<(13*N); j++)		/* Data file has 1664 = 13*128 data */
    {
        for (i=0; i<N; i++) 
        {
            X[i].re = input7_f[j++];	/* Construct input samples */
            X[i].im = 0.0;        
        } 	

        /* Start FFT */
        bit_rev(X,EXP);     			/* Arrange X[] in bit-reverse order */
        fft(X,EXP,W,FFT_FLAG,SCALE_FLAG); /* Perform FFT with scale */

        for (i=0; i<N/2; i++)				/* Verify FFT result */
        {
            temp.re = X[i].re*X[i].re;
            temp.im = X[i].im*X[i].im;        
            spectrum[i] = (Int16)((temp.re+temp.im)*32767);
            fprintf(fp, "%d\t%d\n", n++, spectrum[i]);
        }
        printf("FFT frame #%i\n", k++);
    }

    fclose(fp);
    printf("Exp --- completed\n");
}

