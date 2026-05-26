#include <assert.h>

#include <gen/scanner.h>
#include <gen/parser.h>

typedef struct parser_t{
  yyscan_t scanner;
  FILE* file;
  arena_t* arena;
  bool closed;
} parser_t;

static int parser_init(parser_t* parser, const char* filename)
{
  parser->closed = true;
  parser->arena = arena_create(8 * 1024 * 1024);
  if (parser->arena == NULL) {
    return -1;
  }
  int ret = yylex_init(&parser->scanner);
  if (ret != 0) {
    return ret;
  }
  FILE* file = fopen(filename, "rb");
  if (file == NULL) {
    return -2;
  }
  parser->file = file;
  yyset_in(parser->file, parser->scanner);
  parser->closed = false;
  return 0;
}

static int parse(parser_t* parser, Node** result)
{
  return yyparse(parser->arena, parser->scanner, result);
}

static void parser_close(parser_t* parser)
{
  if (!parser->closed) {
    fclose(parser->file);
    parser->file = NULL;
    yylex_destroy(parser->scanner);
    parser->scanner = NULL;
    parser->closed = true;
  }
}

static void parser_destroy(parser_t* parser)
{
  parser_close(parser);
  if (parser->arena) {
    arena_destroy(parser->arena);
  }
}

int
main (void)
{
  yydebug = 0;
  parser_t parser;
  int ret = parser_init(&parser, "./lib/parser/input.lox");
  if (ret != 0) {
    fprintf(stderr, "Failed to initialize parser\n");
    return 1;
  }

  Node* result;

  ret = parse(&parser, &result);
  if (ret != 0) {
    fprintf(stderr, "Parse error\n");
    return 1;
  }

  parser_close(&parser);

  if (result == NULL) {
    fprintf(stderr, "Failed to parse input\n");
    return 1;
  }

  printNode(stdout, result);
  printf("\n");

  parser_destroy(&parser);

  return ret;
}
