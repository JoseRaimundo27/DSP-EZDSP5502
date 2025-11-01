#include <stdio.h>
#include "tistdtypes.h"
#include "float_dft128.h"
#include "input.dat"

Int16 xin[2*N];
Int16 xout[2*N];
Int16 spectrum[N/2];              /* Using graphic tool to plot spectrum for length N/2 data */

void main()
{
    Int16 i,j;
    FILE *fp;
    fp = fopen("..\\data\\spectrum.xls","wt");

    printf("Exp --- started\n");

    for (j=0,i=0;i<N;i++){
        xin[j++] = input[i];          /* Get real sample */
        xin[j++] = 0;                 /* Imaginary sample=0 */
    }
    float_dft_128(xin, xout);         /* Call DFT routine */
    float_mag_128(xout, spectrum);    /* Compute spectrum */
    fprintf(fp, "Bin #\tDFT spectrum\n");

    for (i=0; i<N/2; i++){
        fprintf(fp, "%d\t%d\n", i, spectrum[i]);
    }

    fclose(fp);
    printf("Exp --- completed\n");

}