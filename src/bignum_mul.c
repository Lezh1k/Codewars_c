#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <stdbool.h>
#include <math.h>

typedef struct iVec {
  int32_t len;
  double *r; //LSB!!!
  double *i;
} iVec_t;

static uint32_t nearest2pow(uint32_t v);
static uint32_t logOfPower2(uint32_t v);

static void vecPrint(const iVec_t *vec);
static iVec_t vecFromStr(const char *str);
static void vecComplement(iVec_t *a, iVec_t *b);
static void vecNormalize(iVec_t *vec);
static bool fft(double *Rdat, double *Idat, int N, int LogN, bool inv);

#define MAX_LOG2_N 32

static double RCoef[MAX_LOG2_N] = {
  -1.00000000000000000000000000, 0.00000000000000006123233996, 0.70710678118654757273731093,
  0.92387953251128673848313611, 0.98078528040323043057924224, 0.99518472667219692873175063,
  0.99879545620517240500646494, 0.99969881869620424996725205, 0.99992470183914450299056398,
  0.99998117528260110908888691, 0.99999529380957619117964441, 0.99999882345170187925020855,
  0.99999970586288222662574299, 0.99999992646571789212117665, 0.99999998161642933425241608,
  0.99999999540410733356310402, 0.99999999885102686114635162, 0.99999999971275665977543667,
  0.99999999992818922045501040, 0.99999999998204724960260137, 0.99999999999551181240065034,
  0.99999999999887800861131382, 0.99999999999971944664167722, 0.99999999999992983390484369,
  0.99999999999998245847621092, 0.99999999999999567013020396, 0.99999999999999888977697537,
  0.99999999999999977795539507, 0.99999999999999988897769754, 1.00000000000000000000000000,
  1.00000000000000000000000000, 1.00000000000000000000000000
};

static double ICoef[MAX_LOG2_N] = {
  0.00000000000000012246467991, 1.00000000000000000000000000, 0.70710678118654746171500847,
  0.38268343236508978177923268, 0.19509032201612824808378832, 0.09801714032956060362877793,
  0.04906767432741801493456535, 0.02454122852291228812360302, 0.01227153828571992538742919,
  0.00613588464915447526909498, 0.00306795676296597614324257, 0.00153398018628476550014039,
  0.00076699031874270448549957, 0.00038349518757139556320718, 0.00019174759731070329152845,
  0.00009587379909597734466923, 0.00004793689960306688130961, 0.00002396844980841821931778,
  0.00001198422490506970529769, 0.00000599211245264242752721, 0.00000299605622633466083516,
  0.00000149802811316901114271, 0.00000074901405658471574140, 0.00000037450702829238412872,
  0.00000018725351414619534661, 0.00000009362675707309808359, 0.00000004681337853654908812,
  0.00000002340668926827455068, 0.00000001170334463413727699, 0.00000000585167231706863850,
  0.00000000292583615853431925, 0.00000000146291807926715962
};
//////////////////////////////////////////////////////////////////////////

#define  NUMBER_IS_2_POW_K(x)   ((!((x)&((x)-1)))&&((x)>1))  // x is pow(2, k), k=1,2, ...

