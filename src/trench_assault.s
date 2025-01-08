.intel_syntax noprefix
.text
.global zbyte_32_asm

# Function prototype:
# extern int zbyte_32_asm(uint32_t x);

zbyte_32_asm:
  # Input: rdi -> input uint32_t
  mov eax, edi
  and eax, 0x7f7f7f7f
  add eax, 0x7f7f7f7f

  or eax, edi
  or eax, 0x7f7f7f7f
  not eax

  bsf eax, eax
  jz not_found
  shr eax, 3
  ret

not_found:
  mov eax, -1
  ret
