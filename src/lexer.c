#include "lexer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define IF_NOT_NULL_DESTROY_STRING(p)

Pair *parse_pair(Token **start);
Object *parse_object(Token **start);
List *parse_list(Token **start);

void destroy_list(List *list);
void destroy_pair(Pair *pair);
void destroy_object(Object *obj);

void destroy_value(ValueTyped *vt) {
  if (vt->type == 0 || vt == NULL) {
    return;
  }

  switch (vt->type) {
  case STRING_VALUE:
    if (vt->val->str != NULL)
      free(vt->val->str);
    break;
  case OBJECT:
    if (vt->val->obj != NULL)
      destroy_object(vt->val->obj);
    break;
  case LIST:
    if (vt->val->list != NULL)
      destroy_list(vt->val->list);
    break;
  }

  free(vt);
}

void destroy_list(List *list) {
  for (int i = 0; i < list->len; i++) {
    destroy_value(&list->elems[i]);
  }

  free(list);
}

void destroy_pair(Pair *pair) {
  free(pair->key);
  destroy_value(pair->value);

  free(pair);
}

void destroy_object(Object *obj) {
  for (int i = 0; i < obj->pair_count; i++) {
    destroy_pair(&obj->pairs[i]);
  }

  free(obj);
}

ValueTyped *parse_value(Token **start) {
  if (start == NULL || *start == NULL) {
    fprintf(stderr, "start is NULL");

    return NULL;
  }

  Token *p = *start;
  ValueTyped *value_typed = malloc(sizeof(ValueTyped));
  if (!value_typed) {
    fprintf(stderr, "failed allocate memory for typed value");

    return NULL;
  }

  value_typed->val = malloc(sizeof(Value));
  if (!value_typed->val) {
    fprintf(stderr, "failed allocate memory for value");

    free(value_typed);

    return NULL;
  }

  value_typed->type = 0;

  value_typed->val->str = NULL;
  value_typed->val->obj = NULL;
  value_typed->val->list = NULL;

  switch (p->type) {
  case STRING: {
    char *str = calloc(p->len + 1, sizeof(char));
    if (!str) {
      fprintf(stderr, "failed allocate memory for value");

      goto fail;
    }

    memcpy(str, p->start, p->len);

    str[p->len] = '\0';

    value_typed->type = STRING_VALUE;

    value_typed->val->str = str;

    p++;

    break;
  }

  case L_ARR_BRACE: {
    List *lst = parse_list(&p);
    if (lst == NULL) {
      fprintf(stderr, "failed parse list");

      goto fail;
    }

    value_typed->type = LIST;

    value_typed->val->list = lst;
    break;
  }

  case LBRACE: {
    Object *obj = parse_object(&p);
    if (obj == NULL) {
      fprintf(stderr, "failed parse object");

      goto fail;
    }

    value_typed->type = OBJECT;
    value_typed->val->obj = obj;
    break;
  }

  default:
    fprintf(stderr, "unknown token type");

    goto fail;
  }

  *start = p;

  return value_typed;

fail:
  destroy_value(value_typed);
  return NULL;
}

Pair *parse_pair(Token **start) {
  Token *p = *start;

  Pair *pair = malloc(sizeof(Pair));
  if (!pair) {
    fprintf(stderr, "failed allocate memory for pair");

    return NULL;
  }

  Token *tkn_key = p;
  if (tkn_key->type != STRING) {
    fprintf(stderr, "key token must be string");

    return NULL;
  }

  char *key = calloc(tkn_key->len + 1, sizeof(char));
  if (!key) {
    fprintf(stderr, "failed allocate memory for key");

    return NULL;
  }

  memcpy(key, tkn_key->start, tkn_key->len);

  key[tkn_key->len] = '\0';

  p++;

  Token *tkn_colon = p;

  if (tkn_colon->type != COLON) {
    fprintf(stderr, "token after key must be colon");

    free(key);

    return NULL;
  }

  p++;

  Token *tkn_value = p;

  ValueTyped *value_typed = parse_value(&p);

  pair->value = value_typed;

  *start = p;

  return pair;
}

