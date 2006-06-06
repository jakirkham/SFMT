#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <limits.h>

#include <NTL/GF2X.h>
#include <NTL/vec_GF2.h>

#include "ht-shortbase32.h"
#include "util.h"

#include "ht-st.h"

NTL_CLIENT;

int get_equiv_distrib(int bit, ht_rand *sfmt);
void make_zero_state(ht_rand *sfmt, const GF2X& poly);
void test_shortest(char *filename);

static uint32_t seed;
static int mexp;
static int maxdegree;
static FILE *frandom;

void generating_polynomial32(ht_rand *sfmt, vec_GF2& vec,
			     unsigned int bitpos, 
			     unsigned int maxdegree)
{
    unsigned int i;
    uint32_t mask;
    uint32_t bit;

    //DPRINTHT("in gene:", rand);
    mask = (uint32_t)1UL << (31 - bitpos);
    for (i = 0; i<= 2 * maxdegree - 1; i++) {
	bit = gen_rand32(sfmt) & mask;
	vec[i] = (bit != 0);
    }
    //DPRINTHT("end gene:", rand);
}

int get_equiv_distrib32(int bit, ht_rand *sfmt) {
    static ht_rand sfmtnew;
    int dist;

	sfmtnew = *sfmt;
	set_up(bit);
	dist = get_shortest_base(&sfmtnew);
	return dist;
}

void make_zero_state(ht_rand *sfmt, const GF2X& poly) {
    ht_rand sfmtnew;
    int i;

    memset(&sfmtnew, 0, sizeof(sfmtnew));
    for (i = 0; i <= deg(poly); i++) {
	if (coeff(poly, i) != 0) {
	    add_rnd(&sfmtnew, sfmt, 0);
	}
	gen_rand32(sfmt);
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
    ht_rand sfmt;
    ht_rand sfmt_save;
    vec_GF2 vec;
    int shortest;
    uint32_t i;
    int dist_sum;
    int count;
    int old;
    int lcmcount;

    printf("filename:%s\n", filename);
    fp = fopen(filename, "r");
    errno = 0;
    if ((fp == NULL) || errno) {
	perror("main");
	fclose(fp);
	exit(1);
    }
    read_random_param(fp);
    init_gen_rand(&sfmt, seed);
    sfmt_save = sfmt;
    print_param(stdout);
    readFile(poly, fp);
    printf("deg poly = %ld\n", deg(poly));
    fclose(fp);
    printBinary(stdout, poly);
    vec.SetLength(2 * maxdegree);
    generating_polynomial32(&sfmt, vec, 0, maxdegree);
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
    for (i = 1; i < 32; i++) {
	//sfmt = sfmt_save;
	generating_polynomial32(&sfmt, vec, i, maxdegree);
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
#if 1 // 0状態を作るにはこれは不要？
    lcmcount = 0;
    while (deg(lcmpoly) < (long)maxdegree) {
	if (lcmcount > 5000) {
	    printf("failure\n");
	    return;
	}
	errno = 0;
	fread(sfmt.gx, sizeof(uint32_t), NN, frandom);
	if (errno) {
	    perror("set_bit");
	    fclose(frandom);
	    exit(1);
	}
	for (int j = 0; j < 1; j++) {
	    generating_polynomial32(&sfmt, vec, j, maxdegree);
	    if (IsZero(vec)) {
		break;
	    }
	    berlekampMassey(minpoly, maxdegree, vec);
	    LCM(tmp, lcmpoly, minpoly);
	    if (deg(tmp) > (long)maxdegree) {
		break;
	    }
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
    generating_polynomial32(&sfmt, vec, 0, maxdegree);
    berlekampMassey(minpoly, maxdegree, vec);
    if (deg(minpoly) != MEXP) {
	printf("deg zero state = %ld\n", deg(minpoly));
	for (i = 0; i < 10; i++) {
	    printf("%d \n", gen_rand32(&sfmt));
	}
	cout << "vec =" << vec << endl;
	cout << "minpoly = " << minpoly << endl; 
	return;
    }
    dist_sum = 0;
    count = 0;
    old = 0;
    printf("start calc distribution\n");
    fflush(stdout);
    for (bit = 1; bit <= 32; bit++) {
    // DEBUG DEBUG DEBUG
    //for (bit = 1; bit <= 1; bit++) {
	shortest = get_equiv_distrib32(bit, &sfmt);
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
    printf("32bit D.D:%7d, DUP:%5d\n", dist_sum, count);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
	printf("usage:%s filename [seed]\n", argv[0]);
	exit(1);
    }
    if (argc >= 3) {
	seed = strtol(argv[2], NULL, 10);
    } else {
	seed = 123;
    }
    if (errno) {
	perror("main");
	exit(1);
    }
    mexp = get_rnd_mexp();
    maxdegree = get_rnd_maxdegree();
    printf("mexp = %d\n", mexp);
    frandom = fopen("/dev/urandom", "r");
    if (errno) {
	perror("main");
	exit(1);
    }
    test_shortest(argv[1]);
    fclose(frandom);
    return 0;
}
