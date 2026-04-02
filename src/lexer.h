#include "tokenizer.h"

typedef struct Object Object;
typedef struct Pair Pair;
typedef union Value Value;
typedef struct List List;

enum ValueType {
  OBJECT,
  STRING_VALUE,
  LIST,
};

typedef struct {
  Value *val;
  enum ValueType type;
} ValueTyped;

union Value {
  char *str;
  Object *obj;
  List *list;
};

struct List {
  int len;
  ValueTyped *elems;
};

struct Object {
  int pair_count;
  Pair *pairs;
};

struct Pair {
  char *key;
  ValueTyped *value;
};

Object *tokens_to_ast(Token *tokens);
void destroy_object(Object *obj);
