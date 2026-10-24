%require "3.8.2"

%code requires {
  #include <common/arena.h>

  #include "token.h"
  #include "statement.h"
  #include "expr.h"
  
  typedef void* yyscan_t;
  static const char* token_name(int t);
}

%{
  #include <stdio.h>
  #include <math.h>
%}

%locations
%define parse.trace
%define parse.error detailed
%define api.location.type {TokenLocation}
%define api.header.include {<gen/parser.h>}
%define api.pure full
%define api.token.prefix {TOKEN_}

%lex-param {arena_t* arena}
%lex-param {yyscan_t yyscanner}

%parse-param { arena_t* arena }
%parse-param { yyscan_t yyscanner }
%parse-param { Node** result }

%union {
  void* stmtlist;
  Node* node;
  Expr* expr;
  Statement* stmt;
  DesignatorTarget* desig;
  Token* tok;
}

%printer {
  fprintf(yyo, "TOKEN at %d:%d = '", @$.line, @$.column);
  printToken(yyo, $$);
  fprintf(yyo, "'");
} <tok>

%printer {
  if ($$ != NULL) {
    fprintf(yyo, "Expression at %d:%d = '", @$.line, @$.column);
    printNode(yyo, (Node*) $$);
    fprintf(yyo, "'");
  }
} <*>

%code provides {

int yylex (YYSTYPE* lvalp, YYLTYPE*, arena_t*, yyscan_t);
void yyerror (YYLTYPE*, arena_t*, yyscan_t yyscanner, Node** result, char const *s);
#define TOKEN_ERROR TOKEN_YYerror
#define TOKEN_EOF TOKEN_YYEOF
#define TOKEN_UNDEF TOKEN_YYUNDEF

#define DEFINE_NODE(type, nodeType) \
  type* value = (type*) ALLOCATE_NODE(arena, type, nodeType); \
  if (value == NULL) { \
    YYNOMEM; \
  } \

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
      (Cur).column =   YYRHSLOC(Rhs, 0).column;           \
    }                                                     \
while (0)

%}

// Single character Tokens
%token <tok> LEFT_PAREN RIGHT_PAREN
%token <tok> LEFT_BRACKET RIGHT_BRACKET 
%token <tok> LEFT_BRACE RIGHT_BRACE COMMA COLON 
%token <tok> DOT VERTICAL_LINE MINUS PERCENT 
%token <tok> PLUS SEMICOLON SLASH STAR 
%token <tok> QUESTION BANG EQUAL GREATER 
%token <tok> LESS

// Two character tokens
%token <tok> MINUS_EQUAL PERCENT_EQUAL PLUS_EQUAL SLASH_EQUAL 
%token <tok> STAR_EQUAL STAR_STAR BANG_EQUAL EQUAL_EQUAL 
%token <tok> GREATER_EQUAL LESS_EQUAL

// Value literal tokens
%token <tok> IDENTIFIER STRING NUMBER

// Keyword tokens
%token <tok> AND AS BREAK CASE 
%token <tok> CATCH CLASS CONTINUE DEFAULT 
%token <tok> ELSE FALSE FOR FUN 
%token <tok> FINALLY IF NIL OR 
%token <tok> PRINT RETURN STATIC SUPER 
%token <tok> SWITCH THIS THROW TRUE 
%token <tok> TRY VAR WHILE

%type <stmtlist> input
%type <stmt> Statement Statements
%type <stmt> ExpressionStatement
%type <stmt> DeclarationStatement
%type <stmt> VarDeclaration VarDeclElement VarDeclarationList VarDeclarationStatement

