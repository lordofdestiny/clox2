//
// Created by djumi on 10/29/2023.
//
#include <stdio.h>
#include <stdlib.h>

#include "../h/common.h"
#include "../h/value.h"
#include "../h/compiler.h"
#include "../h/scanner.h"
#include "../h/object.h"

#include <string.h>

#ifdef DEBUG_PRINT_CODE

#include "../h/debug.h"

#endif

typedef enum {
    PREC_NONE,
    PREC_ASSIGNMENT,    // =
    PREC_CONDITIONAL,   // ?:
    PREC_OR,            // or
    PREC_AND,           // and
    PREC_EQUALITY,      // == !=
    PREC_COMPARISON,    // < > <= >=
    PREC_TERM,          // + -
    PREC_FACTOR,        // * /
    PREC_UNARY,         // ! -
    PREC_CALL,          // ()
    PREC_PRIMARY
} Precedence;

typedef void (*ParseFn)(bool canAssign);

typedef struct {
    ParseFn prefix;
    ParseFn infix;
    Precedence precedence;
} ParseRule;

typedef struct {
    Token name;
    int depth;
    bool isCaptured;
} Local;

typedef struct {
    uint8_t index;
    bool isLocal;
} Upvalue;

typedef enum {
    TYPE_FUNCTION,
    TYPE_SCRIPT
} FunctionType;

typedef enum LoopType {
    LOOP_NONE,
    LOOP_LOOP,
} LoopType;

typedef struct Compiler {
    struct Compiler *enclosing;
    ObjFunction *function;
    FunctionType type;

    Local locals[UINT8_COUNT];
    int localCount;
    Upvalue upvalues[UINT8_COUNT];
    int scopeDepth;

    LoopType loopType;
    int innermostLoopStart;
    int innermostLoopScopeDepth;

    Table stringConstants;
} Compiler;


typedef struct {
    Token current;
    Token previous;
    bool hadError;
    bool panicMode;
} Parser;

Parser parser;
Compiler *current = NULL;

static Chunk *currentChunk() {
    return &current->function->chunk;
}

static void errorAt(Token *token, const char *message) {
    if (parser.panicMode) return;
    parser.panicMode = true;
    fprintf(stderr, "[line %d] Error", token->line);
    if (token->type == TOKEN_EOF) {
        fprintf(stderr, " at end");
    } else if (token->type == TOKEN_ERROR) {
        // Nothing
    } else {
        fprintf(stderr, " at '%.*s'", token->length, token->start);
    }

    fprintf(stderr, ": %s\n", message);
    parser.hadError = true;
}

static void error(const char *message) {
    errorAt(&parser.previous, message);
}

static void errorAtCurrent(const char *message) {
    errorAt(&parser.current, message);
}

static void advance() {
    parser.previous = parser.current;

    while (true) {
        parser.current = scanToken();
        if (parser.current.type != TOKEN_ERROR) break;

        errorAtCurrent(parser.current.start);
    }
}

static void consume(TokenType type, const char *message) {
    if (parser.current.type == type) {
        advance();
        return;
    }
    errorAtCurrent(message);
}

static bool check(TokenType type) {
    return parser.current.type == type;
}

static bool match(TokenType type) {
    if (!check(type)) return false;
    advance();
    return true;
}

static void emitByte(uint8_t byte) {
    writeChunk(currentChunk(), byte, parser.previous.line);
}

static void emitBytes(uint8_t byte1, uint8_t byte2) {
    emitByte(byte1);
    emitByte(byte2);
}

static void emitLoop(int loopStart) {
    emitByte(OP_LOOP);
    int offset = currentChunk()->count - loopStart + 2;
    if (offset > UINT16_MAX) error("Loop body too large");

    emitByte((offset >> 8) & 0xFF);
    emitByte(offset & 0xFF);
}

static int emitJump(uint8_t instruction) {
    emitByte(instruction);
    emitByte(0xFF);
    emitByte(0xFF);
    return currentChunk()->count - 2;
}

static void emitReturn() {
    emitByte(OP_NIL);
    emitByte(OP_RETURN);
}

