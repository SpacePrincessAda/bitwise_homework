#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <assert.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdbool.h>
#include <stdarg.h>

#include "../types.h"

void *xmalloc(size_t num_bytes) {
  void *ptr = malloc(num_bytes);
  if (!ptr) {
    perror("xmalloc failed");
    exit(1);
  }
  return ptr;
}

void fatal(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  printf("FATAL: ");
  vprintf(fmt, args);
  printf("\n");
  va_end(args);
  exit(1);
}

#define kilobytes(value) ((value)*1024LL)
#define megabytes(value) (kilobytes(value)*1024LL)
#define gigabytes(value) (megabytes(value)*1024LL)

typedef struct memory_arena_t {
  size_t size;
  size_t alignment;
  u8 *base;
  size_t used;
} memory_arena_t;

void init_arena(memory_arena_t* arena, size_t size, void* base) {
  arena->size = size;
  arena->base = (u8*)base;
  arena->used = 0;
}

void* push_size(memory_arena_t* arena, size_t size) {
  assert((arena->used + size) <= arena->size);

  void* result = arena->base + arena->used;
  arena->used += size;

  return result;
}

void reset_arena(memory_arena_t *arena) {
  arena->used = 0;
}

typedef enum token_kind_t {
  TOKEN_INT,
} token_kind_t;

typedef struct token_t {
  token_kind_t kind;
  const char *start;
  const char *end;
  union {
    int val;
  };
  
} token_t;

token_t token;
const char *stream;

void next_token() {
  token.start = stream;
  switch (*stream) {
    case '0' ... '9': {
      int val = 0;
      while (isdigit(*stream)) {
        val *= 10;
        val += *stream++ - '0';
      }
      token.kind = TOKEN_INT;
      token.val = val;
      break;
    }
    case ' ':
      // Not ideal
      token.kind = *stream++;
      next_token();
      break;
    default:
      token.kind = *stream++;
      break;
  }
  token.end = stream;
}

static inline bool is_token(token_kind_t kind) {
  return token.kind == kind;
}

typedef struct node_t {
  token_t token;
  struct node_t *left;
  struct node_t *right;
} node_t;

const char *stream;

node_t *create_node(token_t t, node_t *a, node_t *b) {
  node_t *n = xmalloc(sizeof(node_t));
  n->token = t;
  n->left = a;
  n->right = b;
  return n;
}

node_t *parse_expr3() {
  if (is_token(TOKEN_INT)) {
    node_t *node = create_node(token, 0, 0);
    next_token();
    return node;
  } else {
    fatal("expected integer\n");
    return NULL;
  }
}

node_t *parse_expr2() {
  if (is_token('~')) {
    node_t *r = create_node(token, 0, 0);
    next_token();
    r->left = parse_expr2();
    return r;
  } else {
    return parse_expr3();
  }
}

node_t *parse_expr1() {
  node_t *r = parse_expr2();
  while (is_token('*') || is_token('/')) {
    r = create_node(token, r, 0);
    next_token();
    r->right = parse_expr2();
  }
  return r;
}

node_t *parse_expr0() {
  node_t *r = parse_expr1();
  while (is_token('+') || is_token('-')) {
    r = create_node(token, r, 0);
    next_token();
    r->right = parse_expr1();
  }
  return r;
}

node_t *parse_expr() {
  return parse_expr0();
}

void init_stream(const char *str) {
  stream = str;
  next_token();
}

void print_tree(node_t *tree) {
  if (tree->token.kind == TOKEN_INT) {
    printf("%d", tree->token.val);
  } else {
    printf("(%c", tree->token.kind);
    if (tree->left) {
      printf(" ");
      print_tree(tree->left);
    }
    if (tree->right) {
      printf(" ");
      print_tree(tree->right);
    }
    printf(")");
  }
}

