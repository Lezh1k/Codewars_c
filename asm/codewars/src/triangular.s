global triangular

section .text

; int triangular(int n);
; n --> edi, result --> eax
; Tn = (n*n + n) / 2
triangular:
  test edi, edi
  jnz __tr_continue
  xor rax, rax
  ret
__tr_continue:
  mov rax, rdi
  mov rcx, rdi
  mul rcx
  add rax, rdi
  shr rax, 1
  ret
; ----------------------------------------

