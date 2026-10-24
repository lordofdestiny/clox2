#ifndef __PARSER_NODE_H__
#define __PARSER_NODE_H__

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

typedef struct Expr Expr;

typedef struct Statement Statement;

#endif // __PARSER_NODE_H__