int count_array_elements(Token **start) {
  Token *p = *start;

  if (!p || p->type != L_ARR_BRACE) {
    fprintf(stderr, "expexted token");
    return -1;
  }

  int depth = 1;
  int count = 0;
  int after_comma = 0;

  p++;

  while (p && depth > 0) {
    switch (p->type) {
    case L_ARR_BRACE: // [
    case LBRACE:      // {
      depth++;
      after_comma = 0;
      break;

    case R_ARR_BRACE: // ]
      depth--;
      after_comma = 0;
      break;

    case RBRACE: // }
      depth--;
      after_comma = 0;
      break;

    case COMMA: // ,
      if (depth == 1) {
        after_comma = 1;
      }
      break;

    default:
      if (depth == 1 && !after_comma) {
        count++;
        after_comma = 0;
      }
      break;
    }
    p++;
  }

  if (depth != 0) {
    fprintf(stderr, "Error: unclosed list or array\n");
    return -1;
  }

  return count;
}

List *parse_list(Token **start) {
  Token *p = *start;

  int len = count_array_elements(&p);
  if (len < 0) {
    fprintf(stderr, "failed count array elements");

    return NULL;
  }

  List *list = malloc(sizeof(List));
  if (!list) {
    fprintf(stderr, "failed allocate memory for list");

    return NULL;
  }

  list->elems = calloc(len, sizeof(ValueTyped));
  if (!list->elems) {
    fprintf(stderr, "failed allocate memory for elements");

    free(list);

    return NULL;
  }

  ValueTyped *current_list_element = list->elems;

  list->len = len;

  p = *start + 1;

  while (p->type != R_ARR_BRACE) {
    if (p->type != COMMA) {
      ValueTyped *value = parse_value(&p);
      if (value == NULL) {
        fprintf(stderr, "failed parse value from list");

        free(list);
        free(list->elems);

        return NULL;
      }

      current_list_element = value;

      current_list_element++;
    }

    p++;
  }

  return list;
}

int count_pairs_in_object(Token **start) {
  Token *p = *start;

  if (!p || p->type != L_ARR_BRACE) {
    fprintf(stderr, "expexted token");
    return -1;
  }

  int depth = 1;
  int count = 0;
  int after_key = 0;

  while (depth != 0) {
    if (p->type == STRING) {
      if (depth == 1 && after_key == 0) {
        after_key = 1;
        count++;
      }

      after_key = 0;
    }

    if (p->type == L_ARR_BRACE || p->type == LBRACE) {
      depth++;
    }

    if (p->type == R_ARR_BRACE || p->type == RBRACE) {
      depth--;
    }
  }

  return count;
}

Object *parse_object(Token **start) {
  Token *p = *start;

  Object *obj = malloc(sizeof(Object));
  if (!obj) {
    fprintf(stderr, "failed parse object");

    return NULL;
  }

  int pair_count = count_pairs_in_object(&p);
  if (pair_count < 0) {
    fprintf(stderr, "failed count pairs");

    free(obj);

    return NULL;
  }

  Pair *pairs = calloc(pair_count, sizeof(Pair));
  if (!pairs) {
    fprintf(stderr, "failed allocate memory for pairs");

    free(obj);

    return NULL;
  }

  obj->pairs = pairs;

  Pair *current_pair = pairs;

  p++;

  int after_comma = 1;
  int len = 0;

  while (p->type != RBRACE) {
    if (after_comma == 1) {
      after_comma = 0;

      if (p->type != STRING) {
        fprintf(stderr, "token after comma must be key");

        free(obj->pairs);
        free(obj);

        return NULL;
      }

      current_pair->key = calloc(p->len + 1, sizeof(char));
      if (!current_pair->key) {
        fprintf(stderr, "failed allocate memory for key");

        free(obj->pairs);
        free(obj);

        return NULL;
      }

      memcpy(current_pair->key, p->start, p->len);
    } else if (p->type == COMMA) {
      after_comma = 1;
    } else if (p->type == COLON) {
      continue;
    } else {
      ValueTyped *value = parse_value(&p);
      if (value == NULL) {
        fprintf(stderr, "failed parse value from pair");

        free(obj->pairs);
        free(obj);

        return NULL;
      }

      current_pair->value = value;
    }

    p++;
  }

  return obj;
}

Object *tokens_to_ast(Token *tokens) {
  Token *p = tokens;

  Object *ast = parse_object(&p);
  if (ast == NULL) {
    fprintf(stderr, "failed parse json data to ast");

    return NULL;
  }

  
}
