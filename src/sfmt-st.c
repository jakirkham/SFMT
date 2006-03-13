/* SFMT Search Code, M.Saito 2006/2/28 */
/* (1234) ����ִ�����Τ� */
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include "sfmt-st.h"

//#define OUT_INITIAL 1

static unsigned int POS1 = 1;
static unsigned int SL1 = 11;
static unsigned int SL2 = 7;
static unsigned int SL3 = 7;
static unsigned int SL4 = 7;
static unsigned int SR1 = 17;
static unsigned int SR2 = 9;
static unsigned int SR3 = 9;
static unsigned int SR4 = 9;

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
		 unsigned int p7, unsigned int p8, unsigned int p9) {
    POS1 = p1 % (N-2) + 1;
    SL1 = p2 % (32 - 1) + 1;
    SL2 = p3 % (32 - 1) + 1;
    SL3 = p4 % (32 - 1) + 1;
    SL4 = p5 % (32 - 1) + 1;
    SR1 = p6 % (32 - 1) + 1;
    SR2 = p7 % (32 - 1) + 1;
    SR3 = p8 % (32 - 1) + 1;
    SR4 = p9 % (32 - 1) + 1;
}

void print_param(FILE *fp) {
    fprintf(fp, "POS1 = %u\n", POS1);
    fprintf(fp, "SL1 = %u\n", SL1);
    fprintf(fp, "SL2 = %u\n", SL2);
    fprintf(fp, "SL3 = %u\n", SL3);
    fprintf(fp, "SL4 = %u\n", SL4);
    fprintf(fp, "SR1 = %u\n", SR1);
    fprintf(fp, "SR2 = %u\n", SR2);
    fprintf(fp, "SR3 = %u\n", SR3);
    fprintf(fp, "SR4 = %u\n", SR4);
    fflush(fp);
}

void print_param2(FILE *fp) {
    fprintf(fp, "[POS1, SL1, SL2, SL3, SL4, SR1, SR2, SR3, SR4] = "
	    "[%u,%u,%u,%u,%u,%u,%u,%u,%u]\n", 
	    POS1, SL1, SL2, SL3, SL4, SR1, SR2, SR3, SR4);
    fflush(fp);
}

#if 0
static void gen_rand_all(void) {
    int i;

    for (i = 0; i < N; i++) {
	sfmt[i][0] = (sfmt[i][0] << SL1) ^ sfmt[i][0]
	    ^ sfmt[(i + POS1) % N][1]
	    ^ (sfmt[(i + N - 1) % N][0] >> SR1) ^ sfmt[(i + N - 1) % N][0];
	sfmt[i][1] = (sfmt[i][1] << SL2) ^ sfmt[i][1]
	    ^ sfmt[(i + POS1) % N][2]
	    ^ (sfmt[(i + N - 1) % N][1] >> SR2) ^ sfmt[(i + N - 1) % N][1];
	sfmt[i][2] = (sfmt[i][2] << SL3) ^ sfmt[i][2]
	    ^ sfmt[(i + POS1) % N][3]
	    ^ (sfmt[(i + N - 1) % N][2] >> SR3) ^ sfmt[(i + N - 1) % N][2];
	sfmt[i][3] = (sfmt[i][3] << SL4) ^ sfmt[i][3]
	    ^ sfmt[(i + POS1) % N][0]
	    ^ (sfmt[(i + N - 1) % N][3] >> SR4) ^ sfmt[(i + N - 1) % N][3];
    }
}
#endif

