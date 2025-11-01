/*
 * float_dft128.c
 *
 *  Created on: May 29, 2012
 *      Author: BLEE
 *
 *  Description: This is the DFT program in floating-point C
 *
 *  For the book "Real Time Digital Signal Processing:
 *                Fundamentals, Implementation and Application, 3rd Ed"
 *                By Sen M. Kuo, Bob H. Lee, and Wenshun Tian
 *                Publisher: John Wiley and Sons, Ltd
 */

#include <math.h>
#include "tistdtypes.h"
#include "float_dft128.h"

#define PI 3.1415926536

void float_dft_128(Int16 Xin[], Int16 Xout[])
{
  Int16 i,n,k,j;
  float angle;
  float Xr[N],Xi[N];
  float W[2];

  for (i=0, k=0;k<N;k++)
  {
      Xr[k]=0;
      Xi[k]=0;
      for(j=0,n=0;n<N;n++)
      {
        angle =(2.0*PI*k*n)/N; // Calculate twiddle factor angle
        W[0]=cos(angle);       // Compute twiddle complex factor
        W[1]=sin(angle);
        // Multiple complex data by complex twiddle factor
        // The input integer data is convered to floating-point by /32676.0
        Xr[k]=Xr[k]+(float)Xin[j]*W[0]/32767.0+(float)Xin[j+1]/32767.0*W[1];
        Xi[k]=Xi[k]+(float)Xin[j+1]*W[0]/32767.0-(float)Xin[j]/32767.0*W[1];
        j+=2;                 // Advance data pointer
      }
      // Save the real and the imaginary part of the DFT result with rounding
      // Floating-point data is converted to integer by *32767.0
      Xout[i++] = (Int16)(Xr[k]*32767.0 + 0.5); 
      Xout[i++] = (Int16)(Xi[k]*32767.0 + 0.5);
   }
}

