/* SFMT Search Code, M.Saito 2006/2/28 */
/* sfmt7x7 */
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include "sfmt-st.h"

//#define OUT_INITIAL 1
static inline void do_recursion(uint32_t a[4], uint32_t b[4], uint32_t c[4]);


static unsigned int POS1 = 1;
static unsigned int SL1 = 11;
static unsigned int SR1 = 7;
static unsigned int MSK1 = 7;
static unsigned int MSK2 = 7;
static unsigned int MSK3 = 7;
static unsigned int MSK4 = 7;

unsigned int get_rnd_maxdegree(void)
{
    return MAXDEGREE;
}

unsigned int get_rnd_mexp(void)
{
    return MEXP;
}

void setup_param(unsigned int p1, unsigned int p2, unsigned int p3, 
		 unsigned int p4, unsigned int p5, unsigned int p6,
		 unsigned int p7, unsigned int p8, unsigned int p9,
		 unsigned int p10, unsigned int p11, unsigned int p12,
		 unsigned int p13) {
    POS1 = p1 % (N-2) + 1;
    SL1 = p2 % (32 - 1) + 1;
    SR1 = p3 % (32 - 1) + 1;
    MSK1= p4 | p5;
    MSK2= p6 | p7;
    MSK3= p8 | p9;
    MSK4= p10 | p11;
}

void print_param(FILE *fp) {
    fprintf(fp, "POS1 = %u\n", POS1);
    fprintf(fp, "SL1 = %u\n", SL1);
    fprintf(fp, "SR1 = %u\n", SR1);
    fprintf(fp, "MSK1 = %08x\n", MSK1);
    fprintf(fp, "MSK2 = %08x\n", MSK2);
    fprintf(fp, "MSK3 = %08x\n", MSK3);
    fprintf(fp, "MSK4 = %08x\n", MSK4);
    fflush(fp);
}

void print_param2(FILE *fp) {
    fprintf(fp, "[POS1, SL1, SR1, MSK1, MSK2, MSK3, MSK4] = "
	    "[%u,%u,%u,%u,%u,%u,%u]\n", 
	    POS1, SL1, SR1, MSK1, MSK2, MSK3, MSK4);
    fflush(fp);
}

static inline void do_recursion(uint32_t a[4], uint32_t b[4], uint32_t c[4]) {
    uint64_t t;
    uint32_t bb[4];
    uint32_t aa[4];
    int i;

    t = ((uint64_t)b[1] << 32) | ((uint64_t)b[0]);
    if (SR1 == 0) {
	t = t << SL1;
    } else {
	t = t >> SL1;
    }
    bb[1] = (uint32_t)(t >> 32);
    bb[0] = (uint32_t)t;
    t = ((uint64_t)b[3] << 32) | ((uint64_t)b[2]);
    if (SR1 == 0) {
	t = t << SL1;
    } else {
	t = t >> SL1;
    }
    bb[3] = (uint32_t)(t >> 32);
    bb[2] = (uint32_t)t;
    for (i = 0; i < 4; i++) {
	aa[i] = a[i];
    }
    a[0] = aa[1] ^ b[1] ^ bb[0] ^ (c[0] & MSK1);
    a[1] = aa[2] ^ b[3] ^ bb[1] ^ (c[1] & MSK2);
    a[2] = aa[3] ^ b[0] ^ bb[2] ^ (c[2] & MSK3);
    a[3] = aa[0] ^ b[2] ^ bb[3] ^ (c[3] & MSK4);
}

/*
 * これは直接呼び出さないでgenrandを呼び出している。
 */
static void next_state(sfmt_t *sfmt) {
    uint32_t i;

    //sfmt->idx += 4;
    if (sfmt->idx >= N * 4) {
	sfmt->idx = 0;
    }
    i = sfmt->idx / 4;
    do_recursion(sfmt->sfmt[i], sfmt->sfmt[(i + POS1) % N],
		 sfmt->sfmt[(i + N - 1) % N]);
}

/*------------------------------------
 * Output initial state 
 ------------------------------------*/
#ifdef OUT_INITIAL
uint64_t gen_rand128(sfmt_t *sfmt, uint64_t *hi, uint64_t *low)
{
    uint32_t i;

    i = sfmt->idx / 4;
    *low = (uint64_t)sfmt->sfmt[i][0] | ((uint64_t)sfmt->sfmt[i][1] << 32);
    *hi = (uint64_t)sfmt->sfmt[i][2] | ((uint64_t)sfmt->sfmt[i][3] << 32);
    next_state(sfmt);
    sfmt->idx += 4;
    sfmt->idx = (sfmt->idx / 4) * 4;
    if (sfmt->idx >= N * 4) {
	sfmt->idx = 0;
    }
    return *hi;
}

uint64_t gen_rand64(sfmt_t *sfmt)
{
    uint64_t r;
    uint32_t i;

    assert(sfmt->idx % 2 == 0);
 
    i = sfmt->idx / 4;
    r = (uint64_t)sfmt->sfmt[i][sfmt->idx % 4] 
	| ((uint64_t)sfmt->sfmt[i][sfmt->idx % 4 + 1] << 32);
    if (sfmt->idx % 4 == 2) {
	next_state(sfmt);
    }
    sfmt->idx += 2;
    if (sfmt->idx >= N * 4) {
	sfmt->idx = 0;
    }

    return r;
}

uint32_t gen_rand32(sfmt_t *sfmt)
{
    uint32_t r;

    r = sfmt->sfmt[sfmt->idx / 4][sfmt->idx % 4];
    if (sfmt->idx % 4 == 3) {
	next_state(sfmt);
    }
    sfmt->idx++;
    if (sfmt->idx >= N * 4) {
	sfmt->idx = 0;
    }
    return r;
}