bool fft(double *Rdat, double *Idat, int N, int LogN, bool inv) {
  // parameters error check:
  assert(Rdat);
  assert(Idat);
  assert(NUMBER_IS_2_POW_K(N));
  assert(LogN >= 2 && LogN <= MAX_LOG2_N);

  register int  i, j, n, k, io, ie, in, nn;
  double        ru, iu, rtp, itp, rtq, itq, rw, iw, sr;

  nn = N >> 1;
  ie = N; //N для каждого из уровней. для первого N. Для второго N/2 и т.д.
  for(n=1; n<=LogN; n++) {
    //rw и iw - поворотный коэффициент для LogN.
    //w = -2.0 * M_PI / pow(2.0, n)
    rw = RCoef[LogN - n]; //cos(w)
    iw = ICoef[LogN - n]; //sin(w)

    if(inv)
      iw = -iw; //комплексно сопряженное.

    in = ie >> 1; //in - середина... N/2 для каждого уровня.

    //ru и iu - используемый поворотный коэффициент
    ru = 1.0;
    iu = 0.0;

    for(j=0; j<in; j++) {
      for(i=j; i<N; i+=ie) {
        io       = i + in; //io = i + N/2 для каждого из уровней...

        //s(2k) = s0(k) + s1(k)
        rtp      = Rdat[i]  + Rdat[io];
        itp      = Idat[i]  + Idat[io];

        //s0(k) - s1(k)
        rtq      = Rdat[i]  - Rdat[io];
        itq      = Idat[i]  - Idat[io];

        //s(2k+1) = Wkn * (s0(k) - s1(k))
        Rdat[io] = rtq * ru - itq * iu;
        Idat[io] = itq * ru + rtq * iu;

        Rdat[i]  = rtp;
        Idat[i]  = itp;
      }

      sr = ru;
      //Wk+1 = W*Wk
      ru = ru * rw - iu * iw;
      iu = iu * rw + sr * iw;
    }

    ie >>= 1;
  }
  //граф бабочка для всех уровней здесь уже завершен. дальше должна быть перестановка.

  //nn = N/2
  //bit-reversal permutation :)
  for(j=i=1; i<N; i++) {

    if(i < j) {
      //swap dat[io] and dat[in].
      io       = i - 1;
      in       = j - 1;
      rtp      = Rdat[in];
      itp      = Idat[in];
      Rdat[in] = Rdat[io];
      Idat[in] = Idat[io];
      Rdat[io] = rtp;
      Idat[io] = itp;
    }

    k = nn;

    while(k < j) {
      j   = j - k;
      k >>= 1;
    }

    j = j + k;
  }
  //перестановка завершена

  if(!inv)
    return true;

  rw = 1.0 / N;
  for(i=0; i<N; i++) {
    Rdat[i] *= rw;
    Idat[i] *= rw;
  }
  return true;
}
//////////////////////////////////////////////////////////////////////////

/* Let two polynomials A and B be given. We calculate the DFT for each of them: DFT (A) and DFT (B) are two vector-valued polynomials.
Now, what happens when multiplying polynomials? Obviously, at each point their values ​​are simply multiplied, i.e.

 (A * B) (x) = A (x) * B (x).

But this means that if we multiply the vectors DFT (A) and DFT (B) by simply multiplying each element
one vector by the corresponding element of another vector, then we obtain nothing more than the DFT of the polynomial A * B:

 DFT (A * B) = DFT (A) * DFT (B)

Finally, applying the inverse DFT, we obtain:

 A * B = InverseDFT (DFT (A) * DFT (B))

where, we repeat, on the right, under the product of two DFTs, we mean pairwise products of vector elements.
Such a product obviously requires only O (n) operations to be computed.
Thus, if we learn how to compute the DFT and the inverse DFT in the time O (n \ log n),
then we can find the product of two polynomials (and, consequently, two long numbers) for the same asymptotics.

It should be noted that, first, two polynomials should be reduced to the same degree (simply adding the coefficients of one of them with zeros).
Second, as a result of the product of two polynomials of degree n, we obtain a polynomial of degree 2n-1, so that the result is correct,
First it is necessary to double the degrees of each polynomial (again, supplementing them with zero coefficients).
*/


uint32_t nearest2pow(uint32_t v) {
  --v;
  v |= v >> 1;
  v |= v >> 2;
  v |= v >> 4;
  v |= v >> 8;
  v |= v >> 16;
  return ++v;
}
//////////////////////////////////////////////////////////////////////////

uint32_t logOfPower2(uint32_t v) {
  uint32_t c = 32; // c will be the number of zero bits on the right
  v &= -(int32_t)v;
  if (v) --c;
  if (v & 0x0000FFFF) c -= 16;
  if (v & 0x00FF00FF) c -= 8;
  if (v & 0x0F0F0F0F) c -= 4;
  if (v & 0x33333333) c -= 2;
  if (v & 0x55555555) c -= 1;
  return c;
}
//////////////////////////////////////////////////////////////////////////

