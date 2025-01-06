.intel_syntax noprefix

.global sequence_sum

.section .text

sequence_sum:
  // RDI - begin, RSI - end, RDX - step
  mov     ecx, edx
  mov     eax, esi
  xor     edx, edx
  cmp     edi, esi
  jg      .L1

  //; if (begin > end) {
  //;   return 0;
  //; }
  //; int n = (end - begin) / step;
  //; end = begin + step * n;
  //; int s = ((begin + end) / 2.) * (n+1);
  //; return s;
  sub     eax, edi
  pxor    xmm0, xmm0
  pxor    xmm1, xmm1
  cdq
  idiv    ecx
  imul    ecx, eax
  add     eax, 1
  cvtsi2sd        xmm1, eax
  lea     edx, [rcx+rdi*2]
  cvtsi2sd        xmm0, edx
  mulsd   xmm0, QWORD PTR .LC0[rip]
  mulsd   xmm0, xmm1
  cvttsd2si       edx, xmm0
.L1:
  mov     eax, edx
  ret
.LC0:
  .long   0
  .long   1071644672
