#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <scanner/scanner.h>

struct Scanner {
    const char* start;
    const char* current;
    int line;
    int column;
};

int initScanner(Scanner** scanner_ptr, InputFile source) {
    Scanner* scanner = malloc(sizeof(Scanner));
    if(scanner == NULL) {
        return 1;
    }

    scanner->start = source.content;
    scanner->current = source.content;
    scanner->line = 1;
    scanner->column = 1;

    *scanner_ptr = scanner;

    return 0;
}

void freeScanner(Scanner* scanner) {
    free(scanner);
}

static bool isAlpha(const char c) {
    return (c >= 'a' && c <= 'z') ||
           (c >= 'A' && c <= 'Z') ||
           c == '_';
}

static bool isDigit(const char c) {
    return isdigit(c);
}

static bool isOctDigit(const char c) {
    return isDigit(c) && c <= '7';
}

static bool isHexDigit(const char c) {
    return isxdigit(c);
}

static char peek(Scanner* scanner) {
    return *scanner->current;
}

static TokenLocation location(Scanner* scanner) {
    return (TokenLocation) {
        .line = scanner->line,
        .column = scanner->column
    };
}

static Token makeToken(Scanner* scanner, const TokenType type) {
    return (Token) {
        .type = type,
        .start = scanner->start,
        .length = scanner->current - scanner->start,
        .loc = location(scanner)
    };
}

static Token errorToken(Scanner* scanner, const char* message) {
    return (Token) {
        .type = TOKEN_ERROR,
        .start = message,
        .length = strlen(message),
        .loc = location(scanner)
    };
}

static bool isAtEnd(Scanner* scanner) {
    return *scanner->current == '\0';
}

static char advance(Scanner* scanner) {
    scanner->current++;
    return scanner->current[-1];
}

static char peekNext(Scanner* scanner) {
    if (isAtEnd(scanner)) return '\0';
    return scanner->current[1];
}

static char match(Scanner* scanner, const char expected) {
    if (isAtEnd(scanner)) return false;
    if (*scanner->current != expected) return false;
    scanner->current++;
    return true;
}

static void skipWhitespace(Scanner* scanner) {
    while (true) {
        const char c = peek(scanner);
        switch (c) {
        case '\n':
            scanner->line++;
            scanner->column = 0;
            [[fallthrough]];
        case ' ':
        case '\r':
        case '\t': {
            advance(scanner);
            scanner->column++;
            break;
        }
        case '/':
            if (peekNext(scanner) == '/') {
                while (peek(scanner) != '\n' && !isAtEnd(scanner)) advance(scanner);
                break;
            }
        default: return;
        }
    }
}

static TokenType checkKeyword(
	Scanner* scanner, int start, int length, const char* rest, TokenType type
) {
	if (scanner->current - scanner->start == (ptrdiff_t) start + length &&
		memcmp(scanner->start + start, rest, length) == 0) {
		return type;
	}
	return TOKEN_IDENTIFIER;
}

