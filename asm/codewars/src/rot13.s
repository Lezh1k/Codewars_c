global rot_13, rot_13_sse
extern strdup, calloc, strlen, strcpy

section .data
  align 16
    msk_0d: dq 0x0d0d0d0d0d0d0d0d, 0x0d0d0d0d0d0d0d0d
    msk_1a: dq 0x1a1a1a1a1a1a1a1a, 0x1a1a1a1a1a1a1a1a
    msk_20: dq 0x2020202020202020, 0x2020202020202020

    msk_a: dq 0x6161616161616161, 0x6161616161616161
    msk_m: dq 0x6d6d6d6d6d6d6d6d, 0x6d6d6d6d6d6d6d6d  ; 'z' - 13
    msk_z: dq 0x7a7a7a7a7a7a7a7a, 0x7a7a7a7a7a7a7a7a


section .text

; ROT13 is a simple letter substitution cipher 
; that replaces a letter with the 
; letter 13 letters after it in the alphabet. 
; ROT13 is an example of the Caesar cipher.
;
; Create a function that takes a string and returns the string 
; ciphered with Rot13. If there are numbers or special characters 
; included in the string, they should be returned as they are. 
; Only letters from the latin/english alphabet should be shifted, 
; like in the original Rot13 "implementation".


rot_13_sse:
  push rdi      ; save original string pointer
  call strlen

  add rax, 32   ; we need 16 for alignment and 16 for zero end
  mov rdi, rax
  mov rsi, 1
  ; aligned calloc
  call calloc
  add rax, 15
  and rax, ~15

  ; strcpy
  pop rsi       ; src -> original string pointer
  mov rdi, rax  ; dst -> allocated memory
  call strcpy

  ; rot13_sse
  mov rdi, rax
  sub rax, 16   ; just decrement before loop starts

.r13v_str_loop:
  add rax, 16
  movaps xmm0, [rax]
  ptest xmm0, xmm0
  jz .r13v_str_end

  movaps xmm3, xmm0       ; in xmm1 we are going to store offsets
  orps xmm3, [msk_20]     ; convert to lower case

  ; check if value is in range ['a'..'z']
  movaps xmm2, [msk_a]      ; load 'a' mask into xmm2
  pcmpgtb xmm2, xmm3        ; now in xmm2 0xff on positions less than 'a'
  pcmpeqb xmm1, xmm1        ; but we need 0xff on positions greater than 'a'
  pxor xmm2, xmm1           ; so we invert them

  movaps xmm1, [msk_z]      ; load 'z' mask into xmm1
  pcmpgtb xmm1, xmm3        ; now in xmm1 0xff on positions less then 'z'
  pand xmm1, xmm2
  ; now we have something like [0xff, 0x00, 0x00, 0xff..] in xmm1
  ; 0xff - where we have ['a'..'z'] char and 0x00 otherwise


  movaps [rax], xmm0
  jmp .r13v_str_loop

.r13v_str_end:
  mov rax, rdi
  ret

rot_13:
  call strdup
  mov rdi, rax
  dec rax

.str_loop:
  inc rax
  mov bh, [rax]  ; load char int bh register
  test bh, bh
  jz .str_end

  or  bh, 0x20          ; if bh in [A..Z] -> convert to [a..z]
  sub bh, 0x61          ; bh - 'a'
  cmp bh, 0x7a - 0x61   ; if bh not in range ['a'..'z'] -
  ja .str_loop          ; if bh > 'z' - 'a' - don't change

  add bh, 13 + 0x61     ; bh += 13 + 'a'
  cmp bh, 0x7a          ; cmp bh, 'z'
  jbe .less_z
  sub bh, 26
.less_z:
  cmp byte [rax], 0x61  ; cmp [rax], 'a'
  jge .greater_a
  sub bh, 32

.greater_a:
  mov byte [rax], bh
  ; next iteration
  jmp .str_loop
.str_end:
  mov rax, rdi
  ret
