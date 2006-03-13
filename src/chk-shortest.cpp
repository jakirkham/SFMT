#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <iostream>

#include <NTL/GF2X.h>
#include <NTL/vec_GF2.h>

#include "shortbase128.h"
#include "util.h"

extern "C" {
#include "sfmt-st.h"
}

NTL_CLIENT;

int get_equiv_distrib(int bit, sfmt_t *sfmt);
void make_zero_state(sfmt_t *sfmt, const GF2X& poly);
void test_shortest(char *filename);

static int mexp;
static int maxdegree;
static FILE *frandom;

bool generating_polynomial128_hi(sfmt_t *sfmt, vec_GF2& vec,
				 unsigned int bitpos, 
				 unsigned int maxdegree)
{
    unsigned int i;
    uint64_t hi, low;
    uint64_t mask;
    uint64_t bit;

    //DPRINTHT("in gene:", rand);
    i = 0;
    gen_rand128(sfmt, &hi, &low);
    mask = (uint64_t)1UL << (63 - bitpos);
    bit = hi & mask;
    while (!bit) {
	i++;
	if(i > 2 * maxdegree){
	    //printf("generating_polynomial:too much zeros\n");
	    vec[0] = 1;
	    return false;
	}
	gen_rand128(sfmt, &hi, &low);
	bit = hi & mask;
    }
    //DPRINTHT("middle gene:", rand);
    vec[0] = 1;

    for (i=1; i<= 2 * maxdegree-1; i++) {
	gen_rand128(sfmt, &hi, &low);
	bit = (hi & mask);
	vec[i] = (bit != 0);
    }
    //DPRINTHT("end gene:", rand);
    return true;
}

bool generating_polynomial128_low(sfmt_t *sfmt, vec_GF2& vec,
				 unsigned int bitpos, 
				 unsigned int maxdegree)
{
    unsigned int i;
    uint64_t hi, low;
    uint64_t mask;
    uint64_t bit;

    //DPRINTHT("in gene:", rand);
    i = 0;
    gen_rand128(sfmt, &hi, &low);
    mask = (uint64_t)1UL << (63 - bitpos);
    bit = low & mask;
    while (!bit) {
	i++;
	if(i > 2 * maxdegree){
	    //printf("generating_polynomial:too much zeros\n");
	    vec[0] = 1;
	    return false;
	}
	gen_rand128(sfmt, &hi, &low);
	bit = low & mask;
    }
    //DPRINTHT("middle gene:", rand);
    vec[0] = 1;

    for (i=1; i<= 2 * maxdegree-1; i++) {
	gen_rand128(sfmt, &hi, &low);
	bit = (low & mask);
	vec[i] = (bit != 0);
    }
    //DPRINTHT("end gene:", rand);
    return true;
}

bool generating_polynomial128(sfmt_t *sfmt, vec_GF2& vec, unsigned int bitpos, 
			   unsigned int maxdegree) {
    if (bitpos < 64) {
	return generating_polynomial128_hi(sfmt, vec, bitpos, maxdegree);
    } else {
	return generating_polynomial128_low(sfmt, vec, bitpos - 64, maxdegree);
    }
}


bool check_minpoly128_hi(sfmt_t *sfmt, const GF2X& minpoly,
			 unsigned int bitpos) {
    uint32_t sum;
    uint64_t hi, low;
    uint64_t mask;
    int i;

    sum = 0;
    mask = (uint64_t)1UL << (63 - bitpos);
    for (int j = 0; j < 500; j++) {
	for (i = 0; i <= deg(minpoly); i++) {
	    gen_rand128(sfmt, &hi, &low);
	    if (mask & hi != 0) {
		sum ^= 1;
	    }
	}
	if (sum != 0) {
	    return false;
	}
    }
    return true;
}

bool check_minpoly128_low(sfmt_t *sfmt, const GF2X& minpoly,
			  unsigned int bitpos) {
    uint32_t sum;
    uint64_t hi, low;
    uint64_t mask;
    int i;

    sum = 0;
    mask = (uint64_t)1UL << (63 - bitpos);
    for (int j = 0; j < 500; j++) {
	for (i = 0; i <= deg(minpoly); i++) {
	    gen_rand128(sfmt, &hi, &low);
	    if (mask & low != 0) {
		sum ^= 1;
	    }
	}
	if (sum != 0) {
	    return false;
	}
    }
    return true;
}

bool check_minpoly128(sfmt_t *sfmt, const GF2X& minpoly, unsigned int bitpos) {
    if (bitpos < 64) {
	return check_minpoly128_hi(sfmt, minpoly, bitpos);
    } else {
	return check_minpoly128_low(sfmt, minpoly, bitpos - 64);
    }
}

int get_equiv_distrib(int bit, sfmt_t *sfmt) {
    static sfmt_t sfmtnew;
    int shortest;

    //fprintf(stderr, "now start get_equiv %d\n", bit);
    sfmtnew = *sfmt;
    set_bit_len(bit);
    shortest = get_shortest_base(bit, &sfmtnew);
    return shortest;
}

void make_zero_state(sfmt_t *sfmt, const GF2X& poly) {
    sfmt_t sfmtnew;
    int i;

    memset(&sfmtnew, 0, sizeof(sfmtnew));
    for (i = 0; i <= deg(poly); i++) {
	if (coeff(poly, i) != 0) {
	    add_rnd(&sfmtnew, sfmt);
	}
	next_state128(sfmt);
    }
    *sfmt = sfmtnew;
}

