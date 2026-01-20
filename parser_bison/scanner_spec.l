%top{

#include <stdio.h>
#include "parser_spec.h"

#define YY_DECL int yylex (Token* lvalp, TokenLocation* loc, yyscan_t yyscanner)

#define SET_STRING_ERROR(str) do{ if (stringError == NULL) stringError = str; }while(0);

#define TOKEN(NAME) do { *lvalp = (Token) {yytext, yyleng}; *loc = (TokenLocation){yylineno, yycolumn}; return TOKEN_##NAME; }while(0);

}

%{
static int first_string_eof = 1;
static int prevLength = 0;
static char* stringError = NULL;
%}

%option noyywrap reentrant 
%option warn nodefault batch
%option yylineno
%option nounistd never-interactive

whitespace      [ \t\r]
cloxalpha       [[:alnum:]]|_
ident           ([[:alpha:]]){cloxalpha}*
number          [[:digit:]]+(\.[[:digit:]]+)?

%x string
%x comment

%{
    YY_DECL;
%}

%%
    char buffer[1024];
    char* buffer_ptr = buffer;

    int comment_caller = 0;
    
%{
    yycolumn += yyleng;
%}

{whitespace}+   yycolumn += yyleng;
\/\/.+$?         yycolumn = 1;

"/*"                {
                        comment_caller = YY_START;
                        BEGIN(comment);
                    }
<comment>{
    [^*\n]*         /* eat anything that's not a '*' */
    "*"+[^*/\n]*    /* eat up '*'s not followed by '/'s */
    \n              yycolumn = 1;
    "*"+"/"         BEGIN(comment_caller);
    <<EOF>>         {
                        BEGIN(INITIAL);
                        return TOKEN_ERROR;
                    }
}


\"                  {
                        buffer_ptr = buffer;
                        BEGIN(string);
                        yymore();
                        prevLength = yyleng;
                    }           
<string>{
    \"              {
                        BEGIN(INITIAL);
                        *buffer_ptr = 0;
                        if (stringError != NULL) {
                            char* tmp = stringError;
                            stringError = NULL;
                            return TOKEN_ERROR;
                        }
                        TOKEN(STRING);
                    }
    
    \n              {
                        yycolumn = 1;
                        prevLength = yyleng;
                        SET_STRING_ERROR("unterminated string literal");
                        yymore();
                    }
    <<EOF>>         {
                        if (first_string_eof){
                            first_string_eof--;
                            return TOKEN_ERROR;
                        }
                        prevLength = yyleng;
                        return TOKEN_EOF;
                    }

    \\x[[:xdigit:]]+ {
                            int result = strtoul(yytext+prevLength+2, NULL, 16);
                            if ( result > 0xFF ) {
                                SET_STRING_ERROR("hex escape sequence out of range");
                                yymore();
                            }else {
                                *buffer_ptr++ = result;
                                yymore();
                                prevLength = yyleng;
                            }
                        }

    \\x                 {
                            prevLength = yyleng;
                            SET_STRING_ERROR("\\x used with no following hex digits");
                            yymore();
                        }

    \\[0-7]{1,3}        {   
                            int result = strtoul(yytext+prevLength+1, NULL, 8);
                            if ( result > 0xFF ) {
                                SET_STRING_ERROR("octal escape sequence out of range");
                                yymore();
                            } else {
                                *buffer_ptr++ = result;
                                yymore();
                                prevLength = yyleng;   
                            }
                        }
        
    \\a             prevLength = yyleng; yymore(); *buffer_ptr++ = '\a';
    \\b             prevLength = yyleng; yymore(); *buffer_ptr++ = '\b';
    \\f             prevLength = yyleng; yymore(); *buffer_ptr++ = '\f';
    \\r             prevLength = yyleng; yymore(); *buffer_ptr++ = '\r';
    \\n             prevLength = yyleng; yymore(); *buffer_ptr++ = '\n';
    \\t             prevLength = yyleng; yymore(); *buffer_ptr++ = '\t';
    \\v             prevLength = yyleng; yymore(); *buffer_ptr++ = '\v';
    \\\\            prevLength = yyleng; yymore(); *buffer_ptr++ = '\\';
    \\\'            prevLength = yyleng; yymore(); *buffer_ptr++ = '\'';
    \\\"            prevLength = yyleng; yymore(); *buffer_ptr++ = '\"';

    \\(.|\n)        {
                        prevLength = yyleng;
                        yymore();
                        SET_STRING_ERROR("unknow escape sequence");
                    }

    [^\\\n\"]+      {
                        char *yptr = yytext;
                        while (*yptr) *buffer_ptr++ = *yptr++;
                        yymore();
                        prevLength = yyleng;
                    }

    /* must be here to suppress flex -s flag */
    .               {
                        prevLength = yyleng;
                        yymore();
                        SET_STRING_ERROR("CRITICAL: character code");
                    }
}

-=              TOKEN(MINUS_EQUAL);
%=              TOKEN(PERCENT_EQUAL);
\+=             TOKEN(PLUS_EQUAL);
\/=             TOKEN(SLASH_EQUAL);
\*=             TOKEN(STAR_EQUAL);
\*\*            TOKEN(STAR_STAR);
!=              TOKEN(BANG_EQUAL);
==              TOKEN(EQUAL_EQUAL);
\>=             TOKEN(GREATER_EQUAL);
\<=             TOKEN(LESS_EQUAL);

\(              TOKEN(LEFT_PAREN);
\)              TOKEN(RIGHT_PAREN);
\[              TOKEN(LEFT_BRACKET);
\]              TOKEN(RIGHT_BRACKET);
\{              TOKEN(LEFT_BRACE);
\}              TOKEN(RIGHT_BRACE);

,               TOKEN(COMMA);
:               TOKEN(COLON);
\.              TOKEN(DOT);
\|              TOKEN(VERTICAL_LINE);
-               TOKEN(MINUS);
%               TOKEN(PERCENT);
\+              TOKEN(PLUS);
\;              TOKEN(SEMICOLON);
\/              TOKEN(SLASH);
\*              TOKEN(STAR);
\?              TOKEN(QUESTION);
\!              TOKEN(BANG);
=               TOKEN(EQUAL);
\>              TOKEN(GREATER);
\<              TOKEN(LESS);

and             TOKEN(AND);
as              TOKEN(AS);
break           TOKEN(BREAK);
case            TOKEN(CASE);
catch           TOKEN(CATCH);
class           TOKEN(CLASS);
continue        TOKEN(CONTINUE);
default         TOKEN(DEFAULT);
else            TOKEN(ELSE);
false           TOKEN(FALSE);
for             TOKEN(FOR);
fun             TOKEN(FUN);
finally         TOKEN(FINALLY);
if              TOKEN(IF);
nil             TOKEN(NIL);
or              TOKEN(OR);
print           TOKEN(PRINT);
return          TOKEN(RETURN);
static          TOKEN(STATIC);
super           TOKEN(SUPER);
switch          TOKEN(SWITCH);
this            TOKEN(THIS);
throw           TOKEN(THROW);
true            TOKEN(TRUE);
try             TOKEN(TRY);
var             TOKEN(VAR);
while           TOKEN(WHILE);

{ident}         TOKEN(IDENTIFIER);
{number}        TOKEN(NUMBER);

\n              yycolumn = 1;

<<EOF>>         return TOKEN_EOF;
.               TOKEN(ERROR);

%%
