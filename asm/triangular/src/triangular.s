global triangular, _start

section .data
  msg db "Helo! Warkd"

section .text

_start:
  mov rdi, -9
  call triangular

  mov rdi, msg
  call is_isogram
  push rax  ; for debug

  call _exit
; ----------------------------------------

; The pointer to the string is passed on with RDI
is_isogram:
  mov ecx, 0x80000000 ; sign bit sentinel
  xor eax, eax
__is_isogram_loop:
  movzx edx, byte [rdi]
  inc rdi
  and dl, ~0x20
  sub dl, 'A' ; if dl was 0 dl - 'A' gives -65 = 0xBF. and bit 31 is already set
  bts ecx, edx
  jnc __is_isogram_loop
  sets al
__is_isogram_loop_end:
  ret


; ----------------------------------------

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

_exit:
  mov  rax, 60
  mov  rdi, 0
  syscall
; ----------------------------------------
