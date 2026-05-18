#include <assert.h>
#include <ctype.h>
#include <stdalign.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define CMD_TOKENS_N 4

typedef struct str_view {
  const char *data;
  size_t len;
} str_view_t;
#define SV_FMT "%.*s"

enum {
  TT_UNKNOWN = 0,
  TT_CMD,
  TT_REG,
  TT_CONST,
};

enum {
  ST_UNKNOWN = 0,
  ST_MOV,
  ST_INC,
  ST_DEC,
  ST_JNZ,
};

typedef struct token_type {
  int type;
  int subtype;
} token_type_t;

static token_type_t tt(const str_view_t *sv) {
  token_type_t res = {0};
  static const char *supported_commands[] = {"mov", "inc", "dec", "jnz", NULL};
  for (size_t i = 0; supported_commands[i]; ++i) {
    // HACK! 
    if (memcmp(supported_commands[i], sv->data, 3) == 0) {
      res.type = TT_CMD;
      res.subtype = i + 1;
      return res;
    }
  }

  if (sv->len == 1 && isalpha((char)sv->data[0])) {
    res.type = TT_REG;
    return res;
  }

  res.type = TT_CONST;
  return res;
}

typedef struct token {
  str_view_t sv;
  token_type_t tt;
} token_t;

typedef struct cmd {
  token_t tokens[CMD_TOKENS_N];
} cmd_t;

str_view_t sv(const char *str, size_t len) {
  assert(str);
  return (str_view_t){
      .data = str,
      .len = len,
  };
}

str_view_t sv_from_str(const char *data) {
  assert(data);
  return (str_view_t){
      .data = data,
      .len = strlen(data),
  };
}

int sv_cmp(const str_view_t *l, const str_view_t *r) {
  size_t len = l->len >= r->len ? r->len : l->len;
  for (size_t i = 0; i < len; ++i) {
    if (l->data[i] == r->data[i])
      continue;
    return (int)(l->data[i] - r->data[i]);
  }
  return 0;
}

cmd_t tokenize(const char *str) {
  static const char delim = ' ';
  cmd_t tv = {0};
  char *ptr = strchr(str, delim);
  size_t i = 0;
  for (; ptr != NULL; ptr = strchr(str, delim), ++i) {
    size_t len = (size_t)(ptr - str);
    tv.tokens[i].sv = sv(str, len);
    tv.tokens[i].tt = tt(&tv.tokens[i].sv);
    str = ptr + 1;
  }
  tv.tokens[i].sv = sv_from_str(str);
  tv.tokens[i].tt = tt(&tv.tokens[i].sv);
  return tv;
}

void simple_assembler(size_t n, const char *const program[n], int registers[]) {
  int IR_step = 0;
  // size_t it_max = 12200;
  // size_t it = 0;
  for (size_t IR = 0; IR < n; IR += IR_step) {
    IR_step = 1;
    const char *cmd = program[IR];
    cmd_t tv = tokenize(cmd);

    if (tv.tokens[0].tt.type != TT_CMD) {
      fprintf(stderr, "invalid input (not cmd): %s\n", program[IR]);
      exit(1);
    }

    switch (tv.tokens[0].tt.subtype) {
    case ST_DEC:
      --registers[(int)tv.tokens[1].sv.data[0]];
      break;

    case ST_INC:
      ++registers[(int)tv.tokens[1].sv.data[0]];
      break;

    case ST_MOV: {
      int *dst = &registers[(int)tv.tokens[1].sv.data[0]];

      if (tv.tokens[2].tt.type == TT_REG) {
        *dst = registers[(int)tv.tokens[2].sv.data[0]];
      } else if (tv.tokens[2].tt.type == TT_CONST) {
        *dst = strtol(tv.tokens[2].sv.data, NULL, 10);
      } else {
        fprintf(stderr, "invalid input (2nd token type) %d: %s\n",
                tv.tokens[2].tt.type, program[IR]);
        exit(2);
      }
      break;
    }

    case ST_JNZ:
      if (tv.tokens[1].tt.type == TT_REG &&
          !registers[(int)tv.tokens[1].sv.data[0]]) {
        break;
      }
      if (tv.tokens[1].tt.type == TT_CONST &&
          !strtol(tv.tokens[1].sv.data, NULL, 10)) {
        break;
      }

      if (tv.tokens[2].tt.type == TT_REG) {
        IR_step = registers[(int)tv.tokens[2].sv.data[0]];
      } else if (tv.tokens[2].tt.type == TT_CONST) {
        IR_step = strtol(tv.tokens[2].sv.data, NULL, 10);
      } else {
        fprintf(stderr, "invalid input: %s\n %d\n", program[IR],
                tv.tokens[2].tt.type);
        exit(2);
      }
      break;

    default:
      fprintf(stderr, "unsupported cmd: %s\n", program[IR]);
      exit(3);
    } // switch

    // printf("%zu -> %zu: %s\n", IR, IR + IR_step, program[IR]);
    // printf("a = %d\nb = %d\nc = %d\nd = %d\n", registers['a'],
    // registers['b'],
    //        registers['c'], registers['d']);
  }
}

int main(int argc, char *argv[]) {
  (void)argc;
  (void)argv;

  int registers[128] = {0};
  // clang-format off
  const char *program[] = {
    "mov d 100","dec d","mov b d","jnz b -2","inc d","mov a d","jnz 5 10","mov c a"
      // "mov a 1",  
      // "mov b 1",  
      // "mov c 0", 
      // "mov d 26", 
      // "jnz c 2",  
      // "jnz 1 5",
      // "mov c 7",  
      // "inc d",    
      // "dec c",   
      // "jnz c -2", 
      // "mov c a",  
      // "inc a",
      // "dec b",    
      // "jnz b -2", 
      // "mov b c", 
      // "dec d",    
      // "jnz d -6", 
      // "mov c 18",
      // "mov d 11", 
      // "inc a",    
      // "dec d",   
      // "jnz d -2", 
      // "dec c",    
      // "jnz c -5"
  };
  // clang-format on
  size_t program_len = sizeof(program) / sizeof(*program);
  simple_assembler(program_len, program, registers);

  printf("\n\na = %d\nb = %d\nc = %d\nd = %d", registers['a'], registers['b'],
         registers['c'], registers['d']);
  // initscr();
  // printw("Hello World!\n");
  // refresh();
  // getch();
  // endwin();
  return 0;
}
//////////////////////////////////////////////////////////////
