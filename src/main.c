#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <float.h>
#include <time.h>

#include "skyscapers.h"

//#define SIZE 6

//int check(int **solution, int (*expected)[SIZE]) {
//  bool result = false;
//  if (solution && expected) {
//    result = true;
//    for (int i = 0; i < SIZE; i++)
//      if (memcmp(solution[i], expected[i], SIZE * sizeof(int))) {
//        return false;
//      }
//  }
//  return result;
//}
///////////////////////////////////////////////////////////

//int main(int argc, char **argv) {
//  (void)argc;
//  (void)argv;
//  setbuf(stdout, NULL);

//  static const int clues[][SIZE*4] = {
//    {
//      3, 2, 2, 3, 2, 1,
//      1, 2, 3, 3, 2, 2,
//      5, 1, 2, 2, 4, 3,
//      3, 2, 1, 2, 2, 4
//    },
//    {
//      0, 0, 0, 2, 2, 0,
//      0, 0, 0, 6, 3, 0,
//      0, 4, 0, 0, 0, 0,
//      4, 4, 0, 3, 0, 0
//    }
//  };

//  static int expected[][SIZE][SIZE] = {
//    { { 2, 1, 4, 3, 5, 6 },
//      { 1, 6, 3, 2, 4, 5 },
//      { 4, 3, 6, 5, 1, 2 },
//      { 6, 5, 2, 1, 3, 4 },
//      { 5, 4, 1, 6, 2, 3 },
//      { 3, 2, 5, 4, 6, 1 } },
//    { { 5, 6, 1, 4, 3, 2 },
//      { 4, 1, 3, 2, 6, 5 },
//      { 2, 3, 6, 1, 5, 4 },
//      { 6, 5, 4, 3, 2, 1 },
//      { 1, 2, 5, 6, 4, 3 },
//      { 3, 4, 2, 5, 1, 6 } }
//  };

//  if (!check(SolvePuzzle(clues[0]), expected[0]))
//    return -1;
//  if (!check(SolvePuzzle(clues[1]), expected[1]))
//    return -2;

//  return 0;
//}
/////////////////////////////////////////////////////////

#define pi 3.14159265359
#define dtr pi/180.0

#define a   6378.137
#define b   6356.7523142
#define re  6371.2

#define a2  (a*a)
#define b2  (b*b)
#define c2  (a2-b2)
#define a4  (a2*a2)
#define b4  (b2*b2)
#define c4  (a4-b4)

//be careful with this.
#define epoch 2015.0
#define maxord 12

static double sp[13];
static double cp[13];
static double pp[13];
static double fn[13];
static double fm[13];

static double c[13][13];
static double cd[13][13];
static double k[13][13];
static double snorm[13][13];

static void initFromFile() {
  int i, icomp, n, m, j, D1, D2;
  double gnm, hnm, dgnm, dhnm, flnmj;
  static char c_str[81], c_new[5];
  FILE *wmmdat;
  wmmdat = fopen("/home/lezh1k/SRC/WMM2015LegacyC/WMM.COF","r");
  if (wmmdat == NULL) {
    fprintf(stderr, "Error opening model file WMM.COF\n");
    exit(1);
  }

  /* INITIALIZE CONSTANTS */
  sp[0] = 0.0;
  cp[0] = pp[0] = 1.0;

  /* READ WORLD MAGNETIC MODEL SPHERICAL HARMONIC COEFFICIENTS */
  c[0][0] = 0.0;
  cd[0][0] = 0.0;

  fgets(c_str, 80, wmmdat); //read first line and do nothing!!!

  while (true) {
    if (fgets(c_str, 80, wmmdat) == NULL)
      break;

    /* CHECK FOR LAST LINE IN FILE */
    for (i=0; i<4 && (c_str[i] != '\0'); i++) {
      c_new[i] = c_str[i];
      c_new[i+1] = '\0';
    }

    icomp = strcmp("9999", c_new);
    if (icomp == 0)
      break;

    /* END OF FILE NOT ENCOUNTERED, GET VALUES */
    sscanf(c_str,"%d%d%lf%lf%lf%lf",&n,&m,&gnm,&hnm,&dgnm,&dhnm);

    if (n > maxord)
      break;

    if (m > n || m < 0.0) {
      fprintf(stderr, "Corrupt record in model file WMM.COF\n");
      exit(1);
    }

    if (m > n) continue;

    c[m][n] = gnm;
    cd[m][n] = dgnm;
    if (m != 0) {
      c[n][m-1] = hnm;
      cd[n][m-1] = dhnm;
    }
  }
  /* CONVERT SCHMIDT NORMALIZED GAUSS COEFFICIENTS TO UNNORMALIZED */

  snorm[0][0] = 1.0;

  fm[0] = 0.0;
  for (n=1; n<=maxord; n++) {

    snorm[0][n] = snorm[0][n-1] * (double)(2*n-1)/(double)n;
    j = 2;

    for (m=0, D1=1, D2=(n-m+D1)/D1; D2>0; D2--,m+=D1) {
      k[m][n] = (double)(((n-1)*(n-1))-(m*m))/(double)((2*n-1)*(2*n-3));

      if (m > 0) {
        flnmj = (double)((n-m+1)*j) / (double)(n+m);
        snorm[m][n] = snorm[m-1][n] * sqrt(flnmj);
        j = 1;
        c[n][m-1] *= snorm[m][n];
        cd[n][m-1] *= snorm[m][n];
      }
      c[m][n] *= snorm[m][n];
      cd[m][n] *= snorm[m][n];
    }
    fn[n] = (double)(n+1);
    fm[n] = (double)n;
  }
  k[1][1] = 0.0;
  fclose(wmmdat);
  return;
  /*************************************************************************/
}
///////////////////////////////////////////////////////

