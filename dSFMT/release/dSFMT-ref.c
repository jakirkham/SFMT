/** 
 * @file dSFMT-ref.c 
 * @brief double precision SIMD-oriented Fast Mersenne Twister (dSFMT)
 * based on IEEE 754 format.
 *
 * @author Mutsuo Saito (Hiroshima University)
 * @author Makoto Matsumoto (Hiroshima University)
 *
 * Copyright (C) 2007 Mutsuo Saito, Makoto Matsumoto and Hiroshima
 * University. All rights reserved.
 *
 * The new BSD License is applied to this software, see LICENSE.txt
 */
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include "dSFMT-params.h"

#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L)
  #include <inttypes.h>
#elif defined(_MSC_VER)
  typedef unsigned int uint32_t;
  typedef unsigned long long uint64_t;
#else
  #include <inttypes.h>
#endif

#ifndef PRIu64
  #if defined(_MSC_VER)
    #define PRIu64 "I64u"
    #define PRIx64 "I64x"
  #else
    #define PRIu64 "llu"
    #define PRIx64 "llx"
  #endif
#endif

#ifndef UINT64_C
  #define UINT64_C(v) (v ## ULL) 
#endif

#if defined(BIG_ENDIAN) || defined(__ppc__)
/**
 * This function simulate a 64-bit index of LITTLE ENDIAN 
 * in BIG ENDIAN machine.
 */
inline static int idxof(int i) {
    return i ^ 1;
}
#else
inline static int idxof(int i) {
    return i;
}
#endif

double genrand_close1_open2(void);

char *get_idstring(void);
int get_min_array_size(void);
void init_gen_rand(uint32_t seed);

/*------------------------------------------
  128-bit SIMD like data type for standard C
  ------------------------------------------*/
/** 128-bit data structure */
union W128_T {
    uint64_t u[2];
    double d[2];
};

/** 128-bit data type */
typedef union W128_T w128_t;

/*--------------------------------------
  FILE GLOBAL VARIABLES
  internal state, index counter and flag 
  --------------------------------------*/
/** the 128-bit internal state array */
static w128_t sfmt[N + 1];
/** the double pointer to the 128-bit internal state array */
static double *psfmt64 = &sfmt[0].d[0];
/** index counter to the internal state array as double */
static int idx;
/** a flag: it is 0 if and only if the internal state is not yet
 * initialized. */
static int initialized = 0;
/** a period certification vector which certificate the period of 2^{MEXP}-1. */
static uint64_t pcv[2] = {PCV1, PCV2};

/*----------------
  STATIC FUNCTIONS
  ----------------*/
static void lshift128(w128_t *out, const w128_t *in, int shift);
static void gen_rand_all(void);
static void gen_rand_array(w128_t array[], int size);
static void initial_mask(void);
static void period_certification(void);

/**
 * This function simulates SIMD 128-bit left shift by the standard C.
 * The 128-bit integer given in \b in is shifted by (shift * 8) bits.
 * This function simulates the LITTLE ENDIAN SIMD.
 * @param out the output of this function
 * @param in the 128-bit data to be shifted
 * @param shift the shift value
 */
inline static void lshift128(w128_t *out, const w128_t *in, int shift) {
    out->u[0] = in->u[0] << (shift * 8);
    out->u[1] = in->u[1] << (shift * 8);
    out->u[1] |= in->u[0] >> (64 - shift * 8);
}

/**
 * This function represents the recursion formula.
 * @param r output
 * @param a a 128-bit part of the internal state array
 * @param b a 128-bit part of the internal state array
 * @param c a 128-bit part of the internal state array
 * @param lung a 128-bit part of the internal state array
 */
inline static void do_recursion(w128_t *r, w128_t *a, w128_t *b, w128_t *c,
				w128_t *lung) {
    w128_t x;

    lshift128(&x, a, SL2);
    r->u[0] = a->u[0] ^ x.u[0] ^ ((b->u[0] >> SR1) & MSK1) ^ (c->u[0] >> SR2)
	^ (c->u[0] << SL1) ^ lung->u[1];
    r->u[1] = a->u[1] ^ x.u[1] ^ ((b->u[1] >> SR1) & MSK2) ^ (c->u[1] >> SR2)
	^ (c->u[1] << SL1) ^ lung->u[0];
    r->u[0] = (r->u[0] & LOW_MASK) | HIGH_CONST;
    r->u[1] = (r->u[1] & LOW_MASK) | HIGH_CONST;
    lung->u[0] ^= r->u[0];
    lung->u[1] ^= r->u[1];
}

