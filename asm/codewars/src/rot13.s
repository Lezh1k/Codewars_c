global rot_13
extern strdup

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

rot_13:
  call strdup
  mov rdi, rax
  dec rax

.str_loop:
  inc rax
  mov bh, [rax]  ; load char int bh register
  test bh, bh
  jz .str_end
  ; do checks and conversions

  or  bh, 0x20          ; if bh in [A..Z] -> convert to [a..z]
  sub bh, 0x61          ; bh - 'a'
  cmp bh, 0x7a - 0x61   ; if bh not in range ['a'..'z'] -
  ja .str_loop          ; if bh < 'z' - 'a' - don't change

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