static void computeMagneticFieldParameters(double alt,
                                           double glat,
                                           double glon,
                                           double time,
                                           double *dec,
                                           double *dip,
                                           double *ti,
                                           double *gv)
{
  static int n, m, D3, D4;
  double tc[13][13],
      dt, rlon, rlat, srlon, srlat,
      crlon, crlat, srlat2, crlat2,
      q, q1, q2, ct, st, r2, r, d, ca, sa,
      aor, ar, br, bt, bp, bpp,
      par, temp1, temp2, parp, bx, by, bz, bh;

  double dp[13][13];
  dp[0][0] = 0.0;

  dt = time - epoch;
  rlon = glon*dtr;
  rlat = glat*dtr;
  srlon = sin(rlon);
  srlat = sin(rlat);
  crlon = cos(rlon);
  crlat = cos(rlat);
  srlat2 = srlat*srlat;
  crlat2 = crlat*crlat;
  sp[1] = srlon;
  cp[1] = crlon;

  /* CONVERT FROM GEODETIC COORDS. TO SPHERICAL COORDS. */
  q = sqrt(a2 - c2*srlat2);
  q1 = alt*q;
  q2 = ((q1+a2) / (q1+b2)) * ((q1+a2) / (q1+b2));
  ct = srlat / sqrt(q2*crlat2 + srlat2);
  st = sqrt(1.0 - ct*ct);
  r2 = (alt*alt) + 2.0*q1 + (a4 - c4*srlat2) / (q*q);
  r = sqrt(r2);
  d = sqrt(a2*crlat2 + b2*srlat2);
  ca = (alt + d) / r;
  sa = c2*crlat*srlat/(r*d);

  for (m=2; m<=maxord; m++) {
    sp[m] = sp[1]*cp[m-1] + cp[1]*sp[m-1];
    cp[m] = cp[1]*cp[m-1] - sp[1]*sp[m-1];
  }

  aor = re / r;
  ar = aor*aor;
  br = bt = bp = bpp = 0.0;
  for (n=1; n<=maxord; n++) {
    ar = ar*aor;
    for (m=0,D3=1,D4=(n+m+D3)/D3; D4>0; D4--,m+=D3) {
      /* COMPUTE UNNORMALIZED ASSOCIATED LEGENDRE POLYNOMIALS
     AND DERIVATIVES VIA RECURSION RELATIONS */

      do {
        if (n == m) {
          snorm[m][n] = st*snorm[m-1][n-1];
          dp[m][n] = st*dp[m-1][n-1] + ct*snorm[m-1][n-1];
          break;
        }

        if (n == 1 && m == 0) {
          snorm[m][n] = ct*snorm[m][n-1];
          dp[m][n] = ct*dp[m][n-1]-st*snorm[m][n-1];
          break;
        }

        if (n > 1 && n != m) {
          if (m > n-2)
            snorm[m][n-2] = 0.0;
          if (m > n-2)
            dp[m][n-2] = 0.0;
          snorm[m][n] = ct*snorm[m][n-1] - k[m][n]*snorm[m][n-2];
          dp[m][n] = ct*dp[m][n-1] - st*snorm[m][n-1] - k[m][n]*dp[m][n-2];
        }
      } while (0);

      /*
      TIME ADJUST THE GAUSS COEFFICIENTS
  */
      tc[m][n] = c[m][n] + dt*cd[m][n];
      if (m != 0)
        tc[n][m-1] = c[n][m-1] + dt*cd[n][m-1];
      /*
      ACCUMULATE TERMS OF THE SPHERICAL HARMONIC EXPANSIONS
  */
      par = ar * snorm[m][n];
      if (m == 0) {
        temp1 = tc[m][n]*cp[m];
        temp2 = tc[m][n]*sp[m];
      } else {
        temp1 = tc[m][n]*cp[m]+tc[n][m-1]*sp[m];
        temp2 = tc[m][n]*sp[m]-tc[n][m-1]*cp[m];
      }
      bt = bt - ar * temp1 * dp[m][n];
      bp += (fm[m] * temp2 * par);
      br += (fn[n] * temp1 * par);

      /*
      SPECIAL CASE:  NORTH/SOUTH GEOGRAPHIC POLES
  */
      if (st == 0.0 && m == 1) {
        if (n == 1) pp[n] = pp[n-1];
        else pp[n] = ct*pp[n-1]-k[m][n]*pp[n-2];
        parp = ar*pp[n];
        bpp += (fm[m]*temp2*parp);
      }
    } //for (m=0,D3=1,D4=(n+m+D3)/D3; D4>0; D4--,m+=D3)
  } //for (n=1; n<=maxord; n++)

  if (st == 0.0) bp = bpp;
  else bp /= st;

  /*
      ROTATE MAGNETIC VECTOR COMPONENTS FROM SPHERICAL TO
      GEODETIC COORDINATES
  */
  bx = -bt*ca - br*sa;
  by = bp;
  bz = bt*sa - br*ca;

  /*
      COMPUTE DECLINATION (DEC), INCLINATION (DIP) AND
      TOTAL INTENSITY (TI)
  */

  bh = sqrt((bx*bx)+(by*by));
  *ti = sqrt((bh*bh)+(bz*bz));
  *dec = atan2(by,bx)/dtr;
  *dip = atan2(bz,bh)/dtr;
  /*
      COMPUTE MAGNETIC GRID VARIATION IF THE CURRENT
      GEODETIC POSITION IS IN THE ARCTIC OR ANTARCTIC
      (I.E. GLAT > +55 DEGREES OR GLAT < -55 DEGREES)

      OTHERWISE, SET MAGNETIC GRID VARIATION TO -999.0
  */
  *gv = -999.0;
  if (fabs(glat) >= 55.0) {
    if (glat > 0.0 && glon >= 0.0) *gv = *dec-glon;
    if (glat > 0.0 && glon < 0.0) *gv = *dec+fabs(glon);
    if (glat < 0.0 && glon >= 0.0) *gv = *dec+glon;
    if (glat < 0.0 && glon < 0.0) *gv = *dec-fabs(glon);
    if (*gv > +180.0) *gv -= 360.0;
    if (*gv < -180.0) *gv += 360.0;
  }
  return;
}
/*************************************************************************/

