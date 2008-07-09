/* Hearty Twister Search Code, Makoto Matsumoto 2005/5/6 */

#include <stdio.h>
#include <errno.h>

#include <NTL/GF2X.h>
#include <NTL/vec_GF2.h>
#include <NTL/GF2XFactoring.h>
#include "util.h"

NTL_CLIENT;

long IterIrredTest(const GF2X& f, long d, GF2X& g)
{
   long df = deg(f);

   if (df <= 0) return 0;
   if (df == 1) return 1;

   GF2XModulus F;

   g %= f;
   build(F, f);
   
   long i, limit, limit_sqr;
   GF2X X, t, prod;


   SetX(X);

   i = 0;
   limit = 2;
   limit_sqr = limit*limit;

   set(prod);

   while (2*d <= df) {
      add(t, g, X);
      MulMod(prod, prod, t, F);
      i++;
      if (i == limit_sqr) {
         GCD(t, f, prod);
         if (!IsOne(t)) return 0;

         set(prod);
         limit++;
         limit_sqr = limit*limit;
         i = 0;
      }

      d = d + 1;
      if (2*d <= deg(f)) {
         SqrMod(g, g, F);
      }
   }

   if (i > 0) {
      GCD(t, f, prod);
      if (!IsOne(t)) return 0;
   }

   return 1;
}

int non_reducible(GF2X& fpoly, int degree) {
    int m;
    long df;

    df = deg(fpoly);
    if (df < degree) {
	return 0;
    }
    // lazy square free
    GF2X d, g;
    diff(d, fpoly);
    GCD(g, fpoly, d);
    if (df - deg(g) < degree) {
	return 0;
    }
    fpoly /= g;
    df = deg(fpoly);
    // end
    GF2X t2(2, 1);
    GF2X t1(1, 1);
    GF2X t2m;
    GF2X t;
    GF2X alpha;
    GF2XModulus modf;
    build(modf, fpoly);
    t2m = t2;
    add(t, t2m, t1);
    for (m = 1; df > degree; m++) {
	GCD(alpha, fpoly, t);
	if (!IsOne(alpha)) {
	    if (df - deg(alpha) < degree) {
		return 0;
	    }
	    fpoly /= alpha;
	    df = deg(fpoly);
	    if (df == degree) {
		break;
	    }
	}
	if (df <= degree + m) {
	    return 0;
	}
	SqrMod(t2m, t2m, modf);
	add(t, t2m, t1);
    }
    if (df != degree) {
	return 0;
    }
    return IterIrredTest(fpoly, m, t2m);
}

void berlekampMassey(GF2X& minpoly, unsigned int maxdegree, vec_GF2& genvec) 
{
    GF2X zero;

    if (genvec.length() == 0) {
	minpoly = zero;
	return;
    }
    MinPolySeq(minpoly, genvec, maxdegree);
}

void printBinary(FILE *fp, GF2X& poly)
{
    int i;
    if (deg(poly) < 0) {
	fprintf(fp, "0deg=-1\n");
	return;
    }
    for(i = 0; i <= deg(poly); i++) {
	if(rep(coeff(poly, i)) == 1) {
	    fprintf(fp, "1");
	} else {
	    fprintf(fp, "0");
	}
	/* printf("%1d", (unsigned int)(poly[j] >> (i % WORDLL)) & 0x1ULL);*/
	if ((i % 32) == 31) {
	    fprintf(fp, "\n");
	}
    }
    fprintf(fp, "deg=%ld\n", deg(poly));
}

void LCM(GF2X& lcm, const GF2X& x, const GF2X& y) {
    GF2X gcd;
    mul(lcm, x, y);
    GCD(gcd, x, y);
    lcm /= gcd;
}

int32_t gauss_plus(mat_GF2& mat) {
    int32_t rank;
    int32_t rows;
    int32_t cols;
    int32_t i, j, pos;
    
    rank = gauss(mat);
    rows = mat.NumRows();
    cols = mat.NumCols();
    pos = 0;
    for (i = 0; i < rows; i++) {
	while ((i + pos < cols) && IsZero(mat.get(i, i + pos))) {
	    pos++;
	}
	if (i + pos >= cols) {
	    break;
	}
	//cout << "mat[i] = " << mat[i] << endl;
	for (j = 0; j < i; j++) {
	    //cout << "mat[j] = " << mat[j] << endl;
	    if (IsOne(mat.get(j, i + pos))) {
		mat[j] += mat[i];
		//cout << "new mat[j] = " << mat[j] << endl;
	    }
	}
    }
    return rank;
}

void readFile(GF2X& poly, FILE *fp) {
    char c;
    unsigned int j = 0;

    while ((c = getc(fp)) != EOF) {
	if (c < ' ') {
	    continue;
	} else if (c == '1') {
	    SetCoeff(poly, j, 1);
	    j++;
	} else if (c == '0') {
	    SetCoeff(poly, j, 0);
	    j++;
	} else {
	    break;
	}
    }
}

unsigned int get_uint(char *line, int radix) {
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

uint64_t get_uint64(char *line, int radix) {
    int i;
    unsigned int x;
    uint64_t result;

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