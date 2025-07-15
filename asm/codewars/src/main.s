global _start
extern triangular, is_leap_year, is_isogram
extern rot_13, rot_13_sse, is_prime, printf

section .data
  print_is_prime_fmt: db  "%d", 0x0a, 0x00
  print_rot_13_fmt: db  "%s", 0x0a, 0x00
  ; rot_13_tst_data: db "12345 abcdefg hijklmnop qrstuvw xyz ABCDEFG HIJKLMNOP QRSTUVW XYZ !%!#$", 0x00
  rot_13_tst_data: db "12345 abcdefghijklmnopqrstuvwxyz ABCDEFGHIJKLMNOPQRSTUVWXYZ 54321 !!!", 0x00

section .text

_start:
  mov rdi, rot_13_tst_data 
  call rot_13
  mov rdi, print_rot_13_fmt
  mov rsi, rax
  call printf

  mov rdi, rot_13_tst_data
  call rot_13_sse
  mov rdi, print_rot_13_fmt
  mov rsi, rax
  call printf

  call _exit
; ----------------------------------------

_exit:
  mov  rax, 60
  mov  rdi, 0
  syscall
; ----------------------------------------