static uint8_t makeConstant(Value value) {
    int constant = addConstant(currentChunk(), value);
    if (constant > UINT8_MAX) {
        error("Too many constants in chunk.");
        return 0;
    }

    return (uint8_t) constant;
}

static void emitConstant(Value value) {
    emitBytes(OP_CONSTANT, makeConstant(value));
}

static void patchJump(int offset) {
    int jump = currentChunk()->count - offset - 2;
    if (jump > UINT16_MAX) {
        error("Too much code to jump over");
    }

    currentChunk()->code[offset] = (jump >> 8) & 0xFF;
    currentChunk()->code[offset + 1] = jump & 0xFF;
}

static void initCompiler(Compiler *compiler, FunctionType type) {
    compiler->enclosing = current;
    compiler->function = NULL;
    compiler->type = type;
    compiler->localCount = 0;
    compiler->scopeDepth = 0;
    compiler->function = newFunction();
    current = compiler;

    if (type != TYPE_SCRIPT) {
        current->function->name = copyString(parser.previous.start,
                                             parser.previous.length);
    }

    Local *local = &current->locals[current->localCount++];
    local->depth = 0;
    local->isCaptured = false;
    local->name.start = "";
    local->name.length = 0;

    compiler->loopType = LOOP_NONE;
    compiler->innermostLoopStart = -1;
    compiler->innermostLoopScopeDepth = 0;

    initTable(&compiler->stringConstants);
}

static ObjFunction *endCompiler() {
    emitReturn();
    ObjFunction *function = current->function;
#ifdef DEBUG_PRINT_CODE
    if (!parser.hadError) {
        disassembleChunk(currentChunk(), function->name != NULL
                                         ? function->name->chars
                                         : "<script>");
    }
#endif

    freeTable(&current->stringConstants);
    current = current->enclosing;
    return function;
}

static void beginScope() {
    current->scopeDepth++;
}

static void endScope() {
    current->scopeDepth--;
    while (current->localCount > 0 &&
           current->locals[current->localCount - 1].depth > current->scopeDepth) {

        if (current->locals[current->localCount - 1].isCaptured) {
            emitByte(OP_CLOSE_UPVALUE);
        } else {
            emitByte(OP_POP);
        }
        current->localCount--;
    }
}


static void expression();

static void statement();

static void declaration();

static ParseRule *getRule(TokenType type);

static void parsePrecedence(Precedence precedence);

static uint8_t identifierConstant(Token *name) {
    ObjString *string = copyString(name->start, name->length);
    Value indexValue;
    if (tableGet(&current->stringConstants, string, &indexValue)) {
        // TODO free string with GCv
        return (uint8_t) AS_NUMBER(indexValue);
    }
    uint8_t index = makeConstant(OBJ_VAL((Obj *) string));
    tableSet(&current->stringConstants, string, NUMBER_VAL((double) index));
    return index;
}

static bool identifiersEqual(Token *a, Token *b) {
    if (a->length != b->length) return false;
    return memcmp(a->start, b->start, a->length) == 0;
}

static int resolveLocal(Compiler *compiler, Token *name) {
    for (int i = compiler->localCount - 1; i >= 0; i--) {
        Local *local = &compiler->locals[i];
        if (identifiersEqual(name, &local->name)) {
            if (local->depth == -1) {
                error("Can't read local variable in it's own initializer");
            }
            return i;
        }
    }
    return -1;
}

static int addUpvalue(Compiler *compiler, uint8_t index, bool isLocal) {
    int upvalueCount = compiler->function->upvalueCount;

    for (int i = 0; i < upvalueCount; i++) {
        Upvalue *upvalue = &compiler->upvalues[i];
        if (upvalue->index == index && upvalue->isLocal == isLocal) {
            return i;
        }
    }

    if (upvalueCount == UINT8_COUNT) {
        error("Too many closing variables in function.");
        return 0;
    }

    compiler->upvalues[upvalueCount].isLocal = isLocal;
    compiler->upvalues[upvalueCount].index = index;
    return compiler->function->upvalueCount++;
}

