.global umul_32
.global umul_64

.text

umul_32:
# edi, esi, edx, ecx
  movl %edx, %eax
  mull %ecx
  movl %eax, (%rsi)
  movl %edx, (%rdi)
  ret

umul_64:
# rdi, rsi, rdx, rcx
  movq %rdx, %rax
  mulq %rcx
  movq %rax, (%rsi)
  movq %rdx, (%rdi)
  ret

