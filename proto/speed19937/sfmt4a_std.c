/* Hearty Twister Search Code, Makoto Matsumoto 2005/5/6 */

#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include "random-common.h"

#ifndef MEXP
#define MEXP 19937
#endif

#define WORDSIZE 128
#define N (MEXP / WORDSIZE + 1)
#define MAXDEGREE (WORDSIZE * N)

static uint32_t sfmt[N][4];
static unsigned int idx;

static unsigned int POS1 = 71;
static unsigned int SL1 = 11;
static unsigned int SL2 = 7;
static unsigned int SR1 = 17;

unsigned int get_rnd_maxdegree(void)
{
    return MAXDEGREE;
}

unsigned int get_rnd_mexp(void)
{
    return MEXP;
}

unsigned int get_onetime_rnds(void) {
    return N * 4;
}

void print_param(FILE *fp) {
    fprintf(fp, "POS1 = %u\n", POS1);
    fprintf(fp, "SL1 = %u\n", SL1);
    fprintf(fp, "SL2 = %u\n", SL2);
    fprintf(fp, "SR1 = %u\n", SR1);
    fflush(fp);
}

void print_param2(FILE *fp) {
    fprintf(fp, "[POS1, SL1, SL2, SR1] = [%u,%u,%u,%u]\n", 
	    POS1, SL1, SL2, SR1);
    fflush(fp);
}

void print_state(FILE *fp) {
    int i, j;
    for (i = 0; i < N; i++) {
	for (j = 0; j < 4; j++) {
	    printf("%08x ", sfmt[i][j]);
	}
	if (i % 2 == 1) {
	    printf("\n");
	}
    }
}

void gen_rand_all(void) {
    int i;

    for (i = 0; i < N; i++) {
	sfmt[i][0] = (sfmt[i][0] << SL1) ^ sfmt[i][0]
	    ^ (sfmt[(i + POS1) % N][0] >> SR1) ^ sfmt[(i + POS1) % N][1]
	    ^ (sfmt[(i + N - 1) % N][0] << SL2); 
	sfmt[i][1] = (sfmt[i][1] << SL1) ^ sfmt[i][1]
	    ^ (sfmt[(i + POS1) % N][1] >> SR1) ^ sfmt[(i + POS1) % N][2]
	    ^ (sfmt[(i + N - 1) % N][1] << SL2) ^ sfmt[(i + N -1) % N][0];
	sfmt[i][2] = (sfmt[i][2] << SL1) ^ sfmt[i][2]
	    ^ (sfmt[(i + POS1) % N][2] >> SR1) ^ sfmt[(i + POS1) % N][3]
	    ^ (sfmt[(i + N - 1) % N][2] << SL2) ^ sfmt[(i + N -1) % N][1];
	sfmt[i][3] = (sfmt[i][3] << SL1) ^ sfmt[i][3]
	    ^ (sfmt[(i + POS1) % N][3] >> SR1)
	    ^ (sfmt[(i + N - 1) % N][3] << SL2) ^ sfmt[(i + N -1) % N][2];
    }
}

uint32_t gen_rand(void)
{
    uint32_t r;

    if (idx >= N * 4) {
	gen_rand_all();
	idx = 0;
    }
    r = sfmt[idx / 4][idx % 4];
    idx++;
    return r;
}

void init_gen_rand(uint32_t seed)
{
    int i;

    sfmt[0][0] = seed;
    for (i = 1; i < N * 4; i++) {
	sfmt[i/4][i%4] = 1812433253UL 
	    * (sfmt[(i - 1) / 4][(i - 1) % 4]
	       ^ (sfmt[(i - 1) / 4][(i - 1) % 4] >> 30)) 
	    + i;
    }
    idx = N * 4;
}