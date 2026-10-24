#ifndef __NODE_STATEMENT_H__
#define __NODE_STATEMENT_H__

#include <common/arena.h>

#include "node.h"

typedef struct Statement {
    Node base;
} Statement;

typedef struct {
    Statement base;
    Expr* expr;
} ExpressionStatement;

#endif // __NODE_STATEMENT_H__