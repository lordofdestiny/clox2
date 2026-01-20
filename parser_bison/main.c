#include "scanner_spec.h"
#include "parser_spec.h"

int
main (void)
{
  yydebug = 1;
  yyscan_t scanner;
  yylex_init(&scanner);

  FILE* file = fopen("input.lox", "rb");
  if(file == NULL){
    printf("???\n");
    return 0;
  }
  yyset_in(file, scanner);

  int ret = yyparse (scanner);
  yylex_destroy( scanner);

  return ret;
}
