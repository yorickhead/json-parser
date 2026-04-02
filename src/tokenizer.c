#include "tokenizer.h"
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

int get_token_numbers(const char *data) {
  if (data == NULL) {
    return 0;
  }

  const char *p = data;

  int number = 0;
  int q_number = 0;

  bool in_str = true;
  while (*p != '\0') {
    if (*p == '{' | *p == '}' | *p == ':' | *p == ',' | *p == '[' | *p == ']') {
      number++;
    }

    else if (*p == '"') {
      q_number++;
    }

    p++;
  }

  return number + (q_number/2)+5;
}

Token get_token_from_string(const char **start) {
  Token tkn;

  const char *p = *start + 1;

  int len = 0;

  while (*p != '"') {
    len++;
    p++;
  }

  tkn.type = STRING;
  tkn.start = *start;
  tkn.len = len + 2;

  *start = p + 1;

  return tkn;
}

Token get_next_token(const char **pos, int *opened_braces) {
  const char *p = *pos;

  while (*p && isspace(*p))
    p++;

  Token tkn;

  switch (*p) {
  case '{':
    tkn.type = LBRACE;
    tkn.start = p;
    tkn.len = 1;

    (*opened_braces)++;
    *pos = p + 1;

    return tkn;
  case '}':
    tkn.type = RBRACE;
    tkn.start = p;
    tkn.len = 1;

    (*opened_braces)--;
    *pos = p + 1;

    return tkn;
  case ',':
    tkn.type = COMMA;
    tkn.start = p;
    tkn.len = 1;

    *pos = p + 1;

    return tkn;
  case ':':
    tkn.type = COLON;
    tkn.start = p;
    tkn.len = 1;

    *pos = p + 1;

    return tkn;

  case '[':
    tkn.type = L_ARR_BRACE;
    tkn.start = p;
    tkn.len = 1;

    *pos = p + 1;

    return tkn;
  case ']':
    tkn.type = R_ARR_BRACE;
    tkn.start = p;
    tkn.len = 1;

    *pos = p + 1;
    return tkn;
  case '"':
    tkn = get_token_from_string(&p);
    *pos = p;
    return tkn;

  default:
    tkn.type = UNKNOWN;

    return tkn;
  }
}

Token *tokenize(const char *data) {
  int token_number = get_token_numbers(data);

  Token *start = calloc(token_number, sizeof(Token));
  Token *position = start;
  Token tkn;

  const char *p = data;

  int opened_braces = 0;

  do {
    Token tkn = get_next_token(&p, &opened_braces);

    printf("new token with len: %zu\n", tkn.len);

    if (position - start >= token_number - 1) {
      fprintf(stderr, "Too many tokens (max %d)\n", token_number - 1);
      free(start);
      return NULL;
    }

    *position = tkn;
    position++;

    if (tkn.type == UNKNOWN) {
      fprintf(stderr, "Unknown token near position %zu\n", (size_t)(p - data));
      free(start);
      return NULL;
    }
  } while (opened_braces != 0);

  tkn.type = EOF_TOKEN;
  tkn.len = 0;
  tkn.start = NULL;

  *position = tkn;

  return start;
}
