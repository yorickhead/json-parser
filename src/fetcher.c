#include "fetcher.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static Pair *find_pair(const char *key, size_t key_len, Object *ast) {
  if (ast == NULL || key == NULL) {
    return NULL;
  }

  for (int i = 0; i < ast->pair_count; i++) {
    Pair *pair = &ast->pairs[i];
    if (strncmp(pair->key, key, key_len) == 0 && pair->key[key_len] == '\0') {
      return pair;
    }
  }

  return NULL;
}

static Object *fetch_object_from_ast_internal(const char *key, size_t key_len,
                                              Object *ast) {
  Pair *pair = find_pair(key, key_len, ast);
  if (pair == NULL) {
    return NULL;
  }

  if (pair->value->type != OBJECT) {
    fprintf(stderr, "value for key '%.*s' is not an object\n", (int)key_len,
            key);
    return NULL;
  }

  return pair->value->val->obj;
}

static List *fetch_list_from_ast_internal(const char *key, size_t key_len,
                                          Object *ast) {
  Pair *pair = find_pair(key, key_len, ast);
  if (pair == NULL) {
    return NULL;
  }

  if (pair->value->type != LIST) {
    fprintf(stderr, "value for key '%.*s' is not a list (type=%d)\n", 
            (int)key_len, key, pair->value->type);
    return NULL;
  }

  return pair->value->val->list;
}

static void *fetch_string_from_ast_internal(const char *key, size_t key_len,
                                            Object *ast) {
  Pair *pair = find_pair(key, key_len, ast);
  if (pair == NULL) {
    return NULL;
  }

  if (pair->value->type != STRING_VALUE) {
    fprintf(stderr, "value for key '%.*s' is not a string\n", (int)key_len,
            key);
    return NULL;
  }

  return pair->value->val->str;
}

static ValueTyped *fetch_from_list_internal(int index, List *list) {
  if (list == NULL || index < 0 || index >= list->len) {
    fprintf(stderr, "list index out of bounds: %d (len: %d)\n", index,
            list ? list->len : 0);
    return NULL;
  }

  return &list->elems[index];
}

static int parse_array_index(const char *start, const char **end) {
  if (*start != '[') {
    return -1;
  }

  start++;
  int index = 0;

  while (*start != ']' && *start != '\0') {
    if (!isdigit(*start)) {
      fprintf(stderr, "invalid array index\n");
      return -1;
    }
    index = index * 10 + (*start - '0');
    start++;
  }

  if (*start != ']') {
    fprintf(stderr, "unclosed array index bracket\n");
    return -1;
  }

  *end = start + 1;
  return index;
}

void *fetch_from_ast(const char *key, Object *ast) {
  if (ast == NULL || key == NULL) {
    fprintf(stderr, "ast or key is null\n");
    return NULL;
  }

  const char *current_key = key;
  Object *current_obj = ast;
  List *current_list = NULL;

  while (*current_key != '\0') {
    // Check if we're looking at an array index
    if (*current_key == '[') {
      if (current_list == NULL) {
        fprintf(stderr, "array index without preceding list\n");
        return NULL;
      }

      const char *end;
      int index = parse_array_index(current_key, &end);
      if (index < 0) {
        return NULL;
      }

      ValueTyped *elem = fetch_from_list_internal(index, current_list);
      if (elem == NULL) {
        return NULL;
      }

      current_key = end;
      current_list = NULL;

      // Skip the dot after the index if present
      if (*current_key == '.') {
        current_key++;
      }

      // If we're at the end, return the element value based on type
      if (*current_key == '\0') {
        if (elem->type == STRING_VALUE) {
          return elem->val->str;
        }
        fprintf(stderr, "cannot return non-string value directly\n");
        return NULL;
      }

      // If element is an object, continue traversing
      if (elem->type == OBJECT) {
        current_obj = elem->val->obj;
        continue;
      } else if (elem->type == LIST) {
        current_list = elem->val->list;
        continue;
      } else {
        fprintf(stderr, "cannot traverse non-object/non-list value\n");
        return NULL;
      }
    }

    // Find the next dot or bracket
    const char *dot = strchr(current_key, '.');
    const char *bracket = strchr(current_key, '[');

    const char *next_pos;
    if (dot == NULL && bracket == NULL) {
      next_pos = NULL;
    } else if (dot == NULL) {
      next_pos = bracket;
    } else if (bracket == NULL) {
      next_pos = dot;
    } else {
      // Both dot and bracket exist - choose the earlier one
      next_pos = (dot < bracket) ? dot : bracket;
    }

    if (next_pos == NULL) {
      // This is the last key
      return fetch_string_from_ast_internal(current_key, strlen(current_key),
                                           current_obj);
    }

    size_t key_len = next_pos - current_key;

    if (*next_pos == '[') {
      // Direct bracket notation (no preceding dot or object key)
      // This shouldn't happen in normal usage, but handle it
      if (key_len == 0) {
        fprintf(stderr, "invalid syntax: '[' at start of path\n");
        return NULL;
      }
      // fetch "current_key" as a list first
      current_list = fetch_list_from_ast_internal(current_key, key_len,
                                                  current_obj);
      if (current_list == NULL) {
        return NULL;
      }
      current_key = next_pos;
    } else if (*next_pos == '.') {
      // Check if this dot is immediately followed by bracket (.[index] pattern)
      if (*(next_pos + 1) == '[') {
        // This is ".[" - treat it as array access on current key
        current_list = fetch_list_from_ast_internal(current_key, key_len,
                                                    current_obj);
        if (current_list == NULL) {
          return NULL;
        }
        // Skip the dot and move to the bracket
        current_key = next_pos + 1;
      } else {
        // Regular dot separator - access nested object
        current_obj = fetch_object_from_ast_internal(current_key, key_len,
                                                     current_obj);
        if (current_obj == NULL) {
          return NULL;
        }
        current_key = next_pos + 1; // Skip the dot
      }
    }
  }

  return NULL;
}

Object *fetch_object_from_ast(const char *key, Object *ast) {
  if (ast == NULL || key == NULL) {
    fprintf(stderr, "ast or key is null\n");
    return NULL;
  }

  return fetch_object_from_ast_internal(key, strlen(key), ast);
}