static TokenType identifierType(Scanner* scanner) {
	switch (scanner->start[0]) {
	case 'a':
		if (scanner->current - scanner->start > 1) {
			switch (scanner->start[1]) {
			case 'n': return checkKeyword(scanner, 2, 1, "d", TOKEN_AND);
			case 's': return checkKeyword(scanner, 2, 0, "", TOKEN_AS);
			default: ;
			}
		}
		break;
	case 'b': return checkKeyword(scanner, 1, 4, "reak", TOKEN_BREAK);
	case 'c':
		if (scanner->current - scanner->start > 1) {
			switch (scanner->start[1]) {
			case 'a':
				if (scanner->current - scanner->start > 2) {
					switch (scanner->start[2]) {
					case 's': return checkKeyword(scanner, 3, 1, "e", TOKEN_CASE);
					case 't': return checkKeyword(scanner, 3, 2, "ch", TOKEN_CATCH);
					default: ;
					}
				}
				break;
			case 'l': return checkKeyword(scanner, 2, 3, "ass", TOKEN_CLASS);
			case 'o': return checkKeyword(scanner, 2, 6, "ntinue", TOKEN_CONTINUE);
			default: ;
			}
		}
		break;
	case 'd': return checkKeyword(scanner, 1, 6, "efault", TOKEN_DEFAULT);
	case 'e': return checkKeyword(scanner, 1, 3, "lse", TOKEN_ELSE);
	case 'f':
		if (scanner->current - scanner->start > 1) {
			switch (scanner->start[1]) {
			case 'a': return checkKeyword(scanner, 2, 3, "lse", TOKEN_FALSE);
			case 'o': return checkKeyword(scanner, 2, 1, "r", TOKEN_FOR);
			case 'u': return checkKeyword(scanner, 2, 1, "n", TOKEN_FUN);
			case 'i': return checkKeyword(scanner, 2, 5, "nally", TOKEN_FINALLY);
			default: ;
			}
		}
		break;
	case 'i': return checkKeyword(scanner, 1, 1, "f", TOKEN_IF);
	case 'n': return checkKeyword(scanner, 1, 2, "il", TOKEN_NIL);
	case 'o': return checkKeyword(scanner, 1, 1, "r", TOKEN_OR);
	case 'p': return checkKeyword(scanner, 1, 4, "rint", TOKEN_PRINT);
	case 'r': return checkKeyword(scanner, 1, 5, "eturn", TOKEN_RETURN);
	case 's':
		if (scanner->current - scanner->start > 1) {
			switch (scanner->start[1]) {
			case 't': return checkKeyword(scanner, 2, 4, "atic", TOKEN_STATIC);
			case 'u': return checkKeyword(scanner, 2, 3, "per", TOKEN_SUPER);
			case 'w': return checkKeyword(scanner, 2, 4, "itch", TOKEN_SWITCH);
			default: ;
			}
		}
		break;
	case 't':
		if (scanner->current - scanner->start > 1) {
			switch (scanner->start[1]) {
			case 'h':
				if (scanner->current - scanner->start > 2) {
					switch (scanner->start[2]) {
					case 'i': return checkKeyword(scanner, 3, 1, "s", TOKEN_THIS);
					case 'r': return checkKeyword(scanner, 3, 2, "ow", TOKEN_THROW);
					default: ;
					}
				}
				break;
			case 'r':
				if (scanner->current - scanner->start > 2) {
					switch (scanner->start[2]) {
					case 'u': return checkKeyword(scanner, 3, 1, "e", TOKEN_TRUE);
					case 'y': return checkKeyword(scanner, 3, 0, "", TOKEN_TRY);
					default: ;
					}
				}
				break;
			default: ;
			}
		}
		break;
	case 'v': return checkKeyword(scanner, 1, 2, "ar", TOKEN_VAR);
	case 'w': return checkKeyword(scanner, 1, 4, "hile", TOKEN_WHILE);
	default: ;
	}
	return TOKEN_IDENTIFIER;
}

static Token identifier(Scanner* scanner) {
    while (isAlpha(peek(scanner)) || isDigit(peek(scanner))) advance(scanner);
    return makeToken(scanner, identifierType(scanner));
}

static Token number(Scanner* scanner) {
    while (isDigit(peek(scanner))) advance(scanner);

    if (peek(scanner) == '.' && isDigit(peekNext(scanner))) {
        advance(scanner);

        while (isDigit(peek(scanner))) advance(scanner);
    }

    return makeToken(scanner, TOKEN_NUMBER);
}

static bool isEscapable(char c) {
    static const char* chars = "abfrntv\\\'\"";
    return strchr(chars, c);
}

#define STRING_ERROR(str) do{errorMsg = str; goto skip; } while(0);

static Token string(Scanner* scanner) {
    const char* errorMsg = NULL;
    while (peek(scanner) != '"' && !isAtEnd(scanner)) {
        if (peek(scanner) == '\n') {
            scanner->line++;
            scanner->column = 1;
            STRING_ERROR( "unterminated string literal");
        }
        if (peek(scanner) != '\\') {
            advance(scanner);
            continue;
        }

        // Escape sequences
        // consume backslash
        advance(scanner);
        if (isAtEnd(scanner)) {
            STRING_ERROR( "unterminated string literal");
        }
        if (isEscapable(peek(scanner))) {
            advance(scanner);
            continue;
        }
        if (peek(scanner) == 'x') {
            advance(scanner);
            int i = 0;
            long total = 0;
            bool outOfRange = false;
            while(isHexDigit(peek(scanner)) && !isAtEnd(scanner)) {
                char c = advance(scanner);
                total *= 16;
                if (isDigit(c)) total += c - '0';
                if (islower(c)) total += c - 'a';
                if (isupper(c)) total += c - 'A';
                if (total > 0xff) outOfRange = true;
                i++;
            }
            if (i == 0) STRING_ERROR("\\x used with no following hex digits");
            if (outOfRange) STRING_ERROR("hex escape sequence out of range");
            continue;
        }
        if (isOctDigit(peek(scanner))) {
            int i = 0;
            int total = 0;
            bool outOfRange = false;
            while(i < 3 && isOctDigit(peek(scanner)) && !isAtEnd(scanner)) {
                char c = advance(scanner);
                total = 8 * total + (c - '0');
                if (total > 0xff) outOfRange = true;
                i++;
            }
            if (outOfRange) STRING_ERROR("octal escape sequence out of range");
            continue;
        }
        STRING_ERROR("unknown escape sequence");
    }

    if (isAtEnd(scanner)) {
        return errorToken(scanner, "unterminated string literal");
    }

skip:
    while (peek(scanner) != '"' && !isAtEnd(scanner)) advance(scanner);
    // The closing quote
    advance(scanner);    
    if (errorMsg != NULL) {
        return errorToken(scanner, errorMsg);
    }
    return makeToken(scanner, TOKEN_STRING);
}

