#include <stdio.h>

#include "token.h"
#include "Expr.h"
#include "Statement.h"

#define ENUM_NODE_ITEM(type, desc) \
    static void print##desc(FILE* file, const desc* expr); 

    ENUM_NODE_TYPE_LIST
#undef ENUM_NODE_ITEM

typedef void (*ExprPrintFn)(FILE* file, const Node*);

typedef struct ExprVt {
    ExprPrintFn print;
    size_t size;
} ExprVt;

ExprVt exprVT[] = {
#define ENUM_NODE_ITEM(type, desc) \
    [NODE_##type] = { \
        .print = (ExprPrintFn) print##desc, \
    },

    ENUM_NODE_TYPE_LIST
#undef ENUM_NODE_ITEM
};

#define NODE_PRINT(file, value) \
    exprVT[((Node*) value)->type].print(file, (Node*) value)

Node* allocateNode(arena_t* arena, size_t size, NodeType type) {
    Node* node = arena_alloc(arena, size);
    node->type = type;
    return node;
}

void printNode(FILE* file, const Node* node) {
    NODE_PRINT(file, node);
}

void printToken(FILE* file, const Token* token) {
    fprintf(file, "%.*s", token->length, token->chars);
}

void printFunctionCall(FILE* file, const FunctionCall* functionCall) {
    NODE_PRINT(file, functionCall->source);
    fprintf(file, "(");
    // NODE_PRINT(designator->argumentList);
    fprintf(file, "...arguments...");
    fprintf(file, ")");
}

void printDesignatorValue(FILE* file, const DesignatorValue* designatorValue) {
    NODE_PRINT(file, designatorValue->source);
}

void printPrimitive(FILE* file, const Primitive* primitive) {
    printToken(file, &primitive->value);
}

void printGroup(FILE* file, const Group* group) {
    fprintf(file, "(");
    NODE_PRINT(file, group->expr);
    fprintf(file, ")");
}

void printExponent(FILE* file, const Exponent* exponent) {
    fprintf(file, "(");
    fprintf(file, "**");
    fprintf(file, " ");
    NODE_PRINT(file, exponent->left);
    fprintf(file, " ");
    NODE_PRINT(file, exponent->right);
    fprintf(file, ")");
}

void printFactor(FILE* file, const Factor* factor){
    fprintf(file, "(");
    printToken(file, &factor->op);
    fprintf(file, " ");
    NODE_PRINT(file, factor->expr);
    fprintf(file, ")");
}

void printTerm(FILE* file, const Term* term) {
    fprintf(file, "(");
    printToken(file, &term->op);
    fprintf(file, " ");
    NODE_PRINT(file, term->left);
    fprintf(file, " ");
    NODE_PRINT(file, term->right);
    fprintf(file, ")");
}

void printAddition(FILE* file, const Addition* addition) {
    fprintf(file, "(");
    printToken(file, &addition->op);
    fprintf(file, " ");
    NODE_PRINT(file, addition->left);
    fprintf(file, " ");
    NODE_PRINT(file, addition->right);
    fprintf(file, ")");
}

void printOrdering(FILE* file, const Ordering* ordering) {
    fprintf(file, "(");
    printToken(file, &ordering->op);
    fprintf(file, " ");
    NODE_PRINT(file, ordering->left);
    fprintf(file, " ");
    NODE_PRINT(file, ordering->right);
    fprintf(file, ")");
}

void printDesignatorIdentifier(FILE* file, const DesignatorIdentifier* designator) {
    printToken(file, &designator->name);
}

void printDesignatorIndexed(FILE* file, const DesignatorIndexed* designator) {
    NODE_PRINT(file, designator->source);
    fprintf(file, "[");
    NODE_PRINT(file, designator->expr);
    fprintf(file, "]");
}

void printDesignatorField(FILE* file, const DesignatorField* designator) {
    NODE_PRINT(file, designator->source);
    fprintf(file, " . ");
    printToken(file, &designator->name);
}

void printDesignatorCall(FILE* file, const DesignatorCall* designator) {
    NODE_PRINT(file, designator->functionCall);
}

void printEquality(FILE* file, const Equality* expr) {
    fprintf(file, "(");
    printToken(file, &expr->op);
    fprintf(file, " ");
    NODE_PRINT(file, expr->left);
    fprintf(file, " ");
    NODE_PRINT(file, expr->right);
    fprintf(file, ")");
}

void printConjunction(FILE* file, const Conjunction* expr) {
    fprintf(file, "(and ");
    NODE_PRINT(file, expr->left);
    fprintf(file, " ");
    NODE_PRINT(file, expr->right);
    fprintf(file, ")");
}

void printDisjunction(FILE* file, const Disjunction* expr) {
    fprintf(file, "(оr ");
    NODE_PRINT(file, expr->left);
    fprintf(file, " ");
    NODE_PRINT(file, expr->right);
    fprintf(file, ")");
}

void printConditional(FILE* file, const Conditional* expr) {
    fprintf(file, "(? ");
    NODE_PRINT(file, expr->condition);
    fprintf(file, " ");
    NODE_PRINT(file, expr->ifTrue);
    fprintf(file, " ");
    NODE_PRINT(file, expr->ifFalse);
    fprintf(file, ")");
}

void printAssignment(FILE* file, const Assignment* expr) {
    fprintf(file, "(= ");
    NODE_PRINT(file, expr->target);
    fprintf(file, " ");
    NODE_PRINT(file, expr->expr);
    fprintf(file, ")");
}

void printExpressionStatement(FILE* file, const ExpressionStatement* stmt){
    NODE_PRINT(file, stmt->expr);
    fprintf(file, ";");
}
