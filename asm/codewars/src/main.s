global _start
extern triangular, is_leap_year, is_isogram, is_prime, printf

section .data
  print_is_prime_fmt: db  "%d", 0x0a, 0x00

section .text

_start:
  mov rdi, 1
  call is_prime

  mov rdi, print_is_prime_fmt
  mov rsi, rax
  call printf

  call _exit
; ----------------------------------------

_exit:
  mov  rax, 60
  mov  rdi, 0
  syscall
; ----------------------------------------
