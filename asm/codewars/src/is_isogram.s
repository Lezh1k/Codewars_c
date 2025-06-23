global is_isogram

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