void test_shortest(char *filename) {
    unsigned int bit;
    FILE *fp;
    GF2X poly;
    GF2X lcmpoly;
    GF2X minpoly;
    GF2X tmp;
    GF2X rempoly;
    sfmt_t sfmt;
    sfmt_t sfmt_save;
    vec_GF2 vec;
    int shortest;
    uint32_t i;
    int dist_sum;
    int count;
    int old;
    int lcmcount;
    bool rc;

    cout << "filename:" << filename << endl;
    fp = fopen(filename, "r");
    errno = 0;
    if ((fp == NULL) || errno) {
	perror("main");
	fclose(fp);
	exit(1);
    }
    read_random_param(fp);
    init_gen_rand(&sfmt, 123);
    sfmt_save = sfmt;
    print_param(stdout);
    readFile(poly, fp);
    printf("deg poly = %ld\n", deg(poly));
    fclose(fp);
    printBinary(stdout, poly);
    vec.SetLength(2 * maxdegree);
    generating_polynomial128(&sfmt, vec, 0, maxdegree);
    berlekampMassey(lcmpoly, maxdegree, vec);
#if 0
    if (check_minpoly128(&sfmt, lcmpoly, 0)) {
	printf("check minpoly OK!\n");
	DivRem(tmp, rempoly, lcmpoly, poly);
	if (deg(rempoly) != -1) {
	    printf("rem != 0 deg rempoly = %ld: 0\n", deg(rempoly));
	}

    } else {
	printf("check minpoly NG!\n");
    }
#endif
    for (i = 1; i < 128; i++) {
	//sfmt = sfmt_save;
	generating_polynomial128(&sfmt, vec, i, maxdegree);
	berlekampMassey(minpoly, maxdegree, vec);
	LCM(tmp, lcmpoly, minpoly);
	lcmpoly = tmp;
#if 0
	DivRem(tmp, rempoly, lcmpoly, poly);
	if (deg(rempoly) != -1) {
	    printf("rem != 0 deg rempoly = %ld: %d\n", deg(rempoly), i);
	}
#endif
    }
#if 0 // 0���֤���ˤϤ�������ס�
    lcmcount = 0;
    while (deg(lcmpoly) < (long)maxdegree) {
	if (lcmcount > 1000) {
	    printf("failure\n");
	    return;
	}
	errno = 0;
	fread(sfmt.sfmt, sizeof(uint32_t), N * 4, frandom);
	if (errno) {
	    perror("set_bit");
	    fclose(frandom);
	    exit(1);
	}
	for (int j = 0; j < 128; j++) {
	    rc = generating_polynomial128(&sfmt, vec, j, maxdegree);
	    if (!rc) {
		break;
	    }
	    berlekampMassey(minpoly, maxdegree, vec);
	    LCM(tmp, lcmpoly, minpoly);
	    lcmpoly = tmp;
	    lcmcount++;
	    if (deg(lcmpoly) >= (long)maxdegree) {
		break;
	    }
	}
    }
    if (deg(lcmpoly) != maxdegree) {
	printf("fail to get lcm, deg = %ld\n", deg(lcmpoly));
	exit(1);
    }
#endif
#if 0
    sfmt = sfmt_save;
    if (check_minpoly128(&sfmt, lcmpoly, 0)) {
	printf("check minpoly 2 OK!\n");
    } else {
	printf("check minpoly 2 NG!\n");
    }
#endif
    printf("deg lcm poly = %ld\n", deg(lcmpoly));
    printf("weight = %ld\n", weight(lcmpoly));
    printBinary(stdout, lcmpoly);
    DivRem(tmp, rempoly, lcmpoly, poly);
    printf("deg tmp = %ld\n", deg(tmp));
    if (deg(rempoly) != -1) {
	printf("rem != 0 deg rempoly = %ld\n", deg(rempoly));
	exit(1);
    }
    sfmt = sfmt_save;
    make_zero_state(&sfmt, tmp);
    generating_polynomial128(&sfmt, vec, 0, maxdegree);
    berlekampMassey(minpoly, maxdegree, vec);
    if (deg(minpoly) != MEXP) {
	printf("deg zero state = %ld\n", deg(minpoly));
	return;
    }
    dist_sum = 0;
    count = 0;
    old = 0;
    for (bit = 1; bit <= 128; bit++) {
	shortest = get_equiv_distrib(bit, &sfmt);
	dist_sum += mexp / bit - shortest;
	if (old == shortest) {
	    count++;
	} else {
	    old = shortest;
	}
	//printf("k(%d) = %d, %d, %d\n", bit, shortest, dist_sum, count);
	printf("k(%d) = %d\n", bit, shortest);
	fflush(stdout);
    }
    printf("D.D:%7d, DUP:%5d\n", dist_sum, count);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
	printf("usage:%s filename\n", argv[0]);
	exit(1);
    }
    mexp = get_rnd_mexp();
    maxdegree = get_rnd_maxdegree();
    printf("mexp = %d\n", mexp);
    frandom = fopen("/dev/random", "r");
    if (errno) {
	perror("main");
	exit(1);
    }
    test_shortest(argv[1]);
    fclose(frandom);
    return 0;
}