static int resolveUpvalue(Compiler *compiler, Token *name) {
    if (compiler->enclosing == NULL) return -1;

    int local = resolveLocal(compiler->enclosing, name);

    if (local != -1) {
        compiler->enclosing->locals[local].isCaptured = true;
        return addUpvalue(compiler, (uint8_t) local, true);
    }

    int upvalue = resolveUpvalue(compiler->enclosing, name);
    if (upvalue != -1) {
        return addUpvalue(compiler, (uint8_t) upvalue, false);
    }

    return -1;
}

static void addLocal(Token name) {
    if (current->localCount == UINT8_COUNT) {
        error("Too many local variables in function.");
        return;
    }
    Local *local = &current->locals[current->localCount++];
    local->name = name;
    local->depth = -1;
    local->isCaptured = false;
}


static void declareVariable() {
    if (current->scopeDepth == 0)return;

    Token *name = &parser.previous;
    for (int i = current->localCount - 1; i >= 0; i--) {
        Local *local = &current->locals[i];
        if (local->depth != -1 && local->depth < current->scopeDepth) {
            break;
        }

        if (identifiersEqual(name, &local->name)) {
            error("Already a variable with this name in this scope.");
        }
    }
    addLocal(*name);
}

static uint8_t argumentList() {
    uint8_t argCount = 0;
    if (!check(TOKEN_RIGHT_PAREN)) {
        do {
            expression();
            if (argCount == 255) {
                error("Can't have more than 255 arguments.");
            }
            argCount++;
        } while (match(TOKEN_COMMA));
    }
    consume(TOKEN_RIGHT_PAREN, "Expected ')' after argument list.");
    return argCount;
}

static uint8_t parseVariable(const char *errorMessage) {
    consume(TOKEN_IDENTIFIER, errorMessage);

    declareVariable();
    if (current->scopeDepth > 0) return 0;

    return identifierConstant(&parser.previous);
}

static void markInitialized() {
    if (current->scopeDepth == 0) return;
    current->locals[current->localCount - 1].depth = current->scopeDepth;
}

static void defineVariable(uint8_t global) {
    if (current->scopeDepth > 0) {
        markInitialized();
        return;
    }

    emitBytes(OP_DEFINE_GLOBAL, global);
}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "UnusedParameter"

static void and_(bool canAssign) {
#pragma clang diagnostic pop

    int endJump = emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP);
    parsePrecedence(PREC_AND);
    patchJump(endJump);
}

static void expression() {
    parsePrecedence(PREC_ASSIGNMENT);
}

static void block() {
    while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
        declaration();
    }

    consume(TOKEN_RIGHT_BRACE, "Expect '}' after block.");
}

static void function(FunctionType type) {
    Compiler compiler;
    initCompiler(&compiler, type);
    beginScope();

    consume(TOKEN_LEFT_PAREN, "Expect '(' after function name.");
    if (!check(TOKEN_RIGHT_PAREN)) {
        do {
            current->function->arity++;
            if (current->function->arity > 255) {
                errorAtCurrent("Can't have more than 255 parameters");
            }
            uint8_t constant = parseVariable("Expect parameter name.");
            defineVariable(constant);
        } while (match(TOKEN_COMMA));
    }
    consume(TOKEN_RIGHT_PAREN, "Expected ')' after parameters.");
    consume(TOKEN_LEFT_BRACE, "Expected '{' before function body.");

    block();

    ObjFunction *function = endCompiler(); // endScope() not needed because compiler ends here
    uint8_t functionConstant = makeConstant(OBJ_VAL((Obj *) function));
    if (function->upvalueCount > 0) {
        emitBytes(OP_CLOSURE, functionConstant);

        for (int i = 0; i < function->upvalueCount; i++) {
            emitByte(compiler.upvalues[i].isLocal ? 1 : 0);
            emitByte(compiler.upvalues[i].index);
        }
    } else {
        emitBytes(OP_CONSTANT, functionConstant);
    }
}

static void funDeclaration() {
    uint8_t global = parseVariable("Expect function name");
    markInitialized();
    function(TYPE_FUNCTION);
    defineVariable(global);
}