static Token charToken(Scanner* scanner, char c) {
	switch (c) {
	case '(': return makeToken(scanner, TOKEN_LEFT_PAREN);
	case ')': return makeToken(scanner, TOKEN_RIGHT_PAREN);
	case '[': return makeToken(scanner, TOKEN_LEFT_BRACKET);
	case ']': return makeToken(scanner, TOKEN_RIGHT_BRACKET);
	case '{': return makeToken(scanner, TOKEN_LEFT_BRACE);
	case '}': return makeToken(scanner, TOKEN_RIGHT_BRACE);
	case ',': return makeToken(scanner, TOKEN_COMMA);
	case ':': return makeToken(scanner, TOKEN_COLON);
	case '.': return makeToken(scanner, TOKEN_DOT);
	case '|': return makeToken(scanner, TOKEN_VERTICAL_LINE);
	case '-': {
		if (match(scanner, '=')) return makeToken(scanner, TOKEN_MINUS_EQUAL);
		return makeToken(scanner, TOKEN_MINUS);
	}
	case '%': {
		if (match(scanner, '=')) return makeToken(scanner, TOKEN_PERCENT_EQUAL);
		return makeToken(scanner, TOKEN_PERCENT);
	}
	case '+': {
		if (match(scanner, '=')) return makeToken(scanner, TOKEN_PLUS_EQUAL);
		return makeToken(scanner, TOKEN_PLUS);
	}
	case ';': return makeToken(scanner, TOKEN_SEMICOLON);
	case '/': {
		if (match(scanner, '=')) return makeToken(scanner, TOKEN_SLASH_EQUAL);
		return makeToken(scanner, TOKEN_SLASH);
	}
	case '*': {
		if (match(scanner, '=')) return makeToken(scanner, TOKEN_STAR_EQUAL);
		if (match(scanner, '*')) return makeToken(scanner, TOKEN_STAR_STAR);
		return makeToken(scanner, TOKEN_STAR);
	}
	case '?': return makeToken(scanner, TOKEN_QUESTION);
	case '!': {
		if (match(scanner, '=')) return makeToken(scanner, TOKEN_BANG_EQUAL);
		return makeToken(scanner, TOKEN_BANG);
	}
	case '=': {
		if (match(scanner, '=')) return makeToken(scanner, TOKEN_EQUAL_EQUAL);
		return makeToken(scanner, TOKEN_EQUAL);
	}
	case '>': {
		if (match(scanner, '=')) return makeToken(scanner, TOKEN_GREATER_EQUAL);
		return makeToken(scanner, TOKEN_GREATER);
	}
	case '<': {
		if (match(scanner, '=')) return makeToken(scanner, TOKEN_LESS_EQUAL);
		return makeToken(scanner, TOKEN_LESS);
	}
	default: break;
	}

	return errorToken(scanner, "Unexpected character.");
}

Token scanToken(Scanner* scanner) {
    scanner->column += (scanner->current - scanner->start);
    skipWhitespace(scanner);
    scanner->start = scanner->current;

    if (isAtEnd(scanner)) return makeToken(scanner, TOKEN_EOF);

    const char c = advance(scanner);
    if (isAlpha(c)) return identifier(scanner);
    if (isDigit(c)) return number(scanner);
    if (c == '"') return string(scanner);

    return charToken(scanner, c);
}
