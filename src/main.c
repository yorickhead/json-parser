#include "lexer.h"
#include <stdio.h>
#include <stdlib.h>

char *read_from_file(const char *filename) {
  FILE *f = fopen(filename, "rb");
  if (!f) {
    perror("fopen");
    return NULL;
  }

  fseek(f, 0, SEEK_END);
  long size = ftell(f);
  fseek(f, 0, SEEK_SET);

  if (size < 0) {
    fclose(f);
    return NULL;
  }

  char *buf = malloc(size + 1);
  if (!buf) {
    fclose(f);
    return NULL;
  }

  size_t read = fread(buf, 1, size, f);
  fclose(f);

  if (read != (size_t)size) {
    free(buf);
    return NULL;
  }

  buf[size] = '\0';
  return buf;
}

void print_indent(int indent) {
  for (int i = 0; i < indent; i++) {
    printf("  ");
  }
}

void print_value(ValueTyped *vt, int indent);
void print_object(Object *obj, int indent);
void print_list(List *list, int indent);

void print_value(ValueTyped *vt, int indent) {
  if (vt == NULL) {
    printf("null");
    return;
  }

  switch (vt->type) {
  case STRING_VALUE:
    printf("\"%s\"", vt->val->str);
    break;
  case OBJECT:
    print_object(vt->val->obj, indent);
    break;
  case LIST:
    print_list(vt->val->list, indent);
    break;
  default:
    printf("unknown");
    break;
  }
}

void print_object(Object *obj, int indent) {
  if (obj == NULL) {
    printf("null");
    return;
  }

  printf("{\n");
  for (int i = 0; i < obj->pair_count; i++) {
    print_indent(indent + 1);
    printf("\"%s\": ", obj->pairs[i].key);
    print_value(obj->pairs[i].value, indent + 1);
    if (i < obj->pair_count - 1) {
      printf(",");
    }
    printf("\n");
  }
  print_indent(indent);
  printf("}");
}

void print_list(List *list, int indent) {
  if (list == NULL) {
    printf("null");
    return;
  }

  printf("[\n");
  for (int i = 0; i < list->len; i++) {
    print_indent(indent + 1);
    print_value(&list->elems[i], indent + 1);
    if (i < list->len - 1) {
      printf(",");
    }
    printf("\n");
  }
  print_indent(indent);
  printf("]");
}

int display_token(const Token *tkn) {
  printf("%.*s ", (int)tkn->len, tkn->start);
  return 1;
}

int main() {
  const char *file_name = "test/main.json";

  char *data = read_from_file(file_name);
  if (!data) {
    fprintf(stderr, "Failed to read file\n");
    return 1;
  }

  Token *tokens = tokenize(data);
  if (!tokens) {
    fprintf(stderr, "Tokenization failed\n");
    free(data);
    return 1;
  }

  printf("=== Tokens ===\n");
  Token *current = tokens;
  int token_count = 0;
  while (current->type != EOF_TOKEN) {
    printf("[%d] Type: %d, Value: %.*s\n", token_count, current->type,
           (int)current->len, current->start);
    current++;
    token_count++;
  }

  Object *ast = tokens_to_ast(tokens);
  if (!ast) {
    fprintf(stderr, "Parsing failed\n");
    free(tokens);
    free(data);
    return 1;
  }

  printf("\n=== AST ===\n");
  print_object(ast, 0);
  printf("\n\n");

  destroy_object(ast);
  free(tokens);
  free(data);

  printf("=== Done ===\n");

  return 0;
}