void vecPrint(const iVec_t *vec) {
  int i;
  for (i = 0; i < vec->len; ++i)
    printf("%.4f ", vec->r[i]);
  printf("\n");
}
//////////////////////////////////////////////////////////////////////////

iVec_t vecFromStr(const char *str) {
  iVec_t res;
  int i, sl;
  sl = strlen(str);
  res.len = nearest2pow(sl+1) << 1;
  res.r = malloc(res.len * sizeof(double));
  res.i = malloc(res.len * sizeof(double));

  for (i = 0; i < res.len - sl; ++i) {
    res.r[i] = 0.0;
    res.i[i] = 0.0;
  }

  for (; *str; ++str, ++i) {
    res.r[i] = *str-'0';
    res.i[i] = 0.0;
  }

  return res;
}
//////////////////////////////////////////////////////////////////////////

void vecComplement(iVec_t *a, iVec_t *b) {
  iVec_t *vtn[2] = {a, b};
  double *rdat, *idat;
  int ge, le;

  if (a->len == b->len)
    return;

  ge = a->len < b->len;
  le = 1 - ge;

  rdat = malloc(vtn[ge]->len * sizeof(double));
  idat = malloc(vtn[ge]->len * sizeof(double));

  memcpy(rdat + vtn[ge]->len - vtn[le]->len, vtn[le]->r, vtn[le]->len * sizeof(double));
  memcpy(idat + vtn[ge]->len - vtn[le]->len, vtn[le]->i, vtn[le]->len * sizeof(double));

  memset(rdat, 0, (vtn[ge]->len - vtn[le]->len)*sizeof(double));
  memset(idat, 0, (vtn[ge]->len - vtn[le]->len)*sizeof(double));

  free(vtn[le]->r);
  free(vtn[le]->i);

  vtn[le]->r = rdat;
  vtn[le]->i = idat;
  vtn[le]->len = vtn[ge]->len;
}
//////////////////////////////////////////////////////////////////////////

void vecNormalize(iVec_t *vec) {
  int32_t carry = 0;
  int32_t i;

  for (i = 0; i < vec->len; ++i) {
    vec->r[i] = (int32_t)(vec->r[i]+0.1); //HACK!!!!!!!
  }

  //I have no idea why result is shifted to left by 1
  for (i = vec->len-2; i >= 0; --i) {
    vec->r[i] += carry;
    carry = vec->r[i] / 10;
    vec->r[i] = (int32_t)vec->r[i] % 10;
  }
}

char *multiply(char *aStr, char *bStr) {
  iVec_t a, b;
  int i;
  char *res, *tmp;

  a = vecFromStr(aStr);
  b = vecFromStr(bStr);
  vecComplement(&a, &b);

  fft(a.r, a.i, a.len, logOfPower2(a.len), false);
  fft(b.r, b.i, b.len, logOfPower2(b.len), false);

  for (i = 0; i < a.len; ++i) {
    //a = a.r + a.i; b = b.r + b.i; ab = (a.r*b.r - a.i*b.i) + (a.r*b.i + b.r*a.i)
    double tmpr = a.r[i]*b.r[i] - a.i[i]*b.i[i];
    double tmpi = a.r[i]*b.i[i] + b.r[i]*a.i[i];
    a.r[i] = tmpr;
    a.i[i] = tmpi;
  }

  fft(a.r, a.i, a.len, logOfPower2(a.len), true);

  vecNormalize(&a);
  vecPrint(&a);

  res = malloc(a.len);
  tmp = res;
  for (i = 0; i < a.len-1; ++i) {
    if (a.r[i] == 0.0)
      continue;
    break;
  }

  if (i == a.len-1)
    *tmp++ = '0';

  for (; i < a.len-1; ++i, ++tmp) {
    *tmp = (int32_t)a.r[i] + '0';
  }

  *tmp = 0;

  return res;
}
//////////////////////////////////////////////////////////////////////////