%type <expr> Expression
%type <expr> PrimaryExpression
%type <expr> FunctionCall
%type <expr> ConditionalExpression
%type <expr> OrderingExpression
%type <expr> AdditionExpression
%type <expr> TermExpression
%type <expr> FactorExpression
%type <expr> ConjunctionExpression
%type <expr> DisjunctionExpression
%type <expr> EqualityExpression
%type <expr> ExponentExpression
%type <expr> AssignmentExpression
%type <desig> Designator
%type <tok> UnaryOperator MultiplicationOperator AdditionOperator
%type <tok> OrderingOperator EqualityOperator AssignmentOperator
/* %type <ph> ArgumentList */

%% /* Grammar rules and actions follow. */

input:
  %empty {
    $$ = (void*) (NULL);
    *result = NULL;
  }
  | Statements YYEOF {
    $$ = (void*) $1;
    *result = $$;
  }
  ;

Statements:
  Statement {
    $$ = $1;
  }
  | Statements Statement {
    $$ = $2;
  }
  ;

Statement:
  DeclarationStatement
  | ExpressionStatement {
    $$ = $1;
  }
  ;

DeclarationStatement:
  VarDeclarationStatement {
    $$ = $1;
  }
  ;

VarDeclarationStatement:
  VarDeclaration SEMICOLON {
    $$ = $1;
  }
  ;

VarDeclaration: VAR VarDeclarationList {
  $$ = $2;
}

VarDeclarationList:
  VarDeclElement 
  | VarDeclarationList COMMA VarDeclElement
  ;

VarDeclElement:
  IDENTIFIER EQUAL Expression {
    $$ = NULL;
  }
  ;

ExpressionStatement:
  Expression SEMICOLON {
    DEFINE_NODE(ExpressionStatement, NODE_STATEMENT);
    value->expr = $1;
    $$ = (Statement*) value;
  }
  ;

Expression:
  AssignmentExpression {
    $$ = $1;
  }
  /* | LambdaExpression
  | ContainerExpression */
  ;

/* LambdaExpression: ; */

/* ContainerExpression: ; */

AssignmentExpression:
  ConditionalExpression
  | Designator AssignmentOperator Expression {
      DEFINE_NODE(Assignment, NODE_ASSIGNMENT);
      value->target = (Designator*) $1;
      value->expr = (Expr*) $3;
      $$ = (Expr*) value;
  }
  ;

Designator:
  IDENTIFIER {
    DEFINE_NODE(DesignatorIdentifier, NODE_DESIGNATOR_IDENTIFIER);
    value->name = $1;
    $$ = (DesignatorTarget*) value;
  }
  | Designator LEFT_BRACKET Expression RIGHT_BRACKET {
    DEFINE_NODE(DesignatorIndexed, NODE_DESIGNATOR_INDEXED);
    value->source = $1;
    value->expr = $3;
    $$ = (DesignatorTarget*) value;
  }
  | Designator DOT IDENTIFIER {
    DEFINE_NODE(DesignatorField, NODE_DESIGNATOR_FIELD);
    value->source = $1;
    value->name = $3;
    $$ = (DesignatorTarget*) value;
  }
  ;

ArgumentList:
  Expression 
  | ArgumentList COMMA Expression
  | %empty {
    // $$ = NULL;
  }
  ;

AssignmentOperator:
  EQUAL
  | PLUS_EQUAL
  | MINUS_EQUAL
  | STAR_EQUAL
  | SLASH_EQUAL
  | PERCENT_EQUAL
  ;

ConditionalExpression:
  DisjunctionExpression 
  | DisjunctionExpression QUESTION Expression COLON DisjunctionExpression {
      DEFINE_NODE(Conditional, NODE_CONDITIONAL);
      value->condition = $1;
      value->ifTrue = $3;
      value->ifFalse = $5;
      $$ = (Expr*) value;
  }
  ;

DisjunctionExpression:
  ConjunctionExpression 
  | ConjunctionExpression OR ConjunctionExpression {
      DEFINE_NODE(Disjunction, NODE_DISJUNCTION);
      value->left = $1;
      value->right = $3;
      $$ = (Expr*) value;
    }
  ;

