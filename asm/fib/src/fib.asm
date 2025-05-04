global  _start
extern printf, scanf

section .bss
  fib_input_buff resb 3

; ----------------------------------------
; ----------------------------------------

section .data
  
  msg_enter:         db  "Input fib N:", 0x0a
  msg_enter_len      equ $-msg_enter
  print_fib_out_fmt: db  "%d", 0x0a, 0x00
  scanf_fib_in_fmt:  db  "%d", 0x00
; ----------------------------------------
; ----------------------------------------

section .text

_start:
  call greeting
  call fib_get_input
  call init_fib
  call fib_loop
  call _exit
; ----------------------------------------

greeting:
  mov rax, 1
  mov rdi, 1
  mov rsi, msg_enter
  mov rdx, msg_enter_len
  syscall
  ret
; ----------------------------------------

fib_get_input:
  sub rsp, 8
  mov rdi, scanf_fib_in_fmt
  mov rsi, fib_input_buff
  call scanf
  add rsp, 8
; ----------------------------------------
  
init_fib:
  xor rax, rax    ; initialize rax to 0
  xor rbx, rbx    ; initialize rbx to 0
  inc rbx         ; increment rbx to 1
  ret
; ----------------------------------------

print_fib:
  push rax            
  push rbx

  mov rdi, print_fib_out_fmt  ; set 1st argument (Print Format)
  mov rsi, rbx            ; set 2nd argument (Fib Number)
  call printf             ; printf(outFormat, rbx)

  pop rbx                 
  pop rax
  ret
; ----------------------------------------

fib_loop:
  ; x[k+1] = x[k] + x[k-1]
  ; x[0] = 0, x[1] = 1 . so
  call  print_fib
  add   rax, rbx    
  xchg  rax, rbx   
  cmp   rbx, [fib_input_buff]
  js    fib_loop   
  ret
; ----------------------------------------

_exit:
  mov  rax, 60
  mov  rdi, 0
  syscall
; ----------------------------------------

