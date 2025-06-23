global is_leap_year

; The RDI - year
; #define IS_LEAP_YEAR(Y)     ( ((Y)>0) && !((Y)%4) && ( ((Y)%100) || !((Y)%400) ) )
is_leap_year:
  xor eax, eax
  ret
  

