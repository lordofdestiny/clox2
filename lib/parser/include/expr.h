#ifndef __PARSER_EXPR_H__
#define __PARSER_EXPR_H__

#include <stdio.h>

#include <common/arena.h>

#include "node.h"

#include "token.h"

typedef struct Expr {
    Node base;
} Expr;

typedef enum PrimitiveType {
    NODE_PRIMITIVE_NUMBER,
    NODE_PRIMITIVE_NIL,
    NODE_PRIMITIVE_STRING,
} PrimitiveType;

typedef struct Primitive {
    Node base;
    PrimitiveType type;
    Token* value;
} Primitive;

typedef struct Designator Designator;
typedef struct ArgumentList ArgumentList;

typedef struct FunctionCall {
    Expr base;
    Designator* source;
    ArgumentList* argumentList;
} FunctionCall;

typedef struct DesignatorValue {
    Expr base;
    Designator* source;
} DesignatorValue;

typedef struct Group {
    Expr base;
    Expr* expr;
} Group;

typedef struct Exponent {
    Expr base;
    Expr* left;
    Expr* right;
} Exponent;

typedef struct Factor Factor;

typedef struct Factor {
    Expr base;
    Expr* expr;
    Token* op;
} Factor;

typedef struct Term Term;

typedef struct Term {
    Expr base;
    Expr* left;
    Expr* right;
    Token* op;
} Term;

typedef struct Addition Addition;

typedef struct Addition { 
    Expr base;
    Expr* left;
    Expr* right;
    Token* op;
} Addition;

typedef struct Ordering {
    Expr base;
    Expr* left;  
    Expr* right;
    Token* op;
} Ordering;

typedef struct Equality {
    Expr base;
    Expr* left;  
    Expr* right;
    Token* op;
} Equality;

typedef struct Conjunction {
    Expr base;
    Expr* left;
    Expr* right;
} Conjunction;

typedef struct Disjunction {
    Expr base;
    Expr* left;
    Expr* right;
} Disjunction;

typedef struct Conditional {
    Expr base;
    Expr* condition;
    Expr* ifTrue;
    Expr* ifFalse;
} Conditional;

typedef struct Assignment {
    Expr base;
    Designator* target;
    Expr* expr;
} Assignment;

typedef struct Designator {
    Node base;
} Designator;

typedef struct DesignatorTarget {
    Designator base;
} DesignatorTarget;

typedef struct DesignatorIdentifier {
    DesignatorTarget base;
    Token* name;
} DesignatorIdentifier;

typedef struct DesignatorIndexed {
    DesignatorTarget base;
    DesignatorTarget* source;
    Expr* expr;
} DesignatorIndexed;

typedef struct DesignatorField {
    DesignatorTarget base;
    DesignatorTarget* source;
    Token* name;
} DesignatorField;

typedef struct DesignatorCall {
    Designator base;
    FunctionCall* functionCall;
} DesignatorCall;

void printToken(FILE* file, const Token* token);

void printNode(FILE* file, const Node* expr);

Node* allocateNode(arena_t* arena, size_t size, NodeType type);

#endif // __PARSER_EXPR_H__