static void varDeclaration() {
    uint8_t global = parseVariable("Expect variable name");

    if (match(TOKEN_EQUAL)) {
        expression();
    } else {
        emitByte(OP_NIL);
    }
    consume(TOKEN_SEMICOLON,
            "Expect ';' after variable declaration.");

    defineVariable(global);
}

static void expressionStatement() {
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after expression.");
    emitByte(OP_POP);
}

#define MAX_BREAK_LOCATIONS 255

typedef struct BreakLocations {
    struct BreakLocations *prev;
    int count;
    int locations[MAX_BREAK_LOCATIONS];
} BreakLocations;
BreakLocations *currentBreakLocations = NULL;

void initBreakLocations(BreakLocations *locations) {
    locations->count = 0;
    locations->prev = currentBreakLocations;
    currentBreakLocations = locations;
}

void leaveBreakLocations(BreakLocations *locations) {
    // Patch locations for break statement
    for (int i = 0; i < locations->count; i++) {
        patchJump(locations->locations[i]);
    }
    currentBreakLocations = locations->prev;
}

void addBreakLocation(int location) {
    BreakLocations *loc = currentBreakLocations;
    if (loc == NULL || loc->count >= MAX_BREAK_LOCATIONS) return;
    loc->locations[loc->count] = location;
    loc->count++;
}

static void forStatement() {
    beginScope();

    // 1: Grab the name and slot of the loop variable, so that we can refer to it later.
    int loopVariable = -1;
    Token loopVariableName;
    loopVariableName.start = NULL;
    // end

    consume(TOKEN_LEFT_PAREN, "Expect '(' after for.");

    if (match(TOKEN_SEMICOLON)) {
        // No initializer
    } else if (match(TOKEN_VAR)) {
        // 1: Grab the ame of the loop variable
        loopVariableName = parser.current;
        varDeclaration();
        // and get its slot
        loopVariable = current->localCount - 1;
    } else {
        expressionStatement();
    }

    LoopType surroundingLoopType = current->loopType;
    int surroundingLoopStart = current->innermostLoopStart;
    int surroundingLoopScopeDepth = current->innermostLoopScopeDepth;
    current->loopType = LOOP_LOOP;
    current->innermostLoopStart = currentChunk()->count;
    current->innermostLoopScopeDepth = current->scopeDepth;

    BreakLocations locations;
    initBreakLocations(&locations);

    int exitJump = -1;
    if (!match(TOKEN_SEMICOLON)) {
        expression();
        consume(TOKEN_SEMICOLON, "Expect ';' after loop condition.");

        // Jump out of the loop if the condition is false
        exitJump = emitJump(OP_JUMP_IF_FALSE);
        emitByte(OP_POP); // Condition
    }

    if (!match(TOKEN_RIGHT_PAREN)) {
        int bodyJump = emitJump(OP_JUMP);

        int incrementStart = currentChunk()->count;
        expression();
        emitByte(OP_POP);
        consume(TOKEN_RIGHT_PAREN, "Expected ')' after for clauses.");

        emitLoop(current->innermostLoopStart);
        current->innermostLoopStart = incrementStart;
        patchJump(bodyJump);
    }

    int innerVariable = -1;
    if (loopVariable != -1) {
        // 1: Create a scope for the copy ...
        beginScope();
        // 1: Define a new variable initialized with the current value of the loop
        emitBytes(OP_GET_LOCAL, (uint8_t) loopVariable);
        addLocal(loopVariableName);
        markInitialized();
        // 1: Keep the track of it's slot
        innerVariable = current->localCount - 1;
    }

    statement();

    // Is  this in the right place ?
    // break should execute the pop ??
    // 3: If the loop declares a variable...
    if (loopVariable != -1) {
        emitBytes(OP_GET_LOCAL, (uint8_t) innerVariable);
        emitBytes(OP_SET_LOCAL, (uint8_t) loopVariable);
        emitByte(OP_POP);

        // 4: Close  the temporary scope for the copy of loop variable
        endScope();
    }
    emitLoop(current->innermostLoopStart);


    if (exitJump != -1) {
        patchJump(exitJump);
        emitByte(OP_POP); // Condition;
    }

    current->loopType = surroundingLoopType;
    current->innermostLoopStart = surroundingLoopStart;
    current->innermostLoopScopeDepth = surroundingLoopScopeDepth;

    leaveBreakLocations(&locations);

    endScope();
}