/*------------------------------------
 * Do not output initial state 
 ------------------------------------*/
#else  

uint64_t gen_rand128(sfmt_t *sfmt, uint64_t *hi, uint64_t *low)
{
    uint32_t i;

    assert(sfmt->idx % 4 == 0);

    next_state(sfmt);
    i = sfmt->idx / 4;
    *low = (uint64_t)sfmt->sfmt[i][0] | ((uint64_t)sfmt->sfmt[i][1] << 32);
    *hi = (uint64_t)sfmt->sfmt[i][2] | ((uint64_t)sfmt->sfmt[i][3] << 32);
    sfmt->idx += 4;
    if (sfmt->idx >= N * 4) {
	sfmt->idx = 0;
    }
    return *hi;
}

uint64_t gen_rand64(sfmt_t *sfmt)
{
    uint64_t r;
    uint32_t i;

    assert(sfmt->idx % 2 == 0);
 
    if (sfmt->idx % 4 == 0) {
	next_state(sfmt);
    }
    i = sfmt->idx / 4;
    r = (uint64_t)sfmt->sfmt[i][sfmt->idx % 4] 
	| ((uint64_t)sfmt->sfmt[i][sfmt->idx % 4 + 1] << 32);
    sfmt->idx += 2;
    if (sfmt->idx >= N * 4) {
	sfmt->idx = 0;
    }
    return r;
}

uint32_t gen_rand32(sfmt_t *sfmt)
{
    uint32_t r;

    if (sfmt->idx % 4 == 0) {
	next_state(sfmt);
    }
    r = sfmt->sfmt[sfmt->idx / 4][sfmt->idx % 4];
    sfmt->idx++;
    if (sfmt->idx >= N * 4) {
	sfmt->idx = 0;
    }
    return r;
}
#endif

/* これは初期状態を出力する */
uint32_t gen_rand128sp(sfmt_t *sfmt, uint32_t array[4], uint32_t mode)
{
    uint32_t i, p;

    i = sfmt->idx / 4;
    p = sfmt->idx + 4;
    if (p >= N * 4) {
	p = 0;
    }
    p = p / 4;
    switch (mode) {
    case 0:
	array[0] = sfmt->sfmt[i][0];
	array[1] = sfmt->sfmt[i][1];
	array[2] = sfmt->sfmt[i][2];
	array[3] = sfmt->sfmt[i][3];
	break;
    case 1:
	array[0] = sfmt->sfmt[i][1];
	array[1] = sfmt->sfmt[i][2];
	array[2] = sfmt->sfmt[i][3];
	array[3] = sfmt->sfmt[p][0];
	break;
    case 2:
	array[0] = sfmt->sfmt[i][2];
	array[1] = sfmt->sfmt[i][3];
	array[2] = sfmt->sfmt[p][0];
	array[3] = sfmt->sfmt[p][1];
	break;
    case 3:
    default:
	array[0] = sfmt->sfmt[i][3];
	array[1] = sfmt->sfmt[p][0];
	array[2] = sfmt->sfmt[p][1];
	array[3] = sfmt->sfmt[p][2];
    }

    next_state(sfmt);
    sfmt->idx += 4;
    if (sfmt->idx >= N * 4) {
	sfmt->idx = 0;
    }
    return array[0];
}

void init_gen_rand(sfmt_t *sfmt, uint32_t seed)
{
    int i;

    sfmt->sfmt[0][0] = seed;
    for (i = 1; i < N * 4; i++) {
	sfmt->sfmt[i / 4][i % 4] = 1812433253UL 
	    * (sfmt->sfmt[(i - 1) / 4][(i - 1) % 4]
	       ^ (sfmt->sfmt[(i - 1) / 4][(i - 1) % 4] >> 30)) + i;
    }
    sfmt->idx = 0;
}

void add_rnd(sfmt_t *dist, sfmt_t *src) {
    int i, j, k;

    assert(dist->idx % 4 == 0);
    assert(src->idx % 4 == 0);
    
    k = (src->idx / 4 - dist->idx / 4 + N) % N;
    for (i = 0; i < N; i++) {
	for (j = 0; j < 4; j++) {
	    dist->sfmt[i][j] ^= src->sfmt[(k + i) % N][j];
	}
    }
}

uint32_t get_lung(sfmt_t *sfmt) {
    return sfmt->sfmt[N - 1][3];
}

static unsigned int get_uint(char *line, int radix);
static unsigned int get_uint(char *line, int radix) {
    unsigned int result;

    for (;(*line) && (*line != '=');line++);
    if (!*line) {
	fprintf(stderr, "WARN:can't get = in get_uint\n");
	return 0;
    }
    line++;
    errno = 0;
    result = (unsigned int)strtoll(line, NULL, radix);
    if (errno) {
	fprintf(stderr, "WARN:format error:%s", line);
	perror("get_unit");
    }
    return result;
}

void read_random_param(FILE *f) {
    char line[256];

    fgets(line, 256, f);
    fgets(line, 256, f);
    fgets(line, 256, f);
    POS1 = get_uint(line, 10);
    fgets(line, 256, f);
    SL1 = get_uint(line, 10);
    fgets(line, 256, f);
    SR1 = get_uint(line, 10);
    fgets(line, 256, f);
    MSK1 = get_uint(line, 16);
    fgets(line, 256, f);
    MSK2 = get_uint(line, 16);
    fgets(line, 256, f);
    MSK3 = get_uint(line, 16);
    fgets(line, 256, f);
    MSK4 = get_uint(line, 16);
}

