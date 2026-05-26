# pyright: reportUndefinedVariable=false, reportInvalidTypeForm=false
from metaox import term

term.typename = "Token"

# Single character tokens
LEFT_PAREN: term
RIGHT_PAREN: term
LEFT_BRACKET: term
RIGHT_BRACKET: term
LEFT_BRACE: term
RIGHT_BRACE: term
COMMA: term
COLON: term
DOT: term
VERTICAL_LINE: term
MINUS: term
PERCENT: term
PLUS: term
SEMICOLON: term
SLASH: term
STAR: term
QUESTION: term
BANG: term
EQUAL: term
GREATER: term
LESS: term

# Two character tokens
MINUS_EQUAL: term
PERCENT_EQUAL: term
PLUS_EQUAL: term
SLASH_EQUAL: term
STAR_EQUAL: term
STAR_STAR: term
BANG_EQUAL: term
EQUAL_EQUAL: term
GREATER_EQUAL: term
LESS_EQUAL: term

# Value literal tokens
IDENTIFIER: term
STRING: term
NUMBER: term

# Keyword tokens
AND: term
AS: term
BREAK: term
CASE: term
CATCH: term
CLASS: term
CONTINUE: term
DEFAULT: term
ELSE: term
FALSE: term
FOR: term
FUN: term
FINALLY: term
IF: term
NIL: term
OR: term
PRINT: term
RETURN: term
STATIC: term
SUPER: term
SWITCH: term
THIS: term
THROW: term
TRUE: term
TRY: term
VAR: term
WHILE: term

Statements: list[Statement] = Statement | (Statements, Statement)

Statement = DeclarationStatement | ExpressionStatement

DeclarationStatement = VarDeclarationStatement

VarDeclarationStatement = VarDeclaration, SEMICOLON

VarDeclaration = VAR, VarDeclarationList

VarDeclarationList: list[VarDeclElement] = VarDeclElement | (
    VarDeclarationList,
    COMMA,
    VarDeclElement,
)

VarDeclElement = IDENTIFIER, EQUAL, Expression

ExpressionStatement = Expression, SEMICOLON

Expression = AssignmentExpression

AssignmentExpression = ConditionalExpression | (
    Designator,
    AssignmentOperator,
    Expression,
)

Designator = (
    IDENTIFIER
    | (Designator, LEFT_BRACKET, Expression, RIGHT_BRACKET)
    | (Designator, DOT, IDENTIFIER)
)

ArgumentList: list[Expression] = Expression | (ArgumentList, COMMA, Expression)

AssignmentOperator = (
    EQUAL | PLUS_EQUAL | MINUS_EQUAL | STAR_EQUAL | SLASH_EQUAL | PERCENT_EQUAL
)

ConditionalExpression = DisjunctionExpression | (
    DisjunctionExpression,
    QUESTION,
    Expression,
    COLON,
    DisjunctionExpression,
)

DisjunctionExpression = ConjunctionExpression | (
    ConjunctionExpression,
    OR,
    ConjunctionExpression,
)

ConjunctionExpression = EqualityExpression | (
    EqualityExpression,
    AND,
    EqualityExpression,
)

EqualityExpression = OrderingExpression | (
    OrderingExpression,
    EqualityOperator,
    OrderingExpression,
)

EqualityOperator = EQUAL_EQUAL | BANG_EQUAL

OrderingExpression = AdditionExpression | (
    AdditionExpression,
    OrderingOperator,
    AdditionExpression,
)

OrderingOperator = LESS | LESS_EQUAL | GREATER | GREATER_EQUAL

AdditionExpression = TermExpression | (
    AdditionExpression,
    AdditionOperator,
    TermExpression,
)

AdditionOperator = PLUS | MINUS

TermExpression = FactorExpression | (
    TermExpression,
    MultiplicationOperator,
    FactorExpression,
)

MultiplicationOperator = STAR | SLASH | PERCENT

FactorExpression = ExponentExpression | (UnaryOperator, ExponentExpression)

ExponentExpression = PrimaryExpression | (
    PrimaryExpression,
    STAR_STAR,
    FactorExpression,
)

UnaryOperator = BANG | MINUS

PrimaryExpression = (
    NIL
    | NUMBER
    | STRING
    | Designator
    | (LEFT_PAREN, ConditionalExpression, RIGHT_PAREN)
    | FunctionCall
)

FunctionCall = Designator, LEFT_PAREN, ArgumentList, RIGHT_PAREN
