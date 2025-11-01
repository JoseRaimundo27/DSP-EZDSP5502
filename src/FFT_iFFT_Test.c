/*
 * FFT_iFFT_Test.c
 *
 *  Created on: May 31, 2012
 *      Author: BLEE
 *
 *      Description: This is the test program for FFT-iFFT experiment
 *
 *  For the book "Real Time Digital Signal Processing:
 *                Fundamentals, Implementation and Application, 3rd Ed"
 *                By Sen M. Kuo, Bob H. Lee, and Wenshun Tian
 *                Publisher: John Wiley and Sons, Ltd
 */

#include <stdio.h>
#include "tistdtypes.h"
#include "icomplex.h"    /* Integer complex.h header file */
#include "intrinsic_fft.h"

#include "input.dat"     /* Integer FFT test data file, sweep frequency */

#pragma DATA_SECTION(U,        ".bss:fft_coef");
#pragma DATA_SECTION(X,        ".bss:fft_in");  
#pragma DATA_SECTION(output,   ".bss:fft_output"); 
#pragma DATA_SECTION(error,    ".bss:fft_error"); 
#pragma CODE_SECTION(main,     ".text:fft_code"); 

complex X[N];           /* Declare input array  */
complex U[EXP];         /* Twiddle e^(-j2pi/N) table */

Int16 output[13*N];     /* FFT - IFFT final output */
Int16 error[13*N];      /* IFFT output error from input */

#define FFT_FLAG        0        /* Perform FFT */
#define IFFT_FLAG       1        /* Perform IFFT */
#define SCALE_FLAG      0        /* Scale FFT butterfly output */
#define NOSCALE_FLAG    1        /* Not to scale FFT butterfly output */


void main()
{
    Uint16 i,j,k;
    Int16 *inDataPtr,*outDataPtr,*errPtr;
    FILE *fp;

    printf("Exp --- started\n");
    /* Create Twiddle lookup table for FFT */
    w_table(U, EXP);      
  
    /* Start FFT test */ 
    inDataPtr = input;
    outDataPtr = output;
    errPtr = error;
    for (k=1, j=0; j<(13*N); j+=N)    /* Data file has 1664 = 13*128 data */
    {
        for (i=0; i<N; i++)  
        {
            X[i].re = *inDataPtr++;    /* Generate input samples */
            X[i].im = 0;
    	}
        
        /* Start FFT */
        bit_rev(X,EXP);     /* Arrange X[] in bit reversal order */   
        fft(X,EXP,U,FFT_FLAG,SCALE_FLAG); /* Perform FFT with scale */
                            
        /* Inverse FFT */
        for (i=0; i<N; i++) /* Change the sign of imaginary part */
        {
            X[i].im = -X[i].im;
        }                      
        
        bit_rev(X,EXP);     /* Arrange sample in bit reversal order */
        fft(X,EXP,U,IFFT_FLAG,NOSCALE_FLAG); /* Perform IFFT without scale */
                            
        /* Store IFFT output samples for verification */
        for (i=0; i<N; i++) 
        {
            *outDataPtr++ = X[i].re;  
        } 
        printf("FFT frame #%i\n", k++);
    }

    // Check for errors, we can display output[] and error[] to verify the result
    inDataPtr = input;
    outDataPtr = output;
    errPtr = error;
    fp = fopen("..\\data\\FFT_IFFT.xls", "wt");
    fprintf(fp, "n\t input\t output\t error\n");
    for (j=0; j<(13*N); j++)    
    {
    	fprintf(fp, "%d\t%d\t%d\t%d\n", j, *inDataPtr, *outDataPtr, *inDataPtr-*outDataPtr);
        *errPtr++ = *inDataPtr++ - *outDataPtr++;
    }
    fclose(fp);
    printf("Exp --- completed\n");
}