static void breakStatement() {
    if (current->loopType == LOOP_NONE && currentBreakLocations == NULL) {
        error("Can't use 'break' outside a loop/switch statements.");
    }

    if (currentBreakLocations->count == MAX_BREAK_LOCATIONS) {
        error("Too many break statements in the loop");
    }

    consume(TOKEN_SEMICOLON, "Expect ';' after 'break'.");

    // Pop locals
//    if (current->loopType == LOOP_LOOP) {
    for (int i = current->localCount - 1;
         i >= 0 && current->locals[i].depth > current->innermostLoopScopeDepth;
         i--) {
        if (current->locals[i].isCaptured) {
            emitByte(OP_CLOSE_UPVALUE);
        } else {
            emitByte(OP_POP);
        }
    }
//    }

    int breakJump = emitJump(OP_JUMP);
    addBreakLocation(breakJump);
}

static void continueStatement() {
    if (current->loopType == LOOP_NONE) {
        error("Can't use 'continue' outside of a loop.");
    }

    consume(TOKEN_SEMICOLON, "Expect ';' after 'continue'.");

    // Pop locals
    for (int i = current->localCount - 1;
         i >= 0 && current->locals[i].depth > current->innermostLoopScopeDepth;
         i--) {
        if (current->locals[i].isCaptured) {
            emitByte(OP_CLOSE_UPVALUE);
        } else {
            emitByte(OP_POP);
        }
    }

    emitLoop(current->innermostLoopStart);
}

static void ifStatement() {
    consume(TOKEN_LEFT_PAREN, "Expect '(' after if.");
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

    int thenJump = emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP);
    statement();

    int elseJump = emitJump(OP_JUMP);
    patchJump(thenJump);
    emitByte(OP_POP);
    if (match(TOKEN_ELSE)) statement();
    patchJump(elseJump);
}

static void printStatement() {
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after value.");
    emitByte(OP_PRINT);
}

static void returnStatement() {
    if (current->type == TYPE_SCRIPT) {
        error("Can't return from top-level code.");
    }

    if (match(TOKEN_SEMICOLON)) {
        emitReturn();
    } else {
        expression();
        consume(TOKEN_SEMICOLON, "Expected ';' after return statement.");
        emitByte(OP_RETURN);
    }
}

static void switchStatement() {
    consume(TOKEN_LEFT_PAREN, "Expected '(' after switch");
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expected ')' after switch expression");
    consume(TOKEN_LEFT_BRACE, "Expected '{' before switch cases.");

    // 0: before all cases
    // 1: before default
    // 2: after default
    int state = 0;
    int previousCaseSkip = -1;
    int previousFallthroughLocation = -1;

    // Reserve a stack space for the switch helper location
    // That is used to save the switched expression result
    current->localCount++;

    BreakLocations locations;
    LoopType surroundingLoopType = current->loopType;
    current->loopType = LOOP_NONE;
    initBreakLocations(&locations);

    while (!match(TOKEN_RIGHT_BRACE) && !match(TOKEN_EOF)) {
        // If line is a line wth a case or a default
        if (match(TOKEN_CASE) || match(TOKEN_DEFAULT)) {
            TokenType caseType = parser.previous.type;

            if (state == 2) {
                error("Can't have another case or default after default case ");
            }

            if (state == 1) {
                // Prepare fallthrough jump for previous case (to this one)
                previousFallthroughLocation = emitJump(OP_JUMP);
                // Patch where previous case skips on unmatched value
                patchJump(previousCaseSkip);
                emitByte(OP_POP); // Pop old check
            }

            if (caseType == TOKEN_CASE) {
                state = 1;
                // Duplicate expression result for consecutive comparisons
                emitByte(OP_DUP);
                expression();

                consume(TOKEN_COLON, "Expect ':' after case value");

                emitByte(OP_EQUAL);
                // Prepare jump to next case if this one fails
                previousCaseSkip = emitJump(OP_JUMP_IF_FALSE);
                // Pop test result if case is matched
                emitByte(OP_POP);
            } else {
                state = 2;
                consume(TOKEN_COLON, "Expect ':' after default.");
                previousCaseSkip = -1;
            }
            // Patch where previous case falls through after it's body is done
            if (previousFallthroughLocation != -1) {
                patchJump(previousFallthroughLocation);
            }
        } else {
            if (state == 0) {
                error("Can't have statements before any case");
            }
            statement();
        }
    }
    // If there was no default case
    previousFallthroughLocation = emitJump(OP_JUMP);
    if (state == 1) {
        emitByte(OP_POP);
        patchJump(previousCaseSkip);
        emitByte(OP_POP);
    }
    patchJump(previousFallthroughLocation);

    leaveBreakLocations(&locations);
    current->loopType = surroundingLoopType;
    current->localCount--;

    emitByte(OP_POP);
}

