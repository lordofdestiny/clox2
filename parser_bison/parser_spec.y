%require "3.8.2"

%code requires {
  #include "token.h"
  typedef void* yyscan_t;
  static const char* token_name(int t);
}

%{
  #include <stdio.h>
  #include <math.h>
%}

%locations
%define parse.trace
%define api.location.type {TokenLocation}
%define api.header.include {"parser_spec.h"}
%define api.pure full
%define api.token.prefix {TOKEN_}

%define api.value.type {Token}
%lex-param {yyscan-T yyscanner}
%parse-param {yyscan_t yyscanner}

%code provides {
  int yylex (Token* lvalp, YYLTYPE*, yyscan_t);
  void yyerror (YYLTYPE*, yyscan_t yyscanner, char const *s);
  #define TOKEN_ERROR TOKEN_YYerror
  #define TOKEN_EOF TOKEN_YYEOF
  #define TOKEN_UNDEF TOKEN_YYUNDEF
}

%{

# define YYLLOC_DEFAULT(Cur, Rhs, N)                      \
do                                                        \
  if (N)                                                  \
    {                                                     \
      (Cur).line   = YYRHSLOC(Rhs, 1).line;               \
      (Cur).column = YYRHSLOC(Rhs, 1).column;             \
    }                                                     \
  else                                                    \
    {                                                     \
      (Cur).line   =   YYRHSLOC(Rhs, 0).line;             \
      (Cur).line   =   YYRHSLOC(Rhs, 0).line;             \
    }                                                     \
while (0)

%}

// Single character Tokens
%token  LEFT_PAREN RIGHT_PAREN
%token  LEFT_BRACKET RIGHT_BRACKET 
%token  LEFT_BRACE RIGHT_BRACE COMMA COLON 
%token  DOT VERTICAL_LINE MINUS PERCENT 
%token  PLUS SEMICOLON SLASH STAR 
%token  QUESTION BANG EQUAL GREATER 
%token  LESS

// Two character tokens
%token	MINUS_EQUAL PERCENT_EQUAL PLUS_EQUAL SLASH_EQUAL 
%token	STAR_EQUAL STAR_STAR BANG_EQUAL EQUAL_EQUAL 
%token	GREATER_EQUAL LESS_EQUAL

// Value literal tokens
%token  IDENTIFIER STRING NUMBER

// Keyword tokens
%token  AND AS BREAK CASE 
%token  CATCH CLASS CONTINUE DEFAULT 
%token  ELSE FALSE FOR FUN 
%token  FINALLY IF NIL OR 
%token  PRINT RETURN STATIC SUPER 
%token  SWITCH THIS THROW TRUE 
%token  TRY VAR WHILE

%printer { fprintf(yyo, "TOKEN at %d:%d = %.*s", @$.line, @$.column, $$.length, $$.chars); } <>

%% /* Grammar rules and actions follow. */

input:
  %empty
  | Statements YYEOF ;

Statements:
  Statement
  | Statements Statement
  ;

Statement:
  DeclarationStatement
  | ExpressionStatement
  ;

DeclarationStatement:
  VarDeclarationStatement
  ;

VarDeclarationStatement: VarDeclaration SEMICOLON ;

VarDeclaration: VAR VarDeclarationList ;

VarDeclarationList:
  VarDeclElement 
  | VarDeclarationList COMMA VarDeclElement
  ;

VarDeclElement:
  IDENTIFIER EQUAL Expression ;

ExpressionStatement:
  Expression SEMICOLON
  ;

Expression:
  STRING
  ;

%% 
/* The lexical analyzer returns a double floating point
   number on the stack and the token NUM, or the numeric code
   of the character read if not a number.  It skips all blanks
   and tabs, and returns 0 for end-of-input. */

#include <ctype.h>
#include <stdlib.h>

#include <stdio.h>

/* Called by yyparse on error. */
void
yyerror (YYLTYPE*, yyscan_t yyscanner, char const *s)
{
  fprintf (stderr, "%s\n", s);
}

static const char* token_name(int t) {
  return yysymbol_name(YYTRANSLATE(t));
}