/**
 * This function fills the internal state array with double precision
 * floating point pseudorandom numbers of the IEEE 754 format.
 */
inline static void gen_rand_all(void) {
    int i;
    w128_t lung;

    lung = sfmt[N];
    do_recursion(&sfmt[0], &sfmt[0], &sfmt[POS1], &sfmt[N -1], &lung);
    for (i = 1; i < N - POS1; i++) {
	do_recursion(&sfmt[i], &sfmt[i], &sfmt[i + POS1], &sfmt[i - 1], &lung);
    }
    for (; i < N; i++) {
	do_recursion(&sfmt[i], &sfmt[i], &sfmt[i + POS1 - N], &sfmt[i - 1],
		     &lung);
    }
    sfmt[N] = lung;
}

/**
 * This function initializes the internal state array to fit the IEEE
 * 754 format.
 */
void initial_mask(void) {
    int i;
    uint64_t *psfmt;

    psfmt = &sfmt[0].u[0];
    for (i = 0; i < (N + 1) * 2; i++) {
        psfmt[i] = (psfmt[i] & LOW_MASK) | HIGH_CONST;
    }
}

/**
 * This function certificate the period of 2^{MEXP}-1.
 */
static void period_certification(void) {
    int inner = 0;
    int i, j;
    uint64_t work;

    for (i = 0; i < 2; i++) {
	work = sfmt[N].u[i] & pcv[i];
	for (j = 0; j < 52; j++) {
	    inner ^= work & 1;
	    work = work >> 1;
	}
    }
    /* check OK */
    if (inner == 1) {
	return;
    }
    /* check NG, and modification */
    for (i = 0; i < 2; i++) {
	work = 1;
	for (j = 0; j < 52; j++) {
	    if ((work & pcv[i]) != 0) {
		sfmt[N].u[i] ^= work;
		return;
	    }
	    work = work << 1;
	}
    }
}

/*----------------
  PUBLIC FUNCTIONS
  ----------------*/
/**
 * This function returns the identification string.  The string shows
 * the Mersenne exponent, and all parameters of this generator.
 * @return id string.
 */
char *get_idstring(void) {
    return IDSTR;
}

/**
 * This function returns the minimum size of array used for \b
 * fill_array functions.
 * @return minimum size of array used for fill_array functions.
 */
int get_min_array_size(void) {
    return N64;
}

/**
 * This function generates and returns double precision pseudorandom
 * number which distributes uniformly in the range [1, 2).
 * init_gen_rand() or init_by_array() must be called before this
 * function.
 * @return double precision floating point pseudorandom number
 */
inline double genrand_close1_open2(void) {
    double r;

    assert(initialized);

    if (idx >= N * 2) {
	gen_rand_all();
	idx = 0;
    }
    r = psfmt64[idx++];
    return r;
}

/**
 * This function initializes the internal state array with a 32-bit
 * integer seed.
 * @param seed a 32-bit integer used as the seed.
 */
void init_gen_rand(uint32_t seed) {
    int i;
    uint32_t *psfmt;

    psfmt = (uint32_t *)&sfmt[0];
    psfmt[idxof(0)] = seed;
    for (i = 1; i < (N + 1) * 4; i++) {
	psfmt[idxof(i)] = 1812433253UL * (psfmt[idxof(i - 1)] 
					  ^ (psfmt[idxof(i - 1)] >> 30)) + i;
    }
    initial_mask();
    period_certification();
    idx = N64;
    initialized = 1;
}

#ifdef MAIN
int main(void) {
    int i;
    double d;

    printf("%s\n", get_idstring());
    printf("generated randoms [1, 2)\n");
    init_gen_rand(1234);
    for (i = 0; i < 1000; i++) {
	d = genrand_close1_open2();
	printf("%1.20lf ", d);
	if (i % 3 == 2) {
	    printf("\n");
	}
    }
    printf("\n");
    return 0;
}
#endif
