/*
 * icomplex.h
 *
 *  Created on: May 31, 2012
 *      Author: BLEE
 *
 *  Description: Header file icomplex.h for fixed-point
 *
 *  For the book "Real Time Digital Signal Processing:
 *                Fundamentals, Implementation and Application, 3rd Ed"
 *                By Sen M. Kuo, Bob H. Lee, and Wenshun Tian
 *                Publisher: John Wiley and Sons, Ltd
 */

struct cmpx      /* Q15 format */
{
	Int16 re;
	Int16 im;
};
typedef struct cmpx complex;

struct lcmpx     /* Q31 format */
{
	Int32 re;
	Int32 im;
};
typedef struct lcmpx lcomplex;

