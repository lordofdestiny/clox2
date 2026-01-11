#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "common.h"
#include "value.h"
#include "compiler.h"
#include "scanner.h"

#include <string.h>

#ifdef DEBUG_PRINT_CODE

#include "debug.h"

#endif

#include "memory.h"

typedef enum {
    PREC_NONE,
    PREC_ASSIGNMENT,    // = += -= *= /= %=
    PREC_CONTAINER,     // [ element1, ... , elementN ]
    PREC_CONDITIONAL,   // ?:
    PREC_OR,            // or
    PREC_AND,           // and
    PREC_EQUALITY,      // == !=
    PREC_COMPARISON,    // < > <= >=
    PREC_TERM,          // + -
    PREC_FACTOR,        // * /
    PREC_EXPONENT,      // **
    PREC_UNARY,         // ! -
    PREC_CALL_INDEX,    // func(a, ... ,z), arr[i]
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
    TYPE_LAMBDA,
    TYPE_FUNCTION,
    TYPE_INITIALIZER,
    TYPE_STATIC_METHOD,
    TYPE_METHOD,
    TYPE_SCRIPT
} FunctionType;

typedef enum LoopType {
    LOOP_NONE,
    LOOP_LOOP,
} LoopType;

typedef struct Compiler {
    struct Compiler* enclosing;
    ObjFunction* function;
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

static bool* compilerReplMode() {
    static bool value;
    return &value;
}

bool isRepl() {
    return *compilerReplMode();
}

void setRepl(bool isRepl) {
    *compilerReplMode() = isRepl;
}

typedef struct ClassCompiler {
    struct ClassCompiler* enclosing;
    bool hasSuperclass;
    // Tables for checking for duplicate members
    // No need to be marked by gc because they hold keys
    // that are functions constants. That means they get marked
    // when the function is blackened
    Table methods;
    Table staticMembers;
} ClassCompiler;

typedef struct {
    Token current;
    Token previous;
    bool hadError;
    bool panicMode;
} Parser;

Parser parser;
Compiler* current = NULL;
ClassCompiler* currentClass = NULL;

static Chunk* currentChunk() {
    return &current->function->chunk;
}

static void errorAt(const Token* token, const char* message) {
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

static void error(const char* message) {
    errorAt(&parser.previous, message);
}

static void errorAtCurrent(const char* message) {
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

static void consume(const TokenType type, const char* message) {
    if (parser.current.type == type) {
        advance();
        return;
    }
    errorAtCurrent(message);
}

static void consumeReplOptional(const TokenType type, const char* message) {
    if (parser.current.type == type) {
        advance();
        return;
    }
    if(current->enclosing != NULL || !isRepl()) {
        errorAtCurrent(message);
    }
}

static bool check(const TokenType type) {
    return parser.current.type == type;
}

static bool match(const TokenType type) {
    if (!check(type)) return false;
    advance();
    return true;
}

static void emitByte(const uint8_t byte) {
    writeChunk(currentChunk(), byte, parser.previous.line);
}

static void emitBytes(const uint8_t byte1, const uint8_t byte2) {
    emitByte(byte1);
    emitByte(byte2);
}

static void emitLoop(const int loopStart) {
    emitByte(OP_LOOP);
    const int offset = currentChunk()->count - loopStart + 2;
    if (offset > UINT16_MAX) error("Loop body too large");

    emitByte((offset >> 8) & 0xFF);
    emitByte(offset & 0xFF);
}

static int emitJump(const uint8_t instruction) {
    emitByte(instruction);
    emitByte(0xFF);
    emitByte(0xFF);
    return currentChunk()->count - 2;
}

static void emitReturn() {
    if (current->type == TYPE_INITIALIZER) {
        emitBytes(OP_GET_LOCAL, 0);
    } else {
        emitByte(OP_NIL);
    }
    emitByte(OP_RETURN);
}

static uint8_t makeConstant(const Value value) {
    const int constant = addConstant(currentChunk(), value);
    if (constant > UINT8_MAX) {
        error("Too many constants in chunk.");
        return 0;
    }

    return constant;
}

static void emitConstant(const Value value) {
    if(IS_NUMBER(value)) {
        long val = (long) round(AS_NUMBER(value));
        switch(val){
            case -1:
                return emitByte(OP_CONSTANT_MINUS_ONE);
            case 0:
                return emitByte(OP_CONSTANT_ZERO);
            case 1:
                return emitByte(OP_CONSTANT_ONE);
            case 2:
                return emitByte(OP_CONSTANT_TWO);
            default:
                emitBytes(OP_CONSTANT, makeConstant(value));
        }
    }else {
        emitBytes(OP_CONSTANT, makeConstant(value));
    }
}

static void emitFunction(const Compiler* compiler, ObjFunction* function) {
    const uint8_t constant = makeConstant(OBJ_VAL((Obj*) function));
    if (function->upvalueCount > 0) {
        emitBytes(OP_CLOSURE, constant);

        for (int i = 0; i < function->upvalueCount; i++) {
            emitByte(compiler->upvalues[i].isLocal ? 1 : 0);
            emitByte(compiler->upvalues[i].index);
        }
    } else {
        emitBytes(OP_CONSTANT, constant);
    }
}

static void patchJump(const int offset) {
    const int jump = currentChunk()->count - offset - 2;
    if (jump > UINT16_MAX) {
        error("Too much code to jump over");
    }

    currentChunk()->code[offset] = (jump >> 8) & 0xFF;
    currentChunk()->code[offset + 1] = jump & 0xFF;
}

static void patchAddress(const int offset) {
    currentChunk()->code[offset] = (currentChunk()->count >> 8) & 0xff;
    currentChunk()->code[offset + 1] = currentChunk()->count & 0xff;
}

static void initCompiler(Compiler* compiler, const FunctionType type) {
    compiler->enclosing = current;
    compiler->function = NULL;
    compiler->type = type;
    compiler->localCount = 0;
    compiler->scopeDepth = 0;
    compiler->function = newFunction();
    current = compiler;

    // Has to be called here because GC will try to mark its entries
    initTable(&compiler->stringConstants);

    if (type == TYPE_LAMBDA) {
        const char* template = "%s/[line %d] lambda";
        const int nameLength = snprintf(
            NULL, 0, template,
            compiler->enclosing->function->name != NULL
                ? compiler->enclosing->function->name->chars
                : "script",
            parser.previous.line);
        char* buffer = ALLOCATE(char, nameLength + 1);
        if(buffer == NULL) {
            error("Could not allocate memory for lambda name");
        }
        memset(buffer, 0, nameLength + 1);
        snprintf(
            buffer, nameLength + 1, template,
            compiler->enclosing->function->name != NULL
                ? compiler->enclosing->function->name->chars
                : "script",
            parser.previous.line);

        current->function->name = takeString(buffer, nameLength);
    } else if (type != TYPE_SCRIPT) {
        current->function->name = copyString(
            parser.previous.start,
            parser.previous.length);
    }

    Local* local = &current->locals[current->localCount++];
    local->depth = 0;
    local->isCaptured = false;
    if (type == TYPE_INITIALIZER || type == TYPE_METHOD) {
        local->name.start = "this";
        local->name.length = 4;
    } else {
        local->name.start = "";
        local->name.length = 0;
    }

    compiler->loopType = LOOP_NONE;
    compiler->innermostLoopStart = -1;
    compiler->innermostLoopScopeDepth = 0;

}

static ObjFunction* endCompiler() {
    emitReturn();
    ObjFunction* function = current->function;
#ifdef DEBUG_PRINT_CODE
    if (!parser.hadError) {
        disassembleChunk(stdout ,currentChunk(), function->name != NULL
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

static ParseRule* getRule(TokenType type);

static void parsePrecedence(Precedence precedence);

static void namedVariable(Token name, bool canAssign);

static uint8_t identifierConstant(const Token* name) {
    ObjString* string = copyString(name->start, name->length);
    Value indexValue;
    if (tableGet(&current->stringConstants, string, &indexValue)) {
        return (uint8_t) AS_NUMBER(indexValue);
    }
    const uint8_t index = makeConstant(OBJ_VAL((Obj*) string));
    tableSet(&current->stringConstants, string, NUMBER_VAL((double) index));
    return index;
}

static bool identifiersEqual(const Token* a, const Token* b) {
    if (a->length != b->length) return false;
    return memcmp(a->start, b->start, a->length) == 0;
}

static int resolveLocal(const Compiler* compiler, const Token* name) {
    for (int i = compiler->localCount - 1; i >= 0; i--) {
        const Local* local = &compiler->locals[i];
        if (identifiersEqual(name, &local->name)) {
            if (local->depth == -1) {
                error("Can't read local variable in it's own initializer");
            }
            return i;
        }
    }
    return -1;
}

static int addUpvalue(Compiler* compiler, const uint8_t index, const bool isLocal) {
    const int upvalueCount = compiler->function->upvalueCount;

    for (int i = 0; i < upvalueCount; i++) {
        const Upvalue* upvalue = &compiler->upvalues[i];
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

static int resolveUpvalue(Compiler* compiler, Token* name) {
    if (compiler->enclosing == NULL) return -1;

    const int local = resolveLocal(compiler->enclosing, name);

    if (local != -1) {
        compiler->enclosing->locals[local].isCaptured = true;
        return addUpvalue(compiler, local, true);
    }

    const int upvalue = resolveUpvalue(compiler->enclosing, name);
    if (upvalue != -1) {
        return addUpvalue(compiler, upvalue, false);
    }

    return -1;
}

static void addLocal(const Token name) {
    if (current->localCount == UINT8_COUNT) {
        error("Too many local variables in function.");
        return;
    }
    Local* local = &current->locals[current->localCount++];
    local->name = name;
    local->depth = -1;
    local->isCaptured = false;
}

static void declareVariable() {
    if (current->scopeDepth == 0) return;

    const Token* name = &parser.previous;
    for (int i = current->localCount - 1; i >= 0; i--) {
        const Local* local = &current->locals[i];
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

static uint8_t parseVariable(const char* errorMessage) {
    consume(TOKEN_IDENTIFIER, errorMessage);

    declareVariable();
    if (current->scopeDepth > 0) return 0;

    return identifierConstant(&parser.previous);
}

static void markInitialized() {
    if (current->scopeDepth == 0) return;
    current->locals[current->localCount - 1].depth = current->scopeDepth;
}

static void defineVariable(const uint8_t global) {
    if (current->scopeDepth > 0) {
        markInitialized();
        return;
    }

    emitBytes(OP_DEFINE_GLOBAL, global);
}

static void variable(const bool canAssign) {
    namedVariable(parser.previous, canAssign);
}

static Token syntheticToken(const char* text) {
    Token token;
    token.start = text;
    token.length = (int) strlen(text);
    token.line = -1;
    token.type = TOKEN_IDENTIFIER;
    return token;
}

static void super_(bool canAssign) {
    if (currentClass == NULL) {
        error("Can't use 'super' outside of a class.");
    } else if (!currentClass->hasSuperclass) {
        error("Can't use 'super' in class with no superclass.");
    }

    consume(TOKEN_DOT, "Expect '.' after 'super'.");
    consume(TOKEN_IDENTIFIER, "Expect superclass method name.");
    const uint8_t name = identifierConstant(&parser.previous);

    namedVariable(syntheticToken("this"), false);
    if (match(TOKEN_LEFT_PAREN)) {
        const uint8_t argCount = argumentList();
        namedVariable(syntheticToken("super"), false);
        emitBytes(OP_SUPER_INVOKE, name);
        emitByte(argCount);
    } else {
        namedVariable(syntheticToken("super"), false);
        emitBytes(OP_GET_SUPER, name);
    }
    emitBytes(OP_GET_SUPER, name);
}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "UnusedParameter"

static void and_(bool canAssign) {
#pragma clang diagnostic pop

    const int endJump = emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP);
    parsePrecedence(PREC_AND);
    patchJump(endJump);
}

static void parameterList() {
    /* IDEA
     *  Add rest argument to function declarations/definitions
     *  This creates an array with the rest of the passed arguments, that
     *  is accessible if the declaration contains "...name" as the last argument
     */
    do {
        current->function->arity++;
        if (current->function->arity > 255) {
            errorAtCurrent("Can't have more than 255 parameters");
        }
        const uint8_t constant = parseVariable("Expect parameter name.");
        defineVariable(constant);
    } while (match(TOKEN_COMMA));
}


static void lambda();

static void expression() {
    if (match(TOKEN_VERTICAL_LINE)) {
        lambda();
    } else if (check(TOKEN_LEFT_BRACKET)) {
        parsePrecedence(PREC_CONTAINER);
    } else {
        parsePrecedence(PREC_ASSIGNMENT);
    }
}

static void block() {
    while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
        declaration();
    }

    consume(TOKEN_RIGHT_BRACE, "Expect '}' after block.");
}

static void lambda() {
    Compiler compiler;
    initCompiler(&compiler, TYPE_LAMBDA);
    beginScope();

    if (!check(TOKEN_VERTICAL_LINE)) {
        parameterList();
    }
    consume(TOKEN_VERTICAL_LINE, "Expected '|' after lambda parameter list.");

    if (match(TOKEN_LEFT_BRACE)) {
        block();
    } else {
        expression();
        emitByte(OP_RETURN);
    }
    ObjFunction* function = endCompiler();
    emitFunction(&compiler, function);
}

static void function(const FunctionType type) {
    Compiler compiler;
    initCompiler(&compiler, type);
    beginScope();

    consume(TOKEN_LEFT_PAREN, "Expect '(' after function name.");
    if (!check(TOKEN_RIGHT_PAREN)) {
        parameterList();
    }
    consume(TOKEN_RIGHT_PAREN, "Expected ')' after parameters.");
    consume(TOKEN_LEFT_BRACE, "Expected '{' before function body.");

    block();

    ObjFunction* function = endCompiler(); // endScope() not needed because compiler ends here
    emitFunction(&compiler, function);
}

static void classMember() {
    const bool isStatic = match(TOKEN_STATIC);

    consume(TOKEN_IDENTIFIER, "Expect method name.");
    const uint8_t constant = identifierConstant(&parser.previous);

    FunctionType type = isStatic ? TYPE_STATIC_METHOD : TYPE_METHOD;
    if (parser.previous.length == 4 &&
        memcmp(parser.previous.start, "init", 4) == 0) {
        if (!isStatic) {
            type = TYPE_INITIALIZER;
        } else {
            error("Cannot mark 'init' method as static");
        }
    }
    const Value name = currentChunk()->constants.values[constant];
    if (check(TOKEN_LEFT_PAREN)) {
        if (isStatic && !tableSet(&currentClass->staticMembers, AS_STRING(name), NIL_VAL)) {
            error("Duplicate static member definition.");
        } else if (!tableSet(&currentClass->methods, AS_STRING(name), NIL_VAL)) {
            error("Duplicate method definition.");
        }
        function(type);
        emitBytes(isStatic ? OP_STATIC_METHOD : OP_METHOD, constant);
    } else if (isStatic) {
        if (!tableSet(&currentClass->staticMembers, AS_STRING(name), NIL_VAL)) {
            error("Duplicate static member definition.");
        }
        if (match(TOKEN_EQUAL)) {
            expression();
        } else {
            emitByte(OP_NIL);
        }
        consume(
            TOKEN_SEMICOLON,
            "Expect ';' after static field declaration");
        emitBytes(OP_STATIC_FIELD, constant);
    } else {
        error("Class fields must be declared as static.");
        expression();
        consume(TOKEN_SEMICOLON, "Expected ';' after expression.");
    }

}

static void classDeclaration() {
    consume(TOKEN_IDENTIFIER, "Expect class name");
    const Token className = parser.previous;
    const uint8_t nameConstant = identifierConstant(&parser.previous);
    declareVariable();
    emitBytes(OP_CLASS, nameConstant);
    defineVariable(nameConstant);

    ClassCompiler classCompiler;
    classCompiler.hasSuperclass = false;
    classCompiler.enclosing = currentClass;
    initTable(&classCompiler.methods);
    initTable(&classCompiler.staticMembers);
    currentClass = &classCompiler;

    if (match(TOKEN_LESS)) {
        consume(TOKEN_IDENTIFIER, "Expect superclass name.");
        variable(false);

        if (identifiersEqual(&className, &parser.previous)) {
            error("A class can't inherit from itself.");
        }

        beginScope();
        addLocal(syntheticToken("super"));
        defineVariable(0);

        namedVariable(className, false);
        emitByte(OP_INHERIT);
        classCompiler.hasSuperclass = true;
    }

    namedVariable(className, false);
    consume(TOKEN_LEFT_BRACE, "Expect '{' before class body.");
    while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
        classMember();
    }
    consume(TOKEN_RIGHT_BRACE, "Expect '}' after class body.");
    emitByte(OP_POP);

    if (classCompiler.hasSuperclass) {
        endScope();
    }

    freeTable(&classCompiler.methods);
    freeTable(&classCompiler.staticMembers);
    currentClass = classCompiler.enclosing;
}

static void funDeclaration() {
    const uint8_t global = parseVariable("Expect function name");
    markInitialized();
    function(TYPE_FUNCTION);
    defineVariable(global);
}

static void varDeclaration() {
    const uint8_t global = parseVariable("Expect variable name");

    if (match(TOKEN_EQUAL)) {
        expression();
    } else {
        emitByte(OP_NIL);
    }
    consumeReplOptional(
        TOKEN_SEMICOLON,
        "Expect ';' after variable declaration.");

    defineVariable(global);
}

static void expressionStatement() {
    expression();
    consumeReplOptional(TOKEN_SEMICOLON, "Expect ';' after expression.");
    if(isRepl()) {
        emitByte(OP_PRINT);
    }else {
        emitByte(OP_POP);
    }
}

#define MAX_BREAK_LOCATIONS 255

typedef struct BreakLocations {
    struct BreakLocations* prev;
    int count;
    int locations[MAX_BREAK_LOCATIONS];
} BreakLocations;

BreakLocations* currentBreakLocations = NULL;

void initBreakLocations(BreakLocations* locations) {
    locations->count = 0;
    locations->prev = currentBreakLocations;
    currentBreakLocations = locations;
}

void leaveBreakLocations(const BreakLocations* locations) {
    // Patch locations for break statement
    for (int i = 0; i < locations->count; i++) {
        patchJump(locations->locations[i]);
    }
    currentBreakLocations = locations->prev;
}

void addBreakLocation(const int location) {
    BreakLocations* loc = currentBreakLocations;
    if (loc == NULL || loc->count >= MAX_BREAK_LOCATIONS) return;
    loc->locations[loc->count] = location;
    loc->count++;
}

static void breakStatement() {
    if (current->loopType == LOOP_NONE && currentBreakLocations == NULL) {
        error("Can't use 'break' outside a loop/switch statements.");
    }

    if (currentBreakLocations != NULL &&
        currentBreakLocations->count == MAX_BREAK_LOCATIONS) {
        error("Too many break statements in the loop");
    }

    consume(TOKEN_SEMICOLON, "Expect ';' after 'break'.");

    for (int i = current->localCount - 1;
         i >= 0 && current->locals[i].depth > current->innermostLoopScopeDepth;
         i--) {
        if (current->locals[i].isCaptured) {
            emitByte(OP_CLOSE_UPVALUE);
        } else {
            emitByte(OP_POP);
        }
    }

    const int breakJump = emitJump(OP_JUMP);
    addBreakLocation(breakJump);
}

static void forStatement() {
    beginScope();

    // 1: Grab the name and slot of the loop variable,so that we can refer to it later.
    int loopVariable = -1;
    Token loopVariableName = {
        .start = NULL,
        .length = 0,
        .line = -1,
        .type = TOKEN_IDENTIFIER
    };
    // end

    consume(TOKEN_LEFT_PAREN, "Expect '(' after for.");

    if (match(TOKEN_SEMICOLON)) {
        // No initializer
    } else if (match(TOKEN_VAR)) {
        // 1: Grab the name of the loop variable
        loopVariableName = parser.current;
        varDeclaration();
        // and get its slot
        loopVariable = current->localCount - 1;
    } else {
        expressionStatement();
    }

    const LoopType surroundingLoopType = current->loopType;
    const int surroundingLoopStart = current->innermostLoopStart;
    const int surroundingLoopScopeDepth = current->innermostLoopScopeDepth;
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
        const int bodyJump = emitJump(OP_JUMP);

        const int incrementStart = currentChunk()->count;
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

    const int thenJump = emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP);
    statement();

    const int elseJump = emitJump(OP_JUMP);
    patchJump(thenJump);
    emitByte(OP_POP);
    if (match(TOKEN_ELSE)) statement();
    patchJump(elseJump);
}

static void printStatement() {
    expression();
    consumeReplOptional(TOKEN_SEMICOLON, "Expeced ';' after value.");
    emitByte(OP_PRINT);
}

static void returnStatement() {
    if (current->type == TYPE_SCRIPT) {
        error("Can't return from top-level code.");
    }

    if (match(TOKEN_SEMICOLON)) {
        emitReturn();
    } else {
        if (current->type == TYPE_INITIALIZER) {
            error("Can't return a value from an initializer.");
        }
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

    // Reserve a stack space for the switch helper position
    // That is used to save the switched expression result
    current->localCount++;

    BreakLocations locations;
    const LoopType surroundingLoopType = current->loopType;
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
    const LoopType surroundingLoopType = current->loopType;
    const int surroundingLoopStart = current->innermostLoopStart;
    const int surroundingLoopScopeDepth = current->innermostLoopScopeDepth;
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

static void tryCatchStatement() {
    emitByte(OP_PUSH_EXCEPTION_HANDLER);

    const int exceptionType = currentChunk()->count;
    emitByte(0xFF);

    const int handlerAddress = currentChunk()->count;
    emitBytes(0xFF, 0xFF);

    const int finallyAddress = currentChunk()->count;
    emitBytes(0xFF, 0xFF);

    statement();
    emitByte(OP_POP_EXCEPTION_HANDLER);

    bool tryOnly = true;

    const uint8_t successJump = emitJump(OP_JUMP);
    if (match(TOKEN_CATCH)) {
        tryOnly = false;

        beginScope();
        consume(TOKEN_LEFT_PAREN, "Expect '(' after catch.");
        consume(TOKEN_IDENTIFIER, "Expect type name to catch.");
        const uint8_t name = identifierConstant(&parser.previous);
        currentChunk()->code[exceptionType] = name;
        patchAddress(handlerAddress);
        if (match(TOKEN_AS)) {
            consume(TOKEN_IDENTIFIER, "Expect identifier for exception instance.");
            addLocal(parser.previous);
            markInitialized();
            const uint8_t ex_var = resolveLocal(current, &parser.previous);
            emitBytes(OP_SET_LOCAL, ex_var);
        }
        consume(TOKEN_RIGHT_PAREN, "Expect ')' after catch statement.");
        emitByte(OP_POP_EXCEPTION_HANDLER);
        statement();
        endScope();
    }
    patchJump(successJump);

    if (match(TOKEN_FINALLY)) {
        tryOnly = false;
        emitByte(OP_FALSE);

        patchAddress(finallyAddress);
        statement();

        const int continueExecution = emitJump(OP_JUMP_IF_FALSE);
        emitByte(OP_POP);
        emitByte(OP_PROPAGATE_EXCEPTION);
        patchJump(continueExecution);
        emitByte(OP_POP);
    }

    if (tryOnly) {
        error("Try must be followed by a catch and/or finally block.");
    }
}

void throwStatement() {
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after 'break'.");
    emitByte(OP_THROW);
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
        case TOKEN_BREAK:
        case TOKEN_CONTINUE:
        case TOKEN_SWITCH:
        case TOKEN_TRY:
        case TOKEN_THROW:
        case TOKEN_CATCH:
        case TOKEN_RETURN: return;
        default: ; // Do nothing
        }

        advance();
    }
}

static void declaration() {
    if (match(TOKEN_CLASS)) {
        classDeclaration();
    } else if (match(TOKEN_FUN)) {
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
    } else if (match(TOKEN_TRY)) {
        tryCatchStatement();
    } else if (match(TOKEN_THROW)) {
        throwStatement();
    } else if (match(TOKEN_LEFT_BRACE)) {
        beginScope();
        block();
        endScope();
    } else {
        expressionStatement();
    }
}

static void conditional(bool canAssign) {
    const uint8_t condition = emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP);
    parsePrecedence(PREC_ASSIGNMENT);
    const uint8_t was_true = emitJump(OP_JUMP);
    consume(TOKEN_COLON, "Expect ':' after then branch of conditional operator.");

    patchJump(condition);
    emitByte(OP_POP);
    parsePrecedence(PREC_CONDITIONAL);
    patchJump(was_true);
}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "UnusedParameter"

static void binary(bool canAssign) {
#pragma clang diagnostic pop
    const TokenType operatorType = parser.previous.type;
    const ParseRule* rule = getRule(operatorType);
    if (operatorType == TOKEN_STAR_STAR) {
        parsePrecedence((Precedence) rule->precedence);
    } else {
        parsePrecedence((Precedence) (rule->precedence + 1));
    }

    switch (operatorType) {
    case TOKEN_PLUS: emitByte(OP_ADD);
        break;
    case TOKEN_MINUS: emitByte(OP_SUBTRACT);
        break;
    case TOKEN_STAR: emitByte(OP_MULTIPLY);
        break;
    case TOKEN_STAR_STAR: emitByte(OP_EXPONENT);
        break;
    case TOKEN_SLASH: emitByte(OP_DIVIDE);
        break;
    case TOKEN_PERCENT: emitByte(OP_MODULUS);
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
    default: return; // Unreachable
    }
}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "UnusedParameter"

static void call(bool canAssign) {
#pragma clang diagnostic pop
    const uint8_t argCount = argumentList();
    emitBytes(OP_CALL, argCount);
}

static void dot(const bool canAssign) {
    consume(TOKEN_IDENTIFIER, "Expect property name after '.'");
    const uint8_t name = identifierConstant(&parser.previous);
    if (canAssign && match(TOKEN_EQUAL)) {
        expression();
        emitBytes(OP_SET_PROPERTY, name);
    } else if (canAssign && match(TOKEN_PLUS_EQUAL)) {
        emitByte(OP_DUP);
        emitBytes(OP_GET_PROPERTY, name);
        expression();
        emitByte(OP_ADD);
        emitBytes(OP_SET_PROPERTY, name);
    } else if (canAssign && match(TOKEN_MINUS_EQUAL)) {
        emitByte(OP_DUP);
        emitBytes(OP_GET_PROPERTY, name);
        expression();
        emitByte(OP_SUBTRACT);
        emitBytes(OP_SET_PROPERTY, name);
    } else if (canAssign && match(TOKEN_STAR_EQUAL)) {
        emitByte(OP_DUP);
        emitBytes(OP_GET_PROPERTY, name);
        expression();
        emitByte(OP_MULTIPLY);
        emitBytes(OP_SET_PROPERTY, name);
    } else if (canAssign && match(TOKEN_SLASH_EQUAL)) {
        emitByte(OP_DUP);
        emitBytes(OP_GET_PROPERTY, name);
        expression();
        emitByte(OP_DIVIDE);
        emitBytes(OP_SET_PROPERTY, name);
    } else if (canAssign && match(TOKEN_PERCENT_EQUAL)) {
        emitByte(OP_DUP);
        emitBytes(OP_GET_PROPERTY, name);
        expression();
        emitByte(OP_MODULUS);
        emitBytes(OP_SET_PROPERTY, name);
    } else if (match(TOKEN_LEFT_PAREN)) {
        const uint8_t argCount = argumentList();
        emitBytes(OP_INVOKE, name);
        emitByte(argCount);
    } else {
        emitBytes(OP_GET_PROPERTY, name);
    }
}

static void element(const bool canAssign) {
    expression();
    consume(TOKEN_RIGHT_BRACKET, "Expect ']' after array access.");
    if (canAssign && match(TOKEN_EQUAL)) {
        expression();
        emitByte(OP_SET_INDEX);
    } else if (canAssign && match(TOKEN_PLUS_EQUAL)) {
        emitBytes(OP_DUP, OP_GET_INDEX);
        expression();
        emitBytes(OP_ADD, OP_SET_INDEX);
    } else if (canAssign && match(TOKEN_MINUS_EQUAL)) {
        emitBytes(OP_DUP, OP_GET_INDEX);
        expression();
        emitBytes(OP_SUBTRACT, OP_SET_INDEX);
    } else if (canAssign && match(TOKEN_STAR_EQUAL)) {
        emitBytes(OP_DUP, OP_GET_INDEX);
        expression();
        emitBytes(OP_MULTIPLY, OP_SET_INDEX);
    } else if (canAssign && match(TOKEN_SLASH_EQUAL)) {
        emitBytes(OP_DUP, OP_GET_INDEX);
        expression();
        emitBytes(OP_DIVIDE, OP_SET_INDEX);
    } else if (canAssign && match(TOKEN_PERCENT_EQUAL)) {
        emitBytes(OP_DUP, OP_GET_INDEX);
        expression();
        emitBytes(OP_MODULUS, OP_SET_INDEX);
    } else {
        emitByte(OP_GET_INDEX);
    }
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
#pragma clang diagnostic pop

    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression");
}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "UnusedParameter"

static void number(bool canAssign) {
#pragma clang diagnostic pop

    const double value = strtod(parser.previous.start, NULL);
    emitConstant(NUMBER_VAL(value));
}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "UnusedParameter"

static void or_(bool canAssign) {
#pragma clang diagnostic pop

    const int elseJump = emitJump(OP_JUMP_IF_FALSE);
    const int endJump = emitJump(OP_JUMP);
    patchJump(elseJump);
    emitByte(OP_POP);

    parsePrecedence(PREC_OR);
    patchJump(endJump);
}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "UnusedParameter"

static void string(bool canAssign) {
#pragma clang diagnostic pop

    emitConstant(
        OBJ_VAL(
            (Obj*) copyString(parser.previous.start + 1,
                parser.previous.length - 2)));
}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "ConstantParameter"

static void namedVariable(Token name, const bool canAssign) {
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
    } else if (canAssign && match(TOKEN_PLUS_EQUAL)) {
        emitBytes(getOp, (uint8_t) arg);
        expression();
        emitByte(OP_ADD);
        emitBytes(setOp, (uint8_t) arg);
    } else if (canAssign && match(TOKEN_MINUS_EQUAL)) {
        emitBytes(getOp, (uint8_t) arg);
        expression();
        emitByte(OP_SUBTRACT);
        emitBytes(setOp, (uint8_t) arg);
    } else if (canAssign && match(TOKEN_STAR_EQUAL)) {
        emitBytes(getOp, (uint8_t) arg);
        expression();
        emitByte(OP_MULTIPLY);
        emitBytes(setOp, (uint8_t) arg);
    } else if (canAssign && match(TOKEN_SLASH_EQUAL)) {
        emitBytes(getOp, (uint8_t) arg);
        expression();
        emitByte(OP_DIVIDE);
        emitBytes(setOp, (uint8_t) arg);
    } else if (canAssign && match(TOKEN_PERCENT_EQUAL)) {
        emitBytes(getOp, (uint8_t) arg);
        expression();
        emitByte(OP_MODULUS);
        emitBytes(setOp, (uint8_t) arg);
    } else {
        emitBytes(getOp, (uint8_t) arg);
    }
}

#pragma clang diagnostic pop

static void this_(bool canAssign) {
    if (currentClass == NULL) {
        error("Can't use 'this' outside of a class.");
        return;
    }
    if (current->type == TYPE_STATIC_METHOD) {
        error("Can't use 'this' inside a static method.");
        return;
    }
    variable(false);
}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "UnusedParameter"

static void unary(bool canAssign) {
#pragma clang diagnostic pop

    const TokenType operatorType = parser.previous.type;

    // Compile the operand.
    parsePrecedence(PREC_UNARY);

    // Emit the operand instruction
    switch (operatorType) {
    case TOKEN_BANG: emitByte(OP_NOT);
        break;
    case TOKEN_MINUS: emitByte(OP_NEGATE);
        break;
    default: return; // Unreachable
    }
}

static void array(bool canAssign) {
    int size = 0;
    do {
        size++;
        if (size > UINT16_MAX) {
            error("Array literal can have no more than 65536 elements");
        }
        expression();
    } while (match(TOKEN_COMMA));
    consume(TOKEN_RIGHT_BRACKET, "Expected ']' after array element list.");
    emitByte(OP_ARRAY);
    emitByte((size >> 8) & 0xFF);
    emitByte(size & 0xFF);
}

ParseRule rules[] = {
    [TOKEN_LEFT_PAREN] = {grouping, call, PREC_CALL_INDEX},
    [TOKEN_RIGHT_PAREN] = {NULL, NULL, PREC_NONE},
    [TOKEN_LEFT_BRACE] = {NULL, NULL, PREC_NONE},
    [TOKEN_RIGHT_BRACE] = {NULL, NULL, PREC_NONE},
    [TOKEN_LEFT_BRACKET] = {array, element, PREC_CONTAINER},
    [TOKEN_RIGHT_BRACKET] = {NULL, NULL, PREC_NONE},
    [TOKEN_COLON] = {NULL, NULL, PREC_NONE},
    [TOKEN_QUESTION] = {NULL, conditional, PREC_CONDITIONAL},
    [TOKEN_VERTICAL_LINE] = {NULL, NULL, PREC_NONE},
    [TOKEN_COMMA] = {NULL, NULL, PREC_NONE},
    [TOKEN_DOT] = {NULL, dot, PREC_CALL_INDEX},
    [TOKEN_MINUS] = {unary, binary, PREC_TERM},
    [TOKEN_MINUS_EQUAL] = {NULL, NULL, PREC_NONE},
    [TOKEN_PERCENT] = {NULL, binary, PREC_FACTOR},
    [TOKEN_PERCENT_EQUAL] = {NULL, NULL, PREC_NONE},
    [TOKEN_PLUS] = {NULL, binary, PREC_TERM},
    [TOKEN_PLUS_EQUAL] = {NULL, NULL, PREC_NONE},
    [TOKEN_SEMICOLON] = {NULL, NULL, PREC_NONE},
    [TOKEN_SLASH] = {NULL, binary, PREC_FACTOR},
    [TOKEN_SLASH_EQUAL] = {NULL, NULL, PREC_NONE},
    [TOKEN_STAR] = {NULL, binary, PREC_FACTOR},
    [TOKEN_STAR_STAR] = {NULL, binary, PREC_EXPONENT},
    [TOKEN_STAR_EQUAL] = {NULL, NULL, PREC_NONE},
    [TOKEN_BANG] = {unary, NULL, PREC_NONE},
    [TOKEN_BANG_EQUAL] = {NULL, binary, PREC_EQUALITY},
    [TOKEN_EQUAL] = {NULL, NULL, PREC_NONE},
    [TOKEN_EQUAL_EQUAL] = {NULL, binary, PREC_EQUALITY},
    [TOKEN_GREATER] = {NULL, binary, PREC_COMPARISON},
    [TOKEN_GREATER_EQUAL] = {NULL, binary, PREC_COMPARISON},
    [TOKEN_LESS] = {NULL, binary, PREC_COMPARISON},
    [TOKEN_LESS_EQUAL] = {NULL, binary, PREC_COMPARISON},
    [TOKEN_IDENTIFIER] = {variable, NULL, PREC_NONE},
    [TOKEN_STRING] = {string, NULL, PREC_NONE},
    [TOKEN_NUMBER] = {number, NULL, PREC_NONE},
    [TOKEN_AND] = {NULL, and_, PREC_AND},
    [TOKEN_AS] = {NULL, NULL, PREC_NONE},
    [TOKEN_CLASS] = {NULL, NULL, PREC_NONE},
    [TOKEN_ELSE] = {NULL, NULL, PREC_NONE},
    [TOKEN_FALSE] = {literal, NULL, PREC_NONE},
    [TOKEN_FINALLY] = {NULL, NULL, PREC_NONE},
    [TOKEN_FOR] = {NULL, NULL, PREC_NONE},
    [TOKEN_FUN] = {NULL, NULL, PREC_NONE},
    [TOKEN_IF] = {NULL, NULL, PREC_NONE},
    [TOKEN_NIL] = {literal, NULL, PREC_NONE},
    [TOKEN_OR] = {NULL, or_, PREC_OR},
    [TOKEN_PRINT] = {NULL, NULL, PREC_NONE},
    [TOKEN_RETURN] = {NULL, NULL, PREC_NONE},
    [TOKEN_SUPER] = {super_, NULL, PREC_NONE},
    [TOKEN_THIS] = {this_, NULL, PREC_NONE},
    [TOKEN_THROW] = {NULL, NULL, PREC_NONE},
    [TOKEN_TRY] = {NULL, NULL, PREC_NONE},
    [TOKEN_TRUE] = {literal, NULL, PREC_NONE},
    [TOKEN_VAR] = {NULL, NULL, PREC_NONE},
    [TOKEN_WHILE] = {NULL, NULL, PREC_NONE},
    [TOKEN_ERROR] = {NULL, NULL, PREC_NONE},
    [TOKEN_EOF] = {NULL, NULL, PREC_NONE},
};


static void parsePrecedence(const Precedence precedence) {
    advance();
    const ParseFn prefixRule = getRule(parser.previous.type)->prefix;
    if (prefixRule == NULL) {
        error("Expect expression.");
        return;
    }

    const bool canAssign = precedence <= PREC_ASSIGNMENT;
    prefixRule(canAssign);

    while (precedence <= getRule(parser.current.type)->precedence) {
        advance();
        const ParseFn infixRule = getRule(parser.previous.type)->infix;
        infixRule(canAssign);
    }

    if (canAssign) {
        if (match(TOKEN_EQUAL) || match(TOKEN_PLUS_EQUAL)
            || match(TOKEN_MINUS_EQUAL) || match(TOKEN_STAR_EQUAL)
            || match(TOKEN_SLASH_EQUAL) || match(TOKEN_PERCENT_EQUAL)) {
            error("Invalid assignment target");
        }
    }
}

static ParseRule* getRule(const TokenType type) {
    return &rules[type];
}

ObjFunction* compile(const char* source) {
    initScanner(source);
    Compiler compiler;
    initCompiler(&compiler, TYPE_SCRIPT);

    parser.hadError = false;
    parser.panicMode = false;

    advance();

    while (!match(TOKEN_EOF)) {
        declaration();
    }

    ObjFunction* function = endCompiler();
    return parser.hadError ? NULL : function;
}

void markCompilerRoots() {
    Compiler* compiler = current;
    while (compiler != NULL) {
        markObject((Obj*) compiler->function);

        markTable(&compiler->stringConstants);
        compiler = compiler->enclosing;
    }
}
