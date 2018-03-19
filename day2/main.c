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

// unary -, unary ~    (right associative)
// * / % << >> &       (left associative)
// + - | ^             (left associative)
//
// input:  12*34 + 45/56 + ~25
// output: (+ (+ (* 12 34) (/ 45 56)) (~ 25))

int main(int argc, char **argv) {
  parse_and_print("12*34 + 45/56 + ~25");

  return 0;
}