void next_state128(sfmt_t *sfmt) {
    uint32_t i;

    //sfmt->idx += 4;
    if (sfmt->idx >= N * 4) {
	sfmt->idx = 0;
    }
    i = sfmt->idx / 4;
    sfmt->sfmt[i][0] = (sfmt->sfmt[i][0] << SL1) ^ sfmt->sfmt[i][0]
	^ sfmt->sfmt[(i + POS1) % N][1]
	^ (sfmt->sfmt[(i + N - 1) % N][0] >> SR1) 
	^ sfmt->sfmt[(i + N - 1) % N][0];
    sfmt->sfmt[i][1] = (sfmt->sfmt[i][1] << SL2) ^ sfmt->sfmt[i][1]
	^ sfmt->sfmt[(i + POS1) % N][2]
	^ (sfmt->sfmt[(i + N - 1) % N][1] >> SR2) 
	^ sfmt->sfmt[(i + N - 1) % N][1];
    sfmt->sfmt[i][2] = (sfmt->sfmt[i][2] << SL3) ^ sfmt->sfmt[i][2]
	^ sfmt->sfmt[(i + POS1) % N][3]
	^ (sfmt->sfmt[(i + N - 1) % N][2] >> SR3) 
	^ sfmt->sfmt[(i + N - 1) % N][2];
    sfmt->sfmt[i][3] = (sfmt->sfmt[i][3] << SL4) ^ sfmt->sfmt[i][3]
	^ sfmt->sfmt[(i + POS1) % N][0]
	^ (sfmt->sfmt[(i + N - 1) % N][3] >> SR4)
	^ sfmt->sfmt[(i + N - 1) % N][3];
}

#ifdef OUT_INITIAL
uint64_t gen_rand128(sfmt_t *sfmt, uint64_t *hi, uint64_t *low)
{
    uint32_t i;

    i = sfmt->idx / 4;
    *low = (uint64_t)sfmt->sfmt[i][0] | ((uint64_t)sfmt->sfmt[i][1] << 32);
    *hi = (uint64_t)sfmt->sfmt[i][2] | ((uint64_t)sfmt->sfmt[i][3] << 32);
    next_state128(sfmt);
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
 
    i = sfmt->idx / 4;
    r = (uint64_t)sfmt->sfmt[i][sfmt->idx % 4] 
	| ((uint64_t)sfmt->sfmt[i][sfmt->idx % 4 + 1] << 32);
    if (sfmt->idx % 4 == 2) {
	next_state128(sfmt);
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
	next_state128(sfmt);
    }
    sfmt->idx++;
    if (sfmt->idx >= N * 4) {
	sfmt->idx = 0;
    }
    return r;
}

#else  /* Do not output initial state */

uint64_t gen_rand128(sfmt_t *sfmt, uint64_t *hi, uint64_t *low)
{
    uint32_t i;

    assert(sfmt->idx % 4 == 0);

    next_state128(sfmt);
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
	next_state128(sfmt);
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
	next_state128(sfmt);
    }
    r = sfmt->sfmt[sfmt->idx / 4][sfmt->idx % 4];
    sfmt->idx++;
    if (sfmt->idx >= N * 4) {
	sfmt->idx = 0;
    }
    return r;
}
#endif

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

static unsigned int get_uint(char *line);
static unsigned int get_uint(char *line) {
    unsigned int result;

    for (;(*line) && (*line != '=');line++);
    if (!*line) {
	fprintf(stderr, "WARN:can't get = in get_uint\n");
	return 0;
    }
    line++;
    errno = 0;
    result = (unsigned int)strtol(line, NULL, 10);
    if (errno) {
	fprintf(stderr, "WARN:format error:%s", line);
    }
    return result;
}

void read_random_param(FILE *f) {
    char line[256];

    fgets(line, 256, f);
    fgets(line, 256, f);
    fgets(line, 256, f);
    POS1 = get_uint(line);
    fgets(line, 256, f);
    SL1 = get_uint(line);
    fgets(line, 256, f);
    SL2 = get_uint(line);
    fgets(line, 256, f);
    SL3 = get_uint(line);
    fgets(line, 256, f);
    SL4 = get_uint(line);
    fgets(line, 256, f);
    SR1 = get_uint(line);
    fgets(line, 256, f);
    SR2 = get_uint(line);
    fgets(line, 256, f);
    SR3 = get_uint(line);
    fgets(line, 256, f);
    SR4 = get_uint(line);
}
