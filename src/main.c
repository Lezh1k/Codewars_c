#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

static void initFromFile(const char *path);
static void calculate(double epoch, double alt, double glat, double glon,
                      double time, double *dec, double *dip,
                      double *ti, double *gv);

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;
  static const char *path = "/home/lezh1k/SRC/WMM2015LegacyC/WMM.COF";
  double dec, dip, ti, gv;

  initFromFile(path);
  calculate(2015.0, 0.0, 42.879965, 74.617977, 2017.0, &dec, &dip, &ti, &gv);

  printf("dec : %f\n", dec);
}
///////////////////////////////////////////////////////

#define maxord 12

static double sp[13];
static double cp[13];
static double fn[13];
static double fm[13];
static double pp[13];

static double k[13][13];
static double c[13][13];
static double cd[13][13];
static double tc[13][13];
static double dp[13][13];
static double snorm2d[13][13];

void initFromFile(const char *path) {
  FILE *wmmdat;
  double epoch;
  int n, m, j, D1, D2;
  double gnm, hnm, dgnm, dhnm, flnmj;

  static char model[20], c_str[81], c_new[5];

  wmmdat = fopen(path, "r");
  if (wmmdat == NULL) {
    fprintf(stderr, "Error opening model file \n");
    exit(1);
  }

  /* INITIALIZE CONSTANTS */
  sp[0] = 0.0;
  cp[0] = pp[0] = 1.0;
  snorm2d[0][0] = 1.0;
  dp[0][0] = 0.0;

  /* READ WORLD MAGNETIC MODEL SPHERICAL HARMONIC COEFFICIENTS */
  c[0][0] = 0.0;
  cd[0][0] = 0.0;

  fgets(c_str, 80, wmmdat);
  if (sscanf(c_str,"%lf%s", &epoch, model) < 2) {
    fprintf(stderr, "Invalid header in model file WMM.COF\n");
    exit(1);
  }

  while (1) {
    if (fgets(c_str, 80, wmmdat) == NULL)
      break;

    /* CHECK FOR LAST LINE IN FILE */
    for (int i=0; i<4 && (c_str[i] != '\0'); i++) {
      c_new[i] = c_str[i];
      c_new[i+1] = '\0';
    }

    if (strcmp("9999", c_new) == 0)
      break;

    /* END OF FILE NOT ENCOUNTERED, GET VALUES */
    sscanf(c_str,"%d%d%lf%lf%lf%lf",&n,&m,&gnm,&hnm,&dgnm,&dhnm);

    if (n > maxord)
      break;

    if (m > n || m < 0.0) {
      fprintf(stderr, "Corrupt record in model file WMM.COF\n");
      exit(1);
    }

    if (m <= n) {
      c[m][n] = gnm;
      cd[m][n] = dgnm;
      if (m != 0) {
        c[n][m-1] = hnm;
        cd[n][m-1] = dhnm;
      }
    } //if (m <= n)
  } //while true

  /* CONVERT SCHMIDT NORMALIZED GAUSS COEFFICIENTS TO UNNORMALIZED */
  snorm2d[0][0] = 1.0;
  fm[0] = 0.0;
  for (n=1; n<=maxord; n++) {
    snorm2d[0][n] = snorm2d[0][n-1] * (double)(2*n-1)/(double)n;
    j = 2;
    for (m=0,D1=1,D2=(n-m+D1)/D1; D2>0; D2--,m+=D1) {
      k[m][n] = (double)(((n-1)*(n-1))-(m*m))/(double)((2*n-1)*(2*n-3));
      if (m > 0) {
        flnmj = (double)((n-m+1)*j)/(double)(n+m);
        snorm2d[m][n] = snorm2d[m-1][n]*sqrt(flnmj);
        j = 1;

        c[n][m-1] *= snorm2d[m][n];
        cd[n][m-1] *= snorm2d[m][n];
      }
      c[m][n] *= snorm2d[m][n];
      cd[m][n] *= snorm2d[m][n];
    }
    fn[n] = (double)(n+1);
    fm[n] = (double)n;
  }
  k[1][1] = 0.0;

  fclose(wmmdat);
  return;
}
///////////////////////////////////////////////////////

