/* dSFMT Search Code, M.Saito 2006/9/14 */
#include <string.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include "dsfmtL-st.h"

static uint64_t LOW_MASK   = 0x000FFFFFFFFFFFFFULL;
static uint64_t HIGH_CONST = 0x0000000000000000ULL;

static unsigned int SL1 = 31;
static unsigned int SL2 = 31;
static unsigned int SR1 = 13;
static uint64_t MSK1 = 0xedfffffbfffbffbdULL;
static uint64_t MSK2 = 0xaefeffd36dfdffdfULL;

static unsigned int get_uint(char *line, int radix);
static uint64_t get_uint64(char *line, int radix);

unsigned int get_rnd_maxdegree(void)
{
    return MAXDEGREE;
}

unsigned int get_rnd_mexp(void)
{
    return MEXP;
}

void setup_param(uint32_t array[], int *index) {
    //SL1 = (array[(*index)++] % 6 + 1) * 8; /* 128 bit */
    SL1 = array[(*index)++] % (51 - 12) + 12; 
    SL2 = array[(*index)++] % (51 - 12) + 12; 
    SR1 = array[(*index)++] % (51 - 12) + 12;
    MSK1 = array[(*index)++];
    MSK1 |= array[(*index)++];
    MSK1 |= array[(*index)++];
    MSK1 <<= 32;
    MSK1 |= array[(*index)++];
    MSK1 |= array[(*index)++];
    MSK1 |= array[(*index)++];
    //MSK1 |= 0xFFF0000000000000ULL;
    MSK2 = array[(*index)++];
    MSK2 |= array[(*index)++];
    MSK2 |= array[(*index)++];
    MSK2 <<= 32;
    MSK2 |= array[(*index)++];
    MSK2 |= array[(*index)++];
    MSK2 |= array[(*index)++];
    //MSK2 |= 0xFFF0000000000000ULL;
}

void print_param(FILE *fp) {
    fprintf(fp, "SL1 = %u\n", SL1);
    fprintf(fp, "SL2 = %u\n", SL2);
    fprintf(fp, "SR1 = %u\n", SR1);
    fprintf(fp, "MSK1 = %016"PRIx64"\n", MSK1);
    fprintf(fp, "MSK2 = %016"PRIx64"\n", MSK2);
    fflush(fp);
}

inline static void do_recursion(uint64_t a[2], uint64_t b[2],
				uint64_t lung[2]) {
    uint64_t s0, s1, t0, t1;

    s0 = (lung[1] ^ (b[1] & MSK1)) << SL1; 
    s1 = (lung[0] ^ (b[0] & MSK2)) << SL1; 
    t0 = b[0] ^ (a[0] << SL2);
    t1 = b[1] ^ (a[1] << SL2);
    lung[0] = lung[0] ^ s0 ^ t0; 
    lung[1] = lung[1] ^ s1 ^ t1;
    a[0] = a[0] ^ (lung[0] >> 12);
    a[1] = a[1] ^ (lung[1] >> 12);
}

/*
 * �����ľ�ܸƤӽФ��ʤ���genrand��ƤӽФ��Ƥ��롣
 */
inline static void next_state(dsfmt_t *dsfmt) {
    uint32_t i;

    if (dsfmt->idx >= N * 2) {
	dsfmt->idx = 0;
    }
    i = dsfmt->idx / 2;
    do_recursion(dsfmt->status[i], dsfmt->status[(i + N - 1) % N],
		 dsfmt->status[N]);
}

/* ����Ͻ�����֤���Ϥ��� */
uint64_t gen_rand104sp(dsfmt_t *dsfmt, uint64_t array[2], int mode)
{
    uint32_t i, p;

    i = dsfmt->idx / 2;
    p = dsfmt->idx + 2;
    if (p >= N * 2) {
	p = 0;
    }
    p = p / 2;
    switch (mode) {
    case 0:
	array[0] = dsfmt->status[i][0] & LOW_MASK;
	array[1] = dsfmt->status[i][1] & LOW_MASK;
	break;
    case 1:
    default:
	array[0] = dsfmt->status[i][1] & LOW_MASK;
	array[1] = dsfmt->status[p][0] & LOW_MASK;
    }

    next_state(dsfmt);
    dsfmt->idx += 2;
    if (dsfmt->idx >= N * 2) {
	dsfmt->idx = 0;
    }
    return array[0];
}

