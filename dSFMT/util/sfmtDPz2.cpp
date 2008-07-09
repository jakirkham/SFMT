/* dSFMT Search Code, M.Saito 2006/9/14 */
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include "util.h"
#include "dsfmt.h"

extern "C" {
    #include "mt19937blk.h"
}

#if !defined(MEXP)
#define MEXP 19937
#endif

const int mexp = MEXP;
const int WORDSIZE = 128;
const int N = (MEXP - 128) / WORDSIZE + 1;
const int MAXDEGREE = WORDSIZE * N + 128;
const uint64_t LOW_MASK = 0x000FFFFFFFFFFFFFULL;
const uint64_t HIGH_CONST = 0x0000000000000000ULL;

int DSFMT::pos1;
int DSFMT::pos2;
int DSFMT::pos3;
int DSFMT::sl1;
int DSFMT::sl2;
int DSFMT::sl3;
int DSFMT::sr1;
int DSFMT::sr2;
int DSFMT::sr3;
uint64_t DSFMT::msk1;
uint64_t DSFMT::msk2;
uint64_t DSFMT::msk3;
uint64_t DSFMT::msk4;
uint64_t DSFMT::msk5;
uint64_t DSFMT::msk6;

unsigned int DSFMT::get_rnd_maxdegree(void) {
    return MAXDEGREE;
};

unsigned int DSFMT::get_rnd_mexp(void) {
    return mexp;
};

void DSFMT::setup_param(uint32_t array[], int *index) {
    msk1 = array[(*index)++];
    msk1 |= array[(*index)++];
    msk1 |= array[(*index)++];
    msk1 <<= 32;
    msk1 |= array[(*index)++];
    msk1 |= array[(*index)++];
    msk1 |= array[(*index)++];
    msk2 = array[(*index)++];
    msk2 |= array[(*index)++];
    msk2 |= array[(*index)++];
    msk2 <<= 32;
    msk2 |= array[(*index)++];
    msk2 |= array[(*index)++];
    msk2 |= array[(*index)++];
}

void DSFMT::print_param(FILE *fp) {
    fprintf(fp, "MSK1 = %016" PRIX64 "\n", msk1);
    fprintf(fp, "MSK2 = %016" PRIX64 "\n", msk2);
    fflush(fp);
}

DSFMT::DSFMT(uint64_t seed) {
    status = new uint64_t[N + 1][2];
    idx = 0;
    init_gen_rand(seed);
}

DSFMT::DSFMT(const DSFMT& src) {
    int i;

    status = new uint64_t[N + 1][2];
    for (i = 0; i < N + 1; i++) {
	status[i][0] = src.status[i][0];
	status[i][1] = src.status[i][1];
    }
    idx = src.idx;
}

DSFMT& DSFMT::operator=(const DSFMT& src) {
    int i;

    for (i = 0; i < N + 1; i++) {
	status[i][0] = src.status[i][0];
	status[i][1] = src.status[i][1];
    }
    idx = src.idx;
    return *this;
}

void DSFMT::d_p() {
    int i;

    printf("idx = %d\n", idx);
    for (i = 0; i < N + 1; i++) {
	printf("%016" PRIx64 " %016" PRIx64 "\n", status[i][0], status[i][1]);
    }
}

DSFMT::~DSFMT() {
    delete status;
}

inline static void do_recursion(uint64_t a[2], uint64_t lung[2]) {
    uint64_t mat1[2] = {0, DSFMT::msk1};
    uint64_t mat2[2] = {0, DSFMT::msk2};
    uint64_t t0, t1;

    t0 = a[0];
    t1 = a[1];
    a[0] = lung[1] ^ (t0 >> 1) ^ mat1[t0 & 1];
    a[1] = lung[0] ^ (t1 >> 1) ^ mat2[t1 & 1];
    lung[0] = (lung[0] << 8) ^ lung[0] ^ t0;
    lung[1] = (lung[1] << 8) ^ lung[1] ^ t1;
}

/*
 * �����ľ�ܸƤӽФ��ʤ���genrand��ƤӽФ��Ƥ��롣
 */
void DSFMT::next_state() {
    uint32_t i;

    if (idx >= N * 2) {
	idx = 0;
    }
    i = idx / 2;
    do_recursion(status[i], status[N]);
}