void calculate(double epoch, double alt, double glat, double glon,
               double time, double *dec, double *dip,
               double *ti, double *gv) {

  //todo move to defines
#define a   6378.137
#define b   6356.7523142
#define re  6371.2
#define a2  (a*a)
#define b2  (b*b)
#define c2  (a2-b2)
#define a4  (a2*a2)
#define b4  (b2*b2)
#define c4  (a4-b4)
#define pi M_PI
#define dtr (pi/180.0)


  static int n, m, D3, D4;
  double dt,
      rlon, rlat,
      srlon, srlat,
      crlon, crlat,
      srlat2, crlat2,
      q, q1, q2, ct, st, r2, r,
      d, ca, sa, aor, ar, br, bt,
      bp, bpp, par, temp1, temp2,
      parp, bx, by, bz, bh;

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
  q = sqrt(a2-c2*srlat2);
  q1 = alt*q;
  q2 = ((q1+a2)/(q1+b2))*((q1+a2)/(q1+b2));
  ct = srlat/sqrt(q2*crlat2+srlat2);
  st = sqrt(1.0-(ct*ct));
  r2 = (alt*alt)+2.0*q1+(a4-c4*srlat2)/(q*q);
  r = sqrt(r2);
  d = sqrt(a2*crlat2+b2*srlat2);
  ca = (alt+d)/r;
  sa = c2*crlat*srlat/(r*d);

  for (m=2; m<=maxord; m++) {
    sp[m] = sp[1]*cp[m-1]+cp[1]*sp[m-1];
    cp[m] = cp[1]*cp[m-1]-sp[1]*sp[m-1];
  }

  aor = re/r;
  ar = aor*aor;
  br = bt = bp = bpp = 0.0;
  for (n=1; n<=maxord; n++) {
    ar = ar*aor;
    for (m=0, D3=1 , D4=(n+m+D3)/D3; D4>0; D4--, m+=D3) {
      /*
   COMPUTE UNNORMALIZED ASSOCIATED LEGENDRE POLYNOMIALS
   AND DERIVATIVES VIA RECURSION RELATIONS
*/
      do {
        if (n == m) {
          snorm2d[m][n] = st * snorm2d[m-1][n-1];
          dp[m][n] = st * dp[m-1][n-1] + ct * snorm2d[m-1][n-1];
          break;
        }

        if (n == 1 && m == 0) {
          snorm2d[m][n] = ct * snorm2d[m][n-1];
          dp[m][n] = ct*dp[m][n-1] - st*snorm2d[m][n-1];
          break;
        }

        if (n > 1 && n != m) {
          if (m > n-2) snorm2d[m][n-2] = 0.0;
          if (m > n-2) dp[m][n-2] = 0.0;
          snorm2d[m][n] = ct * snorm2d[m][n-1] - k[m][n]*snorm2d[m][n-2];
          dp[m][n] = ct*dp[m][n-1] - st*snorm2d[m][n-1] - k[m][n]*dp[m][n-2];
        }
      } while(0);

      /*
    TIME ADJUST THE GAUSS COEFFICIENTS
*/

      tc[m][n] = c[m][n]+dt*cd[m][n];
      if (m != 0)
        tc[n][m-1] = c[n][m-1]+dt*cd[n][m-1];
      /*
    ACCUMULATE TERMS OF THE SPHERICAL HARMONIC EXPANSIONS
*/
      par = ar * snorm2d[m][n];
      if (m == 0) {
        temp1 = tc[m][n]*cp[m];
        temp2 = tc[m][n]*sp[m];
      } else {
        temp1 = tc[m][n]*cp[m]+tc[n][m-1]*sp[m];
        temp2 = tc[m][n]*sp[m]-tc[n][m-1]*cp[m];
      }

      bt = bt-ar*temp1*dp[m][n];
      bp += (fm[m]*temp2*par);
      br += (fn[n]*temp1*par);
      /*
    SPECIAL CASE:  NORTH/SOUTH GEOGRAPHIC POLES
*/
      if (st == 0.0 && m == 1) {
        if (n == 1)
          pp[n] = pp[n-1];
        else
          pp[n] = ct*pp[n-1] - k[m][n]*pp[n-2];
        parp = ar*pp[n];
        bpp += (fm[m]*temp2*parp);
      }
    }
  }
  if (st == 0.0) bp = bpp;
  else bp /= st;
  /*
    ROTATE MAGNETIC VECTOR COMPONENTS FROM SPHERICAL TO
    GEODETIC COORDINATES
*/
  bx = -bt*ca-br*sa;
  by = bp;
  bz = bt*sa-br*ca;
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
  if (fabs(glat) >= 55.)
  {
    if (glat > 0.0 && glon >= 0.0) *gv = *dec-glon;
    if (glat > 0.0 && glon < 0.0) *gv = *dec+fabs(glon);
    if (glat < 0.0 && glon >= 0.0) *gv = *dec+glon;
    if (glat < 0.0 && glon < 0.0) *gv = *dec-fabs(glon);
    if (*gv > +180.0) *gv -= 360.0;
    if (*gv < -180.0) *gv += 360.0;
  }

  return;
}
///////////////////////////////////////////////////////
