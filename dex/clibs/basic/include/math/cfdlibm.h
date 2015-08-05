/*Copyright 2011 Alibaba Group*/
/*
 *  cmath.h
 *  madagascar
 *
 *  Created by Alba.Xu on 4/16/09.
 *  Copyright 2009 vmkid. All rights reserved.
 *
 */

#ifndef CLIBS_MATH_H
#define CLIBS_MATH_H

#include "std/cstd.h"

extern double __ieee754_sqrt (double);
extern double __ieee754_sin (double);
extern double __ieee754_cos (double);
extern double __ieee754_tan (double);
extern double __ieee754_tanh (double);
extern double __ieee754_acos (double);
extern double __ieee754_acosh (double);
extern double __ieee754_log  (double);
extern double __ieee754_atanh  (double);
extern double __ieee754_asin  (double);
extern double __ieee754_atan(double x);
extern double __ieee754_atan2  (double,double);
extern double __ieee754_cbrt(double);
extern double __ieee754_ceil(double);
extern double __ieee754_exp  (double);
extern double __ieee754_expm1  (double);
extern double __ieee754_cosh  (double);
extern double __ieee754_floor  (double);
extern double __ieee754_fmod  (double,double);
extern double __ieee754_nextafter  (double,double);
extern float __ieee754_nextafterf  (float,float);
extern double __ieee754_pow  (double,double);
extern double __ieee754_lgamma_r  (double,int *);
extern double __ieee754_gamma_r  (double,int *);
extern double __ieee754_lgamma  (double);
extern double __ieee754_gamma  (double);
extern double __ieee754_log10  (double);
extern double __ieee754_log1p  (double);
extern double __ieee754_rint  (double);
extern double __ieee754_sinh  (double);
extern double __ieee754_hypot  (double,double);
extern double __ieee754_j0  (double);
extern double __ieee754_j1  (double);
extern double __ieee754_y0  (double);
extern double __ieee754_y1  (double);
extern double __ieee754_jn  (int,double);
extern double __ieee754_yn  (int,double);
extern double __ieee754_remainder  (double,double);
extern int    __ieee754_rem_pio2  (double,double*);
#ifdef _SCALB_INT
extern double __ieee754_scalb  (double,int);
#else
extern double __ieee754_scalb  (double,double);
#endif

/* fdlibm kernel function */
extern double __kernel_standard  (double,double,int);
extern double __kernel_sin  (double,double,int);
extern double __kernel_cos  (double,double);
extern double __kernel_tan  (double,double,int);
extern int    __kernel_rem_pio2  (double*,double*,int,int,int,const int*);

#endif

