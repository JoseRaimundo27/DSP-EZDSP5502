/*
 * ibit_rev.c
 *
 *  Created on: May 31, 2012
 *      Author: BLEE
 *
 *  Description: bit_reverse for FFT implementation
 *  Perform in place FFT the output overwrite the input array
 *
 *  For the book "Real Time Digital Signal Processing:
 *                Fundamentals, Implementation and Application, 3rd Ed"
 *                By Sen M. Kuo, Bob H. Lee, and Wenshun Tian
 *                Publisher: John Wiley and Sons, Ltd
 */

#include "tistdtypes.h"
#include "icomplex.h"		/* Integer complex.h header file */

void bit_rev(complex *X, Uint16 EXP)
{
	Uint16 i,j,k;
	Uint16 N=1<<EXP;		/* Number of points for FFT */
	Uint16 N2=N>>1;
    complex temp;			/* Temporary storage of int complex variable */
	
    for (j=0,i=1;i<N-1;i++)
    {
        k=N2;
        while(k<=j)
        {
            j-=k;
            k>>=1;
        }
        j+=k;

        if (i<j)
        {
            temp = X[j];
            X[j] = X[i];
            X[i] = temp;
        }
    }  
}
