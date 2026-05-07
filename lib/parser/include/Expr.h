#ifndef __NODE_EXPR_H__
#define __NODE_EXPR_H__

#include <stdio.h>

#include <common/arena.h>

#include "token.h"

struct Statement;

#define ENUM_NODE_TYPE_LIST \
    ENUM_NODE_ITEM(PRIMITIVE, Primitive) \
    ENUM_NODE_ITEM(FUNCTION_CALL, FunctionCall) \
    ENUM_NODE_ITEM(DESIGNATOR_VALUE, DesignatorValue) \
    ENUM_NODE_ITEM(DESIGNATOR_IDENTIFIER, DesignatorIdentifier) \
    ENUM_NODE_ITEM(DESIGNATOR_INDEXED, DesignatorIndexed) \
    ENUM_NODE_ITEM(DESIGNATOR_FIELD, DesignatorField) \
    ENUM_NODE_ITEM(DESIGNATOR_CALL, DesignatorCall) \
    ENUM_NODE_ITEM(GROUP, Group) \
    ENUM_NODE_ITEM(EXPONENT, Exponent) \
    ENUM_NODE_ITEM(FACTOR, Factor) \
    ENUM_NODE_ITEM(TERM, Term) \
    ENUM_NODE_ITEM(ADDITION, Addition) \
    ENUM_NODE_ITEM(ORDERING, Ordering) \
    ENUM_NODE_ITEM(EQUALITY, Equality) \
    ENUM_NODE_ITEM(CONJUNCTION, Conjunction) \
    ENUM_NODE_ITEM(DISJUNCTION, Disjunction) \
    ENUM_NODE_ITEM(CONDITIONAL, Conditional) \
    ENUM_NODE_ITEM(ASSIGNMENT, Assignment) \
    ENUM_NODE_ITEM(STATEMENT, ExpressionStatement)

#define ALLOCATE_NODE(arena, type, nodeType) \
    allocateNode(arena, sizeof(type), nodeType)

typedef enum NodeType {
#define ENUM_NODE_ITEM(type, desc) NODE_##type,
    ENUM_NODE_TYPE_LIST
#undef ENUM_NODE_ITEM
} NodeType;

typedef struct Node {
    NodeType type;
} Node;

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
    Token value;
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
    Token op;
} Factor;

typedef struct Term Term;

typedef struct Term {
    Expr base;
    Expr* left;
    Expr* right;
    Token op;
} Term;

typedef struct Addition Addition;

typedef struct Addition { 
    Expr base;
    Expr* left;
    Expr* right;
    Token op;
} Addition;

typedef struct Ordering {
    Expr base;
    Expr* left;  
    Expr* right;
    Token op;
} Ordering;

typedef struct Equality {
    Expr base;
    Expr* left;  
    Expr* right;
    Token op;
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
    Token name;
} DesignatorIdentifier;

typedef struct DesignatorIndexed {
    DesignatorTarget base;
    DesignatorTarget* source;
    Expr* expr;
} DesignatorIndexed;

typedef struct DesignatorField {
    DesignatorTarget base;
    DesignatorTarget* source;
    Token name;
} DesignatorField;

typedef struct DesignatorCall {
    Designator base;
    FunctionCall* functionCall;
} DesignatorCall;

void printToken(FILE* file, const Token* token);

void printNode(FILE* file, const Node* expr);

Node* allocateNode(arena_t* arena, size_t size, NodeType type);

#endif // __NODE_EXPR_H__