static void whileStatement() {
    LoopType surroundingLoopType = current->loopType;
    int surroundingLoopStart = current->innermostLoopStart;
    int surroundingLoopScopeDepth = current->innermostLoopScopeDepth;
    current->loopType = LOOP_LOOP;
    current->innermostLoopStart = currentChunk()->count;
    current->innermostLoopScopeDepth = current->scopeDepth;


    BreakLocations locations;
    initBreakLocations(&locations);

    consume(TOKEN_LEFT_PAREN, "Expect '(' after while.");
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

    int exitJump = emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP);
    statement();
    emitLoop(current->innermostLoopStart);

    patchJump(exitJump);
    emitByte(OP_POP);

    leaveBreakLocations(&locations);

    current->loopType = surroundingLoopType;
    current->innermostLoopStart = surroundingLoopStart;
    current->innermostLoopScopeDepth = surroundingLoopScopeDepth;
}

static void synchronize() {
    parser.panicMode = false;

    while (parser.current.type != TOKEN_EOF) {
        if (parser.previous.type == TOKEN_SEMICOLON) return;
        switch (parser.current.type) {
        case TOKEN_CLASS:
        case TOKEN_FUN:
        case TOKEN_VAR:
        case TOKEN_FOR:
        case TOKEN_IF:
        case TOKEN_WHILE:
        case TOKEN_PRINT:
        case TOKEN_RETURN:return;
        default:; // Do nothing
        }

        advance();
    }
}

static void declaration() {
    if (match(TOKEN_FUN)) {
        funDeclaration();
    } else if (match(TOKEN_VAR)) {
        varDeclaration();
    } else {
        statement();
    }

    if (parser.panicMode) synchronize();
}

static void statement() {
    if (match(TOKEN_PRINT)) {
        printStatement();
    } else if (match(TOKEN_BREAK)) {
        breakStatement();
    } else if (match(TOKEN_CONTINUE)) {
        continueStatement();
    } else if (match(TOKEN_IF)) {
        ifStatement();
    } else if (match(TOKEN_RETURN)) {
        returnStatement();
    } else if (match(TOKEN_FOR)) {
        forStatement();
    } else if (match(TOKEN_SWITCH)) {
        switchStatement();
    } else if (match(TOKEN_WHILE)) {
        whileStatement();
    } else if (match(TOKEN_LEFT_BRACE)) {
        beginScope();
        block();
        endScope();
    } else {
        expressionStatement();
    }
}


/*
static void conditional() {
    parsePrecedence(compiler, PREC_CONDITIONAL);
    consume(compiler, TOKEN_COLON, "Expect ':' after then branch of conditional operator.");
    parsePrecedence(compiler, PREC_ASSIGNMENT);
}
*/

#pragma clang diagnostic push
#pragma ide diagnostic ignored "UnusedParameter"

