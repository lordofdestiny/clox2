#include <assert.h>

#include <gen/scanner.h>
#include <gen/parser.h>

int
main (void)
{
  yydebug = 0;
  yyscan_t scanner;
  yylex_init(&scanner);

  FILE* file = fopen("./lib/parser/input.lox", "rb");
  if(file == NULL){
    printf("File not found\n");
    return 0;
  }
  yyset_in(file, scanner);

  Node* result;
  arena_t* arena = arena_create(8 * 1024 * 1024);

  int ret = yyparse (arena, scanner, &result);
  assert(ret == 0);
  
  fclose(file);
  yylex_destroy( scanner);

  printNode(stdout, result);
  printf("\n");

  arena_destroy(arena);
  sbuff_reset();

  return ret;
}
