/*
 * float_mag128.c
 *
 *  Created on: May 29, 2012
 *      Author: BLEE
 *
 *  Description: This program computes magnitude
 *
 *  For the book "Real Time Digital Signal Processing:
 *                Fundamentals, Implementation and Application, 3rd Ed"
 *                By Sen M. Kuo, Bob H. Lee, and Wenshun Tian
 *                Publisher: John Wiley and Sons, Ltd
 */

#include "tistdtypes.h"
#include "float_dft128.h"

void float_mag_128(Int16 x[], Int16 spectrum[])
{
  Int16 j,k;
  float temp1,temp2;
  
  for (j=0, k=0;k<N/2;k++)
  {
      temp1 = (float)x[j++]/32767.0;
      temp1 *= temp1;
      temp2 = (float)x[j++]/32767.0;
      temp2 *= temp2;
      temp2 += temp1;
      spectrum[k] = (Int16)(temp2*32767.0 + 0.5);
  }
}