static void binary(bool canAssign) {
#pragma clang diagnostic pop

    TokenType operatorType = parser.previous.type;
    ParseRule *rule = getRule(operatorType);
    parsePrecedence((Precedence) (rule->precedence + 1));

    switch (operatorType) {
    case TOKEN_PLUS: emitByte(OP_ADD);
        break;
    case TOKEN_MINUS:emitBytes(OP_NEGATE, OP_ADD);
        break;
    case TOKEN_STAR: emitByte(OP_MULTIPLY);
        break;
    case TOKEN_SLASH: emitByte(OP_DIVIDE);
        break;
    case TOKEN_BANG_EQUAL: emitBytes(OP_EQUAL, OP_NOT);
        break;
    case TOKEN_EQUAL_EQUAL: emitByte(OP_EQUAL);
        break;
    case TOKEN_GREATER: emitByte(OP_GREATER);
        break;
    case TOKEN_GREATER_EQUAL: emitBytes(OP_LESS, OP_NOT);
        break;
    case TOKEN_LESS: emitByte(OP_LESS);
        break;
    case TOKEN_LESS_EQUAL: emitBytes(OP_GREATER, OP_NOT);
        break;
    default:return; // Unreachable
    }
}


#pragma clang diagnostic push
#pragma ide diagnostic ignored "UnusedParameter"

static void call(bool canAssign) {
#pragma clang diagnostic pop
    uint8_t argCount = argumentList();
    emitBytes(OP_CALL, argCount);
}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "UnusedParameter"

static void literal(bool canAssign) {
#pragma clang diagnostic pop

    switch (parser.previous.type) {
    case TOKEN_FALSE: emitByte(OP_FALSE);
        break;
    case TOKEN_NIL: emitByte(OP_NIL);
        break;
    case TOKEN_TRUE: emitByte(OP_TRUE);
        break;
    default: return; // Unreachable
    }
}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "UnusedParameter"

static void grouping(bool canAssign) {
#pragma clang diagnostic

    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression");
}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "UnusedParameter"

static void number(bool canAssign) {
#pragma clang diagnostic pop

    double value = strtod(parser.previous.start, NULL);
    emitConstant(NUMBER_VAL(value));
}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "UnusedParameter"

static void or_(bool canAssign) {
#pragma clang diagnostic pop

    int elseJump = emitJump(OP_JUMP_IF_FALSE);
    int endJump = emitJump(OP_JUMP);
    patchJump(elseJump);
    emitByte(OP_POP);

    parsePrecedence(PREC_OR);
    patchJump(endJump);
}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "UnusedParameter"

static void string(bool canAssign) {
#pragma clang diagnostic pop

    emitConstant(OBJ_VAL((Obj *) copyString(parser.previous.start + 1,
                                            parser.previous.length - 2)));
}

static void namedVariable(Token name, bool canAssign) {
    uint8_t getOp, setOp;
    int arg = resolveLocal(current, &name);

    if (arg != -1) {
        getOp = OP_GET_LOCAL;
        setOp = OP_SET_LOCAL;
    } else if ((arg = resolveUpvalue(current, &name)) != -1) {
        getOp = OP_GET_UPVALUE;
        setOp = OP_SET_UPVALUE;
    } else {
        arg = identifierConstant(&name);
        getOp = OP_GET_GLOBAL;
        setOp = OP_SET_GLOBAL;
    }

    if (canAssign && match(TOKEN_EQUAL)) {
        expression();
        emitBytes(setOp, (uint8_t) arg);
    } else {
        emitBytes(getOp, (uint8_t) arg);
    }
}

static void variable(bool canAssign) {
    namedVariable(parser.previous, canAssign);
}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "UnusedParameter"

static void unary(bool canAssign) {
#pragma clang diagnostic pop

    TokenType operatorType = parser.previous.type;

    // Compile the operand.
    parsePrecedence(PREC_UNARY);

    // Emit the operand instruction
    switch (operatorType) {
    case TOKEN_BANG: emitByte(OP_NOT);
        break;
    case TOKEN_MINUS: emitByte(OP_NEGATE);
        break;
    default:return; // Unreachable
    }
}

