#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

int count(int sum,
          int *coins) {
  int *ways = calloc(sum+1, sizeof(int));
  ways[0] = 1;
  for (int *coin = coins; *coin; ++coin) {
    for (int i = 0; i < sum + 1; ++i) {
      if (*coin > i) continue;
      ways[i] += ways[i - *coin];
    }
  }
  int res = ways[sum];
  free(ways);
  return res;
}

// simple recursive method; slow
int count2(int sum,
           int *coins) {
  if (!*coins || sum < 0) return 0;
  if (!sum) return 1;
  return count2(sum - *coins, coins) + count2(sum, coins + 1);
}
//////////////////////////////////////////////////////////////

//int main(int argc, char *argv[]) {
//  (void) argc;
//  (void) argv;

//  int eu_coins[] = { 5,10,20,50,100,200,500,0 };
//  int ko_coins[] = { 1,2,5439,7927,12201,14112,16454,16466,19703,0};

//  printf("%d\n", count(300, eu_coins));
//  printf("%d\n", count( 36071, ko_coins));

//  return 0;
//}
