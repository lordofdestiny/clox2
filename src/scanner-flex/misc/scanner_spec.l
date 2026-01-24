%top{

#include <stdio.h>
#include "token.h"

#define YY_DECL Token yylex(yyscan_t yyscanner)

#define SET_STRING_ERROR(str) do{ if (stringError == NULL) stringError = str; }while(0);

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
    Token makeToken(yyscan_t yyscanner, const TokenType type);
    Token errorToken(yyscan_t yyscanner, const char* message);
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
                        return errorToken(yyscanner, "unterminated block comment");
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
                            return errorToken(yyscanner, tmp);
                        }
                        // TODO move on from yymore to temporary buffer
                        return makeToken(yyscanner, TOKEN_STRING);
                    }
    
    \n              {
                        yycolumn = 1;
                        prevLength = yyleng;
                        SET_STRING_ERROR("unterminated string literal");
                        yymore();
                        //return errorToken(yyscanner, "unterminated string literal");
                    }
    <<EOF>>         {
                        if (first_string_eof){
                            first_string_eof--;
                            return errorToken(yyscanner, "unterminated string literal at end of file");
                        }
                        prevLength = yyleng;
                        return makeToken(yyscanner, TOKEN_EOF);
                    }

    \\x[[:xdigit:]]+ {
                            int result = strtoul(yytext+prevLength+2, NULL, 16);
                            if ( result > 0xFF ) {
                                SET_STRING_ERROR("hex escape sequence out of range");
                                yymore();
                                // return errorToken(yyscanner, "hex escape sequence out of range");
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
                            // return errorToken(yyscanner, "\\x used with no following hex digits");
                        }

    \\[0-7]{1,3}        {   
                            int result = strtoul(yytext+prevLength+1, NULL, 8);
                            if ( result > 0xFF ) {
                                SET_STRING_ERROR("octal escape sequence out of range");
                                yymore();
                                // return errorToken(yyscanner, "octal escape sequence out of range");
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
                        // return errorToken(yyscanner, "unknown escape sequence");    
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
                        // return errorToken(yyscanner, "CRITICAL: character code");
                    }
}

-=              return makeToken(yyscanner, TOKEN_MINUS_EQUAL);
%=              return makeToken(yyscanner, TOKEN_PERCENT_EQUAL);
\+=             return makeToken(yyscanner, TOKEN_PLUS_EQUAL);
\/=             return makeToken(yyscanner, TOKEN_SLASH_EQUAL);
\*=             return makeToken(yyscanner, TOKEN_STAR_EQUAL);
\*\*            return makeToken(yyscanner, TOKEN_STAR_STAR);
!=              return makeToken(yyscanner, TOKEN_BANG_EQUAL);
==              return makeToken(yyscanner, TOKEN_EQUAL_EQUAL);
\>=             return makeToken(yyscanner, TOKEN_GREATER_EQUAL);
\<=             return makeToken(yyscanner, TOKEN_LESS_EQUAL);

\(              return makeToken(yyscanner, TOKEN_LEFT_PAREN);
\)              return makeToken(yyscanner, TOKEN_RIGHT_PAREN);
\[              return makeToken(yyscanner, TOKEN_LEFT_BRACKET);
\]              return makeToken(yyscanner, TOKEN_RIGHT_BRACKET);
\{              return makeToken(yyscanner, TOKEN_LEFT_BRACE);
\}              return makeToken(yyscanner, TOKEN_RIGHT_BRACE);

,               return makeToken(yyscanner, TOKEN_COMMA);
:               return makeToken(yyscanner, TOKEN_COLON);
\.              return makeToken(yyscanner, TOKEN_DOT);
\|              return makeToken(yyscanner, TOKEN_VERTICAL_LINE);
-               return makeToken(yyscanner, TOKEN_MINUS);
%               return makeToken(yyscanner, TOKEN_PERCENT);
\+              return makeToken(yyscanner, TOKEN_PLUS);
\;              return makeToken(yyscanner, TOKEN_SEMICOLON);
\/              return makeToken(yyscanner, TOKEN_SLASH);
\*              return makeToken(yyscanner, TOKEN_STAR);
\?              return makeToken(yyscanner, TOKEN_QUESTION);
\!              return makeToken(yyscanner, TOKEN_BANG);
=               return makeToken(yyscanner, TOKEN_EQUAL);
\>              return makeToken(yyscanner, TOKEN_GREATER);
\<              return makeToken(yyscanner, TOKEN_LESS);

and             return makeToken(yyscanner, TOKEN_AND);
as              return makeToken(yyscanner, TOKEN_AS);
break           return makeToken(yyscanner, TOKEN_BREAK);
case            return makeToken(yyscanner, TOKEN_CASE);
catch           return makeToken(yyscanner, TOKEN_CATCH);
class           return makeToken(yyscanner, TOKEN_CLASS);
continue        return makeToken(yyscanner, TOKEN_CONTINUE);
default         return makeToken(yyscanner, TOKEN_DEFAULT);
else            return makeToken(yyscanner, TOKEN_ELSE);
false           return makeToken(yyscanner, TOKEN_FALSE);
for             return makeToken(yyscanner, TOKEN_FOR);
fun             return makeToken(yyscanner, TOKEN_FUN);
finally         return makeToken(yyscanner, TOKEN_FINALLY);
if              return makeToken(yyscanner, TOKEN_IF);
nil             return makeToken(yyscanner, TOKEN_NIL);
or              return makeToken(yyscanner, TOKEN_OR);
print           return makeToken(yyscanner, TOKEN_PRINT);
return          return makeToken(yyscanner, TOKEN_RETURN);
static          return makeToken(yyscanner, TOKEN_STATIC);
super           return makeToken(yyscanner, TOKEN_SUPER);
switch          return makeToken(yyscanner, TOKEN_SWITCH);
this            return makeToken(yyscanner, TOKEN_THIS);
throw           return makeToken(yyscanner, TOKEN_THROW);
true            return makeToken(yyscanner, TOKEN_TRUE);
try             return makeToken(yyscanner, TOKEN_TRY);
var             return makeToken(yyscanner, TOKEN_VAR);
while           return makeToken(yyscanner, TOKEN_WHILE);

{ident}         return makeToken(yyscanner, TOKEN_IDENTIFIER);
{number}        return makeToken(yyscanner, TOKEN_NUMBER);

\n              yycolumn = 1;

<<EOF>>         return makeToken(yyscanner, TOKEN_EOF);
.               return errorToken(yyscanner, "unrecognized symbol");

%%