ConjunctionExpression:
  EqualityExpression 
  | EqualityExpression AND EqualityExpression {
      DEFINE_NODE(Conjunction, NODE_CONJUNCTION);
      value->left = $1;
      value->right = $3;
      $$ = (Expr*) value;
    }
  ;

EqualityExpression:
  OrderingExpression 
  | OrderingExpression EqualityOperator OrderingExpression {
      DEFINE_NODE(Equality, NODE_EQUALITY);
      value->left = $1;
      value->op = $2;
      value->right = $3;
      $$ = (Expr*) value; 
  }
  ;

EqualityOperator : EQUAL_EQUAL | BANG_EQUAL ;

OrderingExpression:
  AdditionExpression 
  | AdditionExpression OrderingOperator AdditionExpression {
      DEFINE_NODE(Ordering, NODE_ORDERING);
      value->left = $1;
      value->op = $2;
      value->right = $3;
      $$ = (Expr*) value;
  }
  ;

OrderingOperator: LESS | LESS_EQUAL | GREATER | GREATER_EQUAL ;

AdditionExpression:
  TermExpression 
  | AdditionExpression AdditionOperator TermExpression {
      DEFINE_NODE(Addition, NODE_ADDITION);
      value->left = $1;
      value->op = $2;
      value->right = $3;
      $$ = (Expr*) value;
  }
  ;

AdditionOperator: PLUS | MINUS;

TermExpression:
  FactorExpression 
  | TermExpression MultiplicationOperator FactorExpression {
      DEFINE_NODE(Term, NODE_TERM);
      value->left = $1;
      value->op = $2;
      value->right = $3;
      $$ = (Expr*) value;
    }
  ;

MultiplicationOperator: STAR | SLASH | PERCENT;

FactorExpression:
  ExponentExpression 
  | UnaryOperator ExponentExpression {
      DEFINE_NODE(Factor, NODE_FACTOR);
      value->op = $1;
      value->expr = $2;
      $$ = (Expr*) value;
  }
  ;

ExponentExpression:
  PrimaryExpression
  | PrimaryExpression STAR_STAR FactorExpression {
      DEFINE_NODE(Exponent, NODE_EXPONENT);
      value->left = $1;
      value->right = $3;
      $$ = (Expr*) value;
  }
  ;

UnaryOperator: BANG | MINUS ;

PrimaryExpression:
  NIL {
    DEFINE_NODE(Primitive, NODE_PRIMITIVE);
    value->type = NODE_PRIMITIVE_NIL;
    value->value = $1;
    $$ = (Expr*) value;
  }
  | NUMBER {
    DEFINE_NODE(Primitive, NODE_PRIMITIVE);
    value->type = NODE_PRIMITIVE_NUMBER;
    value->value = $1;
    $$ = (Expr*) value;
  }
  | STRING {
    DEFINE_NODE(Primitive, NODE_PRIMITIVE);
    value->type = NODE_PRIMITIVE_STRING;
    value->value = $1;
    $$ = (Expr*) value;
  }
  | Designator {
    DEFINE_NODE(DesignatorValue, NODE_DESIGNATOR_VALUE);
    value->source = (Designator*) $1;
    $$ = (Expr*) value;
  }
  | LEFT_PAREN ConditionalExpression RIGHT_PAREN {
    DEFINE_NODE(Group, NODE_GROUP);
    value->expr = $2;
    $$ = (Expr*) value;
  }
  | FunctionCall
  ;

  FunctionCall:
    Designator LEFT_PAREN ArgumentList RIGHT_PAREN {
    DEFINE_NODE(FunctionCall, NODE_FUNCTION_CALL);
    value->source = (Designator*) $1;
    value->argumentList = NULL;
  }
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
yyerror (YYLTYPE* loc, arena_t* arena, yyscan_t yyscanner, Node** result, char const *s)
{
  fprintf (stderr, "%s\n", s);
}

static const char* token_name(int t) {
  return yysymbol_name(YYTRANSLATE(t));
}