/* ����Ͻ�����֤���Ϥ��� */
uint64_t DSFMT::gen_rand104sp(uint64_t array[2], int mode)
{
    int i, p;

    i = idx / 2;
    p = idx + 2;
    if (p >= N * 2) {
	p = 0;
    }
    p = p / 2;
    switch (mode) {
    case 0:
	array[0] = status[i][0] & LOW_MASK;
	array[1] = status[i][1] & LOW_MASK;
	break;
    case 1:
    default:
	array[0] = status[i][1] & LOW_MASK;
	array[1] = status[p][0] & LOW_MASK;
    }

    next_state();
    idx += 2;
    if (idx >= N * 2) {
	idx = 0;
    }
    return array[0];
}

void DSFMT::gen_rand104spar(uint64_t array[][2], int size) {
    int i;
    int j;

    for (j = 0; j < size; j++) {
	i = idx / 2;
	array[j][0] = status[i][0] & LOW_MASK;
	array[j][1] = status[i][1] & LOW_MASK;

	next_state();
	idx += 2;
	if (idx >= N * 2) {
	    idx = 0;
	}
    }
}

void DSFMT::add(const DSFMT& src) {
    int i, k;

    assert(idx % 2 == 0);
    assert(src.idx % 2 == 0);
    
    k = (src.idx / 2 - idx / 2 + N) % N;
    for (i = 0; i < N; i++) {
	status[i][0] ^= src.status[(k + i) % N][0];
	status[i][1] ^= src.status[(k + i) % N][1];
    }
    status[N][0] ^= src.status[N][0];
    status[N][1] ^= src.status[N][1];
}

void DSFMT::get_lung(uint64_t lung[2]) {
    lung[0] = status[N][0];
    lung[1] = status[N][1];
}

void DSFMT::init_gen_rand(uint64_t seed)
{
    int i;
    uint64_t *psfmt;

    psfmt = status[0];
    psfmt[0] = seed;
    for (i = 1; i < N * 2; i++) {
	psfmt[i] = 6364136223846793005ULL 
	    * (psfmt[i - 1] ^ (psfmt[i - 1] >> 62)) + i;
    }
    for (;i < (N + 1) * 2; i++) {
	psfmt[i] = 6364136223846793005ULL 
	    * (psfmt[i - 1] ^ (psfmt[i - 1] >> 62)) + i;
    }
    idx = 0;
}

void DSFMT::read_random_param(FILE *f) {
    char line[256];

    fgets(line, 256, f);
    fgets(line, 256, f);
    fgets(line, 256, f);
    msk1 = get_uint64(line, 16);
    fgets(line, 256, f);
    msk2 = get_uint64(line, 16);
}

void DSFMT::fill_rnd() {
    const int size = (N + 1) * 4;
    uint32_t array[size];
    uint64_t u;
    int i, j;

    mt_fill(array, size);
    for (i = 0; i < N; i++) {
	for (j = 0; j < 2; j++) {
	    u = array[idx++];
	    u = u << 32;
	    //u = (u | array[idx++]) & 0x000FFFFFFFFFFFFFULL;
	    u = (u | array[idx++]);
	    status[i][j] = u;
	}
    }
    for (j = 0; j < 2; j++) {
	u = array[idx++];
	u = u << 32;
	u = u | array[idx++];
	status[i][j] = u;
    }
}

#if defined(MAIN)
int main(int argc, char * argv[]) {
    int i;
    dsfmt_t dsfmt;
    union {
	uint64_t u;
	double d;
    } un;
    FILE *fp;

    LOW_MASK =   0x000FFFFFFFFFFFFFULL;
    HIGH_CONST = 0x3ff0000000000000ULL;
    if (argc > 1) {
	fp = fopen(argv[1], "r");
	errno = 0;
	if ((fp == NULL) || errno) {
	    perror("main");
	    fclose(fp);
	    exit(1);
	}
	read_random_param(fp);
    }
    init_gen_rand(&dsfmt, 1234);
    printf("generated randoms [1, 2)\n");
    for (i = 0; i < 1000; i++) {
	if (i % 2 == 0) {
	    next_state(&dsfmt);
	}
	un.u = dsfmt.status[dsfmt.idx / 2][dsfmt.idx % 2];
	dsfmt.idx++;
	printf("%1.17f ", un.d);
	//printf("%016"PRIx64" ", un.u);
	if (i % 3 == 2) {
	    printf("\n");
	}
    }
    printf("\n");
    return 0;
}
#endif