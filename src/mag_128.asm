;/*
; * mag_128.asm
; *
; *  Created on: May 29, 2012
; *      Author: BLEE
; *
; *  Description: This assembly program computes magnitude
; *
; *  For the book "Real Time Digital Signal Processing:
; *                Fundamentals, Implementation and Application, 3rd Ed"
; *                By Sen M. Kuo, Bob H. Lee, and Wenshun Tian
; *                Publisher: John Wiley and Sons, Ltd
; *
; *   Compute the magnitude of the input complex sample
; *
; *   Entry: AR0: input buffer pointer
; *          AR1: output buffer pointer
; *   Return: None
; */

    .def   _mag_128
N   .set   128

    .text
_mag_128
   bset    SATD
   pshboth XAR5
   mov     #(N>>1)-1,BRC0       ; Set BRC0 for loop N/2 times
   mov     XAR0,XAR5
   bset    FRCT
   rptblocal mag_loop-1
   mpym    *AR0+,*AR5+,AC0 ; Xr[i]*Xr[i]
   macm    *AR0+,*AR5+,AC0 ; Xr[i]*Xr[i]+Xi[i]*Xi[i]
   mov     hi(saturate(AC0)),*AR1+
mag_loop
   popboth XAR5
   bclr    SATD
   bclr    FRCT
   ret
   .end

	
