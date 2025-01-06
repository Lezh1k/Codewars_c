.intel_syntax noprefix
.text
.global barr4_char_idx

# Function prototype:
# int barr4_char_idx(const char* array, char input_char);

barr4_char_idx:
  # Input: rdi -> address of 4-byte array, 
  # esi -> input_char
  mov eax, [rdi]
  mov ecx, esi

  # broakcast character to make mask
  mov edx, 0x01010101  
  imul ecx, edx

  # xor with the mask. matched bytes should give 0x00. others - not null vals
  xor eax, ecx
  # TODO!
  

  # Extract match mask
  and eax, 0x80808080  
  jz not_found         

  bsf eax, eax         # Find the first set bit
  shr eax, 3           # Divide by 8 to get byte index
  ret

not_found:
  mov eax, -1          # Return -1 if no match
  ret