void parse_and_print(const char *in) {
  init_stream(in);
  node_t *tree = parse_expr();
  printf("input: %s\nresult: ", in);
  print_tree(tree);
  printf("\n\n");
}

static const u8 HALT = 1;
static const u8 LIT = 2;
static const u8 SUB = '-';
static const u8 ADD = '+';
static const u8 MUL = '*';
static const u8 DIV = '/';

#define POP() \
    (*--top)

#define PUSH(x) \
    (*top++ = (x))

#define POPS(n) \
    assert(top - stack >= (n))

#define PUSHES(n) \
    assert(top + (n) <= stack + MAX_STACK)

s32 vm_exec(const u8 *code) {
  enum { MAX_STACK = 1024 };
  s32 stack[MAX_STACK];
  s32 *top = stack;
  for (;;) {
    u8 op = *code++;
    switch (op) {
      case SUB: {
        POPS(2);
        // Note the stack's operand order!
        s32 right = POP();
        s32 left = POP();
        PUSHES(1);
        PUSH(left - right);
        break;
      }
      case ADD: {
        POPS(2);
        // Note the stack's operand order!
        s32 right = POP();
        s32 left = POP();
        PUSHES(1);
        PUSH(left + right);
        break;
      }
      case MUL: {
        POPS(2);
        // Note the stack's operand order!
        s32 right = POP();
        s32 left = POP();
        PUSHES(1);
        PUSH(left * right);
        break;
      }
      case DIV: {
        POPS(2);
        // Note the stack's operand order!
        s32 right = POP();
        s32 left = POP();
        PUSHES(1);
        PUSH(left / right);
        break;
      }
      case LIT:
        PUSHES(1);
        // Unaligned reads are legal and fast on modern PC-class processors.
        // This assumes the target architecture is little endian; as a bonus
        // exercise, figure out how to read this little endian int32 in a way
        // that's correct regardless of the target computer's native endianness.
        PUSH(*(s32 *)code);
        code += sizeof(s32);
        break;
      case HALT:
        POPS(1);
        return POP();
      default:
        fatal("vm_exec: illegal opcode");
        return 0;
    }
  }
}

void push_op(memory_arena_t *arena, u8 op) {
  u8 *_op = push_size(arena, sizeof(u8));
  *_op = op;
}

void push_lit(memory_arena_t *arena, s32 value) {
  push_op(arena, LIT);
  s32 *val = push_size(arena, sizeof(s32));
  *val = value;
}

void test_vm_op(memory_arena_t *arena, s32 a, s32 b, u8 op, s32 result) {
  push_lit(arena, a);
  push_lit(arena, b);
  push_op(arena, op);
  push_op(arena, HALT);
  s32 r = vm_exec(arena->base);
  assert(r == result);
  reset_arena(arena);
}

void compile(memory_arena_t *arena, node_t *tree) {
  if (tree->token.kind == TOKEN_INT) {
    push_lit(arena, tree->token.val);
  } else {
    compile(arena, tree->left);
    compile(arena, tree->right);
    push_op(arena, tree->token.kind);
  }
}

int parse_expr_str(memory_arena_t *arena, const char *str) {
  init_stream(str);
  node_t *tree = parse_expr();
  compile(arena, tree);
  push_op(arena, HALT);
  s32 r = vm_exec(arena->base);
  reset_arena(arena);
  return r;
}

#define assert_expr_vm(a, x) assert(parse_expr_str(a, #x) == (x))

int main(int argc, char **argv) {
  void* memory = xmalloc(megabytes(1));
  memory_arena_t arena = {};
  init_arena(&arena, megabytes(1), memory);

  test_vm_op(&arena, 20, 5, SUB, 15);
  test_vm_op(&arena, 20, 5, ADD, 25);
  test_vm_op(&arena, 20, 5, MUL, 100);
  test_vm_op(&arena, 20, 5, DIV, 4);

  assert_expr_vm(&arena, 12*34+45/56+25);

  return 0;
}

