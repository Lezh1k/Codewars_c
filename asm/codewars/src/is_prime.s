global is_prime
section .text

is_prime:
  xor eax, eax

  ; if n < 1
  cmp rdi, 1
  jle __not_prime

  ; if n <= 3
  cmp rdi, 3
  jle __prime

  ; if n % 2 == 0
  test rdi, 0x01
  jz __not_prime

  ; if n % 3 == 0
  mov rdx, rdi 
  shr rdx, 32 
  mov eax, edi 
  mov rcx, 3
  div rcx
  test edx, edx
  jz __not_prime

  mov ecx, 5 ; i = 5
__loop:
  ; while i*i <= n
  mov eax, ecx
  mul ecx
  and rdx, 0xffffffff  ; clear high part
  shl rdx, 32
  mov edx, eax
  cmp rdx, rdi
  jg __prime

  ; if n % i == 0 or n % (i + 2) == 0:
  mov rdx, rdi
  shr rdx, 32
  mov eax, edi
  div rcx
  test edx, edx
  jz __not_prime

  add ecx, 2
  mov rdx, rdi
  shr rdx, 32
  mov eax, edi
  div rcx
  test edx, edx
  jz __not_prime

  ; i += 6 ( but ecx was updated 8 lines above)
  add ecx, 4
  jmp __loop

__not_prime:
  xor eax, eax
  ret

__prime:
  mov eax, 1
  ret
  