static void printArr1D(const char *descr, const double *arr, int len) {
  const double *tmp = arr;
  printf("%s: ", descr);
  while (len--) {
    printf("%f, ", *tmp++);
  }
  printf("\n----------\n");
}
///////////////////////////////////////////////////////

static void printArr2D(const char *descr, const double mtx[][13], int w, int h) {
  printf("%s: ", descr);
  for (int y = 0; y < h; ++y) {
    printf("\n");
    for (int x = 0; x < w; ++x) {
      printf("%f, ", mtx[y][x]);
    }
  }
  printf("\n----------\n");
}
///////////////////////////////////////////////////////

static void printArraysAfterInit() {
  const double *arrays1D[] = {sp, cp, pp, fn, fm, NULL};
  const char *descriptions1D[] = {"sp", "cp", "pp", "fn", "fm", NULL};
  const double **tmpArr1D = arrays1D;
  const char **descr1D = descriptions1D;

  const char *descriptions2D[] = {"c", "cd", "k", NULL};
  const char **descr2D = descriptions2D;

  initFromFile();
  for (; *tmpArr1D; ++tmpArr1D, ++descr1D) {
    printArr1D(*descr1D, *tmpArr1D, 13);
  }
  ///////////////////////////////////////////////////////

  printArr2D(*descr2D++, c, 13, 13);
  printArr2D(*descr2D++, cd, 13, 13);
  printArr2D(*descr2D++, k, 13, 13);
}

static bool fcmp(double l, double r) {
  return fabs(l-r) <= DBL_EPSILON;
}

int main(int argc, char **argv) {
  (void) argc;
  (void) argv;
  //declination, inclination, total intensity, grid variation
  double dec, dip, ti, gv;
  initFromFile();
  computeMagneticFieldParameters(0.0, 42.879965, 74.617977, 2017.0, &dec, &dip, &ti, &gv);

  printf("dec : %f\n", dec);
  printf("dip : %f\n", dip);
  printf("ti  : %f\n", ti);
  printf("gv  : %f\n", gv);
}