ParseRule rules[] = {
        [TOKEN_LEFT_PAREN] = {grouping, call, PREC_CALL},
        [TOKEN_RIGHT_PAREN] = {NULL, NULL, PREC_NONE},
        [TOKEN_LEFT_BRACE] = {NULL, NULL, PREC_NONE},
        [TOKEN_RIGHT_BRACE] = {NULL, NULL, PREC_NONE},
        [TOKEN_COMMA] = {NULL, NULL, PREC_NONE},
        [TOKEN_DOT] = {NULL, NULL, PREC_NONE},
        [TOKEN_MINUS] = {unary, binary, PREC_TERM},
        [TOKEN_PLUS] = {NULL, binary, PREC_TERM},
        [TOKEN_SEMICOLON] = {NULL, NULL, PREC_NONE},
        [TOKEN_SLASH] = {NULL, binary, PREC_FACTOR},
        [TOKEN_STAR] = {NULL, binary, PREC_FACTOR},
        [TOKEN_BANG] = {unary, NULL, PREC_NONE},
        [TOKEN_BANG_EQUAL]    = {NULL, binary, PREC_EQUALITY},
        [TOKEN_EQUAL]         = {NULL, NULL, PREC_NONE},
        [TOKEN_EQUAL_EQUAL]   = {NULL, binary, PREC_EQUALITY},
        [TOKEN_GREATER]       = {NULL, binary, PREC_COMPARISON},
        [TOKEN_GREATER_EQUAL] = {NULL, binary, PREC_COMPARISON},
        [TOKEN_LESS]          = {NULL, binary, PREC_COMPARISON},
        [TOKEN_LESS_EQUAL]    = {NULL, binary, PREC_COMPARISON},
        [TOKEN_IDENTIFIER]    = {variable, NULL, PREC_NONE},
        [TOKEN_STRING]        = {string, NULL, PREC_NONE},
        [TOKEN_NUMBER]        = {number, NULL, PREC_NONE},
        [TOKEN_AND]           = {NULL, and_, PREC_AND},
        [TOKEN_CLASS]         = {NULL, NULL, PREC_NONE},
        [TOKEN_ELSE]          = {NULL, NULL, PREC_NONE},
        [TOKEN_FALSE]         = {literal, NULL, PREC_NONE},
        [TOKEN_FOR]           = {NULL, NULL, PREC_NONE},
        [TOKEN_FUN]           = {NULL, NULL, PREC_NONE},
        [TOKEN_IF]            = {NULL, NULL, PREC_NONE},
        [TOKEN_NIL]           = {literal, NULL, PREC_NONE},
        [TOKEN_OR]            = {NULL, or_, PREC_OR},
        [TOKEN_PRINT]         = {NULL, NULL, PREC_NONE},
        [TOKEN_RETURN]        = {NULL, NULL, PREC_NONE},
        [TOKEN_SUPER]         = {NULL, NULL, PREC_NONE},
        [TOKEN_THIS]          = {NULL, NULL, PREC_NONE},
        [TOKEN_TRUE]          = {literal, NULL, PREC_NONE},
        [TOKEN_VAR]           = {NULL, NULL, PREC_NONE},
        [TOKEN_WHILE]         = {NULL, NULL, PREC_NONE},
        [TOKEN_ERROR]         = {NULL, NULL, PREC_NONE},
        [TOKEN_EOF]           = {NULL, NULL, PREC_NONE},
};


static void parsePrecedence(Precedence precedence) {
    advance();
    ParseFn prefixRule = getRule(parser.previous.type)->prefix;
    if (prefixRule == NULL) {
        error("Expect expression.");
        return;
    }

    bool canAssign = precedence <= PREC_ASSIGNMENT;
    prefixRule(canAssign);

    while (precedence <= getRule(parser.current.type)->precedence) {
        advance();
        ParseFn infixRule = getRule(parser.previous.type)->infix;
        infixRule(canAssign);
    }

    if (canAssign && match(TOKEN_EQUAL)) {
        error("Invalid assignment target");
    }
}

static ParseRule *getRule(TokenType type) {
    return &rules[type];
}

ObjFunction *compile(const char *source) {
    initScanner(source);
    Compiler compiler;
    initCompiler(&compiler, TYPE_SCRIPT);

    parser.hadError = false;
    parser.panicMode = false;

    advance();

    while (!match(TOKEN_EOF)) {
        declaration();
    }

    ObjFunction *function = endCompiler();
    return parser.hadError ? NULL : function;
}