/**
 * @file SFMT-jump.c
 *
 * @brief do jump using jump polynomial.
 *
 * @author Mutsuo Saito (Hiroshima University)
 * @author Makoto Matsumoto (The University of Tokyo)
 *
 * Copyright (C) 2012 Mutsuo Saito, Makoto Matsumoto,
 * Hiroshima University and The University of Tokyo.
 * All rights reserved.
 *
 * The 3-clause BSD License is applied to this software, see
 * LICENSE.txt
 */

#include <assert.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "SFMT-params.h"
#include "SFMTst.h"
#include "SFMT-jump.h"
#include "SFMT-hardware.h"

#if defined(__cplusplus)
extern "C" {
#endif

inline static void next_state(sfmt_t * sfmt);

inline static void add(sfmt_t *dest, sfmt_t *src) {
    int dp = dest->idx / 4;
    int sp = src->idx / 4;
    int diff = (sp - dp + SFMT_N) % SFMT_N;
    int p;
    for (int i = 0; i < SFMT_N; i++) {
	p = (i + diff) % SFMT_N;
	for (int j = 0; j < 4; j++) {
	    dest->state[i].u[j] ^= src->state[p].u[j];
	}
    }
}

inline static void next_state(sfmt_t * sfmt) {
    int idx = (sfmt->idx / 4) % SFMT_N;
    w128_t *r1, *r2;
    w128_t * pstate = sfmt->state;

    r1 = &pstate[(idx + SFMT_N - 2) % SFMT_N];
    r2 = &pstate[(idx + SFMT_N - 1) % SFMT_N];
    do_recursion(&pstate[idx],
		 &pstate[idx],
		 &pstate[(idx + SFMT_POS1) % SFMT_N],
		 r1,
		 r2);
    r1 = r2;
    r2 = &pstate[idx];
    sfmt->idx = sfmt->idx + 4;
}

void SFMT_jump(sfmt_t * sfmt, const char * jump_string) {
    sfmt_t work;
    int index = sfmt->idx;
    int bits;
    memset(&work, 0, sizeof(sfmt_t));
    sfmt->idx = SFMT_N32;

    for (int i = 0; jump_string[i] != '\0'; i++) {
	bits = jump_string[i];
	assert(isxdigit(bits));
	bits = tolower(bits);
	if (bits >= 'a' && bits <= 'f') {
	    bits = bits - 'a' + 10;
	} else {
	    bits = bits - '0';
	}
	bits = bits & 0x0f;
	for (int j = 0; j < 4; j++) {
	    if ((bits & 1) != 0) {
		add(&work, sfmt);
	    }
	    next_state(sfmt);
	    bits = bits >> 1;
	}
    }
    *sfmt = work;
    sfmt->idx = index;
}

#if defined(__cplusplus)
}
#endif
