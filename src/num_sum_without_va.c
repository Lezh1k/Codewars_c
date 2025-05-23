#include "num_sum_without_va.h"
#include <sys/syscall.h>
#include <unistd.h>

int n_sum(int n, ...) {
  enum { RDI = 0, RSI, RDX, RCX, R8, R9 };
  int regs[6] = {0};
  int *args;
  __asm__ __volatile__("lea 0x8(%%rbp), %0;" : "=r"(args)::);
  __asm__ __volatile__("mov %%edi, %0;" : "=m"(regs[RDI])::);
  __asm__ __volatile__("mov %%esi, %0;" : "=m"(regs[RSI])::);
  __asm__ __volatile__("mov %%edx, %0;" : "=m"(regs[RDX])::);
  __asm__ __volatile__("mov %%ecx, %0;" : "=m"(regs[RCX])::);
  __asm__ __volatile__("mov %%r8d, %0;" : "=m"(regs[R8])::);
  __asm__ __volatile__("mov %%r9d, %0;" : "=m"(regs[R9])::);
  int sum = 0;
  if (n <= 5) {
    for (int i = 1; i <= n; ++i)
      sum += regs[i];
    return (int)sum;
  }

  args += 2;
  for (int i = 1; i <= 5; ++i)
    sum += regs[i];
  for (int i = 0; i < n - 5; ++i)
    sum += args[i * 2];
  return sum;
}