void gen_rand104spar(dsfmt_t *dsfmt, uint64_t array[][2], int size) {
    uint32_t i;
    int j;

    for (j = 0; j < size; j++) {
	i = dsfmt->idx / 2;
	array[j][0] = dsfmt->status[i][0] & LOW_MASK;
	array[j][1] = dsfmt->status[i][1] & LOW_MASK;

	next_state(dsfmt);
	dsfmt->idx += 2;
	if (dsfmt->idx >= N * 2) {
	    dsfmt->idx = 0;
	}
    }
}

void init_gen_rand(dsfmt_t *dsfmt, uint64_t seed)
{
    int i;
    uint64_t *psfmt;

    psfmt = dsfmt->status[0];
    psfmt[0] = (seed & LOW_MASK) | HIGH_CONST;
    for (i = 1; i < N * 2; i++) {
	psfmt[i] = 6364136223846793005ULL 
	    * (psfmt[i - 1] ^ (psfmt[i - 1] >> 62)) + i;
	psfmt[i] = (psfmt[i] & LOW_MASK) | HIGH_CONST;
    }
    for (;i <= N * 2; i++) {
	psfmt[i] = 6364136223846793005ULL 
	    * (psfmt[i - 1] ^ (psfmt[i - 1] >> 62)) + i;
    }
    dsfmt->idx = 0;
}

void add_rnd(dsfmt_t *dist, dsfmt_t *src) {
    int i, k;

    assert(dist->idx % 2 == 0);
    assert(src->idx % 2 == 0);
    
    k = (src->idx / 2 - dist->idx / 2 + N) % N;
    for (i = 0; i < N; i++) {
	dist->status[i][0] ^= src->status[(k + i) % N][0];
	dist->status[i][1] ^= src->status[(k + i) % N][1];
    }
    dist->status[N][0] ^= src->status[N][0];
    dist->status[N][1] ^= src->status[N][1];
}

void get_lung(dsfmt_t *dsfmt, uint64_t lung[2]) {
    lung[0] = dsfmt->status[N][0];
    lung[1] = dsfmt->status[N][1];
}

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

static uint64_t get_uint64(char *line, int radix) {
    int i;
    unsigned int x;
    uint64_t result;

    assert(radix == 16);

    for (;(*line) && (*line != '=');line++);
    if (!*line) {
	fprintf(stderr, "WARN:can't get = in get_uint\n");
	return 0;
    }
    line++;
    for (;(*line) && (*line <= ' ');line++);
    result = 0;
    for (i = 0;(*line) && (i < 16);i++,line++) {
	x = *line;
	if (('0' <= x) && (x <= '9')) {
	    x = x - '0';
	} else if (('A' <= x) && (x <= 'F')) {
	    x = x + 10 - 'A';
	} else if (('a' <= x) && (x <= 'f')) {
	    x = x + 10 - 'a';
	} else {
	    printf("format error %s\n", line);
	    return 0;
	}
	result = result * 16 + x;
    }
    return result;
}

void read_random_param(FILE *f) {
    char line[256];

    fgets(line, 256, f);
    fgets(line, 256, f);
    fgets(line, 256, f);
    SL1 = get_uint(line, 10);
    fgets(line, 256, f);
    SL2 = get_uint(line, 10);
    fgets(line, 256, f);
    SR1 = get_uint(line, 10);
    fgets(line, 256, f);
    MSK1 = get_uint64(line, 16);
    fgets(line, 256, f);
    MSK2 = get_uint64(line, 16);
}

#if defined(MAIN)
int main(void) {
    int i;
    dsfmt_t dsfmt;
    union {
	uint64_t u;
	double d;
    } un;

    LOW_MASK =   0x000FFFFFFFFFFFFFULL;
    HIGH_CONST = 0x3ff0000000000000ULL;
    init_gen_rand(&dsfmt, 1234);
    printf("generated randoms [1, 2)\n");
    for (i = 0; i < 1000; i++) {
	if (i % 2 == 0) {
	    next_state(&dsfmt);
	}
	un.u = dsfmt.status[dsfmt.idx / 2][dsfmt.idx % 2];
	dsfmt.idx++;
	printf("%1.20lf ", un.d);
	//printf("%016"PRIx64" ", un.u);
	if (i % 3 == 2) {
	    printf("\n");
	}
    }
    printf("\n");
    return 0;
}
#endif