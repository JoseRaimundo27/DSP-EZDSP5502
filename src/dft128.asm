;/*
; * mag_128.asm
; *
; *  Created on: Oct 13, 2012
; *      Author: BLEE
; *
; *  Description: This assembly program computes DFT
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

	.global _dft_128
	.global _sine_cos

N	.set 128

	.bss Xin,2        		; Using long word for address pointer
	.bss Xout,2
	.bss Xr,2
	.bss Xi,2
	.bss W,2      			; W[0]=Wr, W[1]=Wi, must be on even address boundary
	.bss n,1
	.bss k,1

	.sect   ".text"
_dft_128:
    mov  XAR1, dbl(*(#Xout))    ; Save output array address
    mov  XAR0, dbl(*(#Xin))		; Save input array address
    mov  #0, *(#k) 				; k = 0;
    mov  #N-1,BRC0       		; Repeat counter for outter loop N times
    mov  #N-1,BRC1       		; Repeat counter for inner loop N times

    rptb outer_loop-1			; for(i=0, k=0;k<N;k++) {
    mov  #0, AC0
    mov  AC0, dbl(*(#Xr)) 		; Xr = 0;
    mov  AC0, dbl(*(#Xi)) 		; Xi = 0;
    mov  AC0, *(#n)				; n = 0;
    mov  dbl(*(#Xin)), XAR3		; AR3 pinter to input array

    rptb inner_loop-1    		; for(j=0,n=0;n<N;n++) {
    bclr ST1_FRCT
    mov  *(#n), T1
    mpym *(#k), T1, AC0			; k*n
    and  #(N-1), AC0			; m = (k*n)&0x127;
    sftl AC0, #9    			; 2*pi/N*m = 0x7fff/128*m=512*m
    amov #W, XAR0    			; Pointer to W[]

    mov  AC0, T0
	call _sine_cos    			; Compute W[]

    bset ST1_FRCT
    amov #W, XAR0    			; Pointer to W[]

    bset ST1_SATD
	mpym *AR3+, *AR0+, AC0
    macm *AR3-, *AR0, AC0
    mpym *AR3+, *AR0-, AC1
	sfts AC0, #-16

    add  dbl(*(#Xr)), AC0
    masm *AR3+, *AR0, AC1
    sfts AC1, #-16
    mov  AC0, dbl(*(#Xr))		; Xr += Xin[j]*W[0]+Xin[j+1]*W[1];

    add  dbl(*(#Xi)), AC1
    mov  AC1, dbl(*(#Xi))		; Xi += Xin[j+1]*W[0]-Xin[j]*W[1];
    bclr ST1_SATD
    add  #1, *(#n)
inner_loop

    mov  dbl(*(#Xout)), XAR2
    mov  AC0, *AR2+	    		; Xout[j++] = Xr;
    mov  AC1, *AR2+	    		; Xout[j++] = Xi;
    mov  XAR2, dbl(*(#Xout))
    add  #1, *(#k)
outer_loop

    bclr ST1_FRCT
    ret

    .end



