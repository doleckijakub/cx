#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// helpers

void info(char *format, ...) {
	fprintf(stderr, "INFO: ");
	va_list val;
	va_start(val, format);
	vfprintf(stderr, format, val);
	va_end(val);
}

void warn(char *format, ...) {
	fprintf(stderr, "WARNING: ");
	va_list val;
	va_start(val, format);
	vfprintf(stderr, format, val);
	va_end(val);
}

void error(char *format, ...) {
	fprintf(stderr, "ERROR: ");
	va_list val;
	va_start(val, format);
	vfprintf(stderr, format, val);
	va_end(val);
}

void panic(char *format, ...) {
	fprintf(stderr, "FATAL ERROR: ");
	va_list val;
	va_start(val, format);
	vfprintf(stderr, format, val);
	va_end(val);
	exit(1);
}

char *consume_arg(int *argc, char ***argv)
{
	assert(*argc > 0);
	char *result = **argv;
	*argc -= 1;
	*argv += 1;
	return result;
}

// StringView

typedef struct {
	char *data;
	size_t size;
} StringView;

// Location

typedef struct {
	char *file_path;
	size_t line, row;
} Location;

void Location_print(Location location, FILE *fp) {
	fprintf(fp, "%s:%lu:%lu", location.file_path, (unsigned long) location.line + 1, (unsigned long) location.row + 1);
}

void loc_error(Location location, char *format, ...) {
	Location_print(location, stderr);
	fprintf(stderr, ": ERROR: ");
	va_list val;
	va_start(val, format);
	vfprintf(stderr, format, val);
	va_end(val);
}

// Token

typedef enum {
	TOKEN_NULL,
	TOKEN_NAME,
	TOKEN_OPEN_PARENTHESIS,
	TOKEN_OPEN_CURLY,
	TOKEN_CLOSE_PARENTHESIS,
	TOKEN_CLOSE_CURLY,
	TOKEN_OPEN_SQUARE,
	TOKEN_CLOSE_SQUARE,
	TOKEN_DOT,
	TOKEN_COMMA,
	TOKEN_SEMICOLON,
	TOKEN_EQUALS,
	TOKEN_NUMBER,
	TOKEN_STRING,
	TOKEN_PLUS,
	TOKEN_PLUS_EQUALS,
	TOKEN_PLUS_PLUS,
	TOKEN_MINUS,
	TOKEN_MINUS_EQUALS,
	TOKEN_MINUS_MINUS,
	TOKEN_ASTERISK,
	TOKEN_TIMES_EQUALS,
	TOKEN_SLASH,
	TOKEN_DIVIDE_EQUALS,
	TOKEN_LESS_THAN,
	TOKEN_GREATER_THAN,
	TOKEN_NOT,
	TOKEN_ARROW,
	TOKEN_CHAR,
	TOKEN_AMPERSTAND,
	TOKEN_AND_EQUALS,
	TOKEN_LOGIC_AND,
	TOKEN_PIPE,
	TOKEN_OR_EQUALS,
	TOKEN_LOGIC_OR,
	TOKEN_XOR,
	TOKEN_XOR_EQUALS,
	TOKEN_MOD,
	TOKEN_MOD_EQUALS,
	TOKEN_COLON,
} Token_Type;

const char* Token_Type_to_string(Token_Type type) {
	switch(type) {
		case TOKEN_NULL:
			return "TOKEN_NULL";
		case TOKEN_NAME:
			return "TOKEN_NAME";
		case TOKEN_OPEN_PARENTHESIS:
			return "TOKEN_OPEN_PARENTHESIS";
		case TOKEN_OPEN_CURLY:
			return "TOKEN_OPEN_CURLY";
		case TOKEN_CLOSE_PARENTHESIS:
			return "TOKEN_CLOSE_PARENTHESIS";
		case TOKEN_CLOSE_CURLY:
			return "TOKEN_CLOSE_CURLY";
		case TOKEN_OPEN_SQUARE:
			return "TOKEN_OPEN_SQUARE";
		case TOKEN_CLOSE_SQUARE:
			return "TOKEN_CLOSE_SQUARE";
		case TOKEN_DOT:
			return "TOKEN_DOT";
		case TOKEN_COMMA:
			return "TOKEN_COMMA";
		case TOKEN_SEMICOLON:
			return "TOKEN_SEMICOLON";
		case TOKEN_EQUALS:
			return "TOKEN_EQUALS";
		case TOKEN_NUMBER:
			return "TOKEN_NUMBER";
		case TOKEN_STRING:
			return "TOKEN_STRING";
		case TOKEN_PLUS:
			return "TOKEN_PLUS";
		case TOKEN_PLUS_EQUALS:
			return "TOKEN_PLUS_EQUALS";
		case TOKEN_PLUS_PLUS:
			return "TOKEN_PLUS_PLUS";
		case TOKEN_MINUS:
			return "TOKEN_MINUS";
		case TOKEN_MINUS_EQUALS:
			return "TOKEN_MINUS_EQUALS";
		case TOKEN_MINUS_MINUS:
			return "TOKEN_MINUS_MINUS";
		case TOKEN_ASTERISK:
			return "TOKEN_ASTERISK";
		case TOKEN_TIMES_EQUALS:
			return "TOKEN_TIMES_EQUALS";
		case TOKEN_SLASH:
			return "TOKEN_SLASH";
		case TOKEN_DIVIDE_EQUALS:
			return "TOKEN_DIVIDE_EQUALS";
		case TOKEN_LESS_THAN:
			return "TOKEN_LESS_THAN";
		case TOKEN_GREATER_THAN:
			return "TOKEN_GREATER_THAN";
		case TOKEN_NOT:
			return "TOKEN_NOT";
		case TOKEN_ARROW:
			return "TOKEN_ARROW";
		case TOKEN_CHAR:
			return "TOKEN_CHAR";
		case TOKEN_AMPERSTAND:
			return "TOKEN_AMPERSTAND";
		case TOKEN_AND_EQUALS:
			return "TOKEN_AND_EQUALS";
		case TOKEN_LOGIC_AND:
			return "TOKEN_LOGIC_AND";
		case TOKEN_PIPE:
			return "TOKEN_PIPE";
		case TOKEN_OR_EQUALS:
			return "TOKEN_OR_EQUALS";
		case TOKEN_LOGIC_OR:
			return "TOKEN_LOGIC_OR";
		case TOKEN_XOR:
			return "TOKEN_XOR";
		case TOKEN_XOR_EQUALS:
			return "TOKEN_XOR_EQUALS";
		case TOKEN_MOD:
			return "TOKEN_MOD";
		case TOKEN_MOD_EQUALS:
			return "TOKEN_MOD_EQUALS";
		case TOKEN_COLON:
			return "TOKEN_COLON";
	}
	assert(false && "unreachable");
	return NULL;
}

typedef struct {
	Location location;
	Token_Type type;
	union {
		StringView value_sv;
		char value_char;
		int value_int;
	};
} Token;

void Token_print(Token token) {
	if(!token.type) return;
	Location_print(token.location, stderr);
	fprintf(stderr, ": %s ", Token_Type_to_string(token.type));
	switch(token.type) {
		case TOKEN_NULL: assert(false && "unreachable"); break;
		case TOKEN_NAME:
			printf("'%.*s'\n", (int) token.value_sv.size, token.value_sv.data);
			break;
		case TOKEN_NUMBER:
			printf("%d\n", token.value_int);
			break;
		case TOKEN_CHAR:
			printf("'%.*s'\n", (int) token.value_sv.size, token.value_sv.data);
			break;
		case TOKEN_STRING:
			printf("\"%.*s\"\n", (int) token.value_sv.size, token.value_sv.data);
			break;
		case TOKEN_OPEN_PARENTHESIS:
		case TOKEN_OPEN_CURLY:
		case TOKEN_CLOSE_PARENTHESIS:
		case TOKEN_CLOSE_CURLY:
		case TOKEN_OPEN_SQUARE:
		case TOKEN_CLOSE_SQUARE:
		case TOKEN_DOT:
		case TOKEN_COMMA:
		case TOKEN_SEMICOLON:
		case TOKEN_EQUALS:
		case TOKEN_LESS_THAN:
		case TOKEN_GREATER_THAN:
		case TOKEN_NOT:
		case TOKEN_COLON:
			printf("'%c'\n", token.value_char);
			break;
		case TOKEN_PLUS:
		case TOKEN_MINUS:
		case TOKEN_ASTERISK:
		case TOKEN_SLASH:
		case TOKEN_AMPERSTAND:
		case TOKEN_PIPE:
		case TOKEN_XOR:
		case TOKEN_MOD:
			printf("'%.1s'\n", token.value_sv.data);
			break;
		case TOKEN_PLUS_EQUALS:
		case TOKEN_PLUS_PLUS:
		case TOKEN_MINUS_EQUALS:
		case TOKEN_MINUS_MINUS:
		case TOKEN_TIMES_EQUALS:
		case TOKEN_DIVIDE_EQUALS:
		case TOKEN_ARROW:
		case TOKEN_AND_EQUALS:
		case TOKEN_OR_EQUALS:
		case TOKEN_XOR_EQUALS:
		case TOKEN_MOD_EQUALS:
		case TOKEN_LOGIC_AND:
		case TOKEN_LOGIC_OR:
			printf("'%.2s'\n", token.value_sv.data);
			break;
	}
}

// Lexer

typedef struct {
	char *file_path;
	char *source;
	size_t source_len;
	size_t cur, bol, row;
} Lexer;

Location Lexer_location(Lexer* lexer) {
	return (Location) {
		.file_path = lexer->file_path,
		.line = lexer->row,
		.row = lexer->cur - lexer->bol
	};
}

bool Lexer_is_not_empty(Lexer* lexer) {
	return lexer->cur < lexer->source_len;
}

bool Lexer_is_empty(Lexer* lexer) {
	return !(Lexer_is_not_empty(lexer));
}

void Lexer_chop_char(Lexer* lexer) {
	if(Lexer_is_not_empty(lexer)) {
		char c = lexer->source[lexer->cur++];
		if(c == '\n') {
			lexer->bol = lexer->cur;
			++lexer->row;
		}
	}
}

void Lexer_trim(Lexer* lexer) {
	while(Lexer_is_not_empty(lexer) && isspace(lexer->source[lexer->cur])) {
		Lexer_chop_char(lexer);
	}
}

void Lexer_drop(Lexer* lexer) {
	while(Lexer_is_not_empty(lexer) && lexer->source[lexer->cur] != '\n') {
		Lexer_chop_char(lexer);
	}
	if(Lexer_is_not_empty(lexer)) {
		Lexer_chop_char(lexer);
	}
}

Token Lexer_next_token(Lexer* lexer) {
	Lexer_trim(lexer);
	while(Lexer_is_not_empty(lexer)) {
		char *s = lexer->source + lexer->cur;
		if(!(s[0] == '#' || (s[0] == '/' && s[1] == '/'))) break;
		Lexer_drop(lexer);
		Lexer_trim(lexer);
	}

	if(Lexer_is_empty(lexer)) return (Token) { 0 };

	Location location = Lexer_location(lexer);
	char first = lexer->source[lexer->cur];

	if(isalpha(first) || first == '_') {
		size_t index = lexer->cur;
		while(Lexer_is_not_empty(lexer) && (isalnum(lexer->source[lexer->cur]) || lexer->source[lexer->cur] == '_')) {
			Lexer_chop_char(lexer);
		}

		return (Token) {
			.location = location,
			.type = TOKEN_NAME,
			.value_sv = (StringView) {
				.data = lexer->source + index,
				.size = lexer->cur - index
			}
		};
	}

	Token_Type literal_tokens[256] = { 0 };
	literal_tokens['('] = TOKEN_OPEN_PARENTHESIS;
	literal_tokens[')'] = TOKEN_CLOSE_PARENTHESIS;
	literal_tokens['{'] = TOKEN_OPEN_CURLY;
	literal_tokens['}'] = TOKEN_CLOSE_CURLY;
	literal_tokens['['] = TOKEN_OPEN_SQUARE;
	literal_tokens[']'] = TOKEN_CLOSE_SQUARE;
	literal_tokens['.'] = TOKEN_COMMA;
	literal_tokens[','] = TOKEN_COMMA;
	literal_tokens[';'] = TOKEN_SEMICOLON;
	literal_tokens[':'] = TOKEN_COLON;
	literal_tokens['='] = TOKEN_EQUALS;
	literal_tokens['<'] = TOKEN_LESS_THAN;
	literal_tokens['>'] = TOKEN_GREATER_THAN;
	literal_tokens['!'] = TOKEN_NOT;

	if(literal_tokens[(int) first]) {
		Lexer_chop_char(lexer);
		return (Token) {
			.location = location,
			.type = literal_tokens[(int) first],
			.value_char = lexer->source[lexer->cur - 1]
		};
	}

	if(first == '"') {
		Lexer_chop_char(lexer);
		size_t start = lexer->cur;
		while(Lexer_is_not_empty(lexer)) {
			char c = lexer->source[lexer->cur];
			switch(c) {
				case '"': goto finished_lexing_string;
				case '\\': {
					Lexer_chop_char(lexer);
					Lexer_chop_char(lexer);
				} break;
				default: {
					Lexer_chop_char(lexer);
				}
			}
		}

		finished_lexing_string:

		if(Lexer_is_not_empty(lexer)) {
			Lexer_chop_char(lexer);
			return (Token) {
				.location = location,
				.type = TOKEN_STRING,
				.value_sv = (StringView) {
					.data = lexer->source + start,
					.size = lexer->cur - start - 1
				}
			};
		}
	}

	if(first == '\'') {
		Lexer_chop_char(lexer);
		size_t start = lexer->cur;
		while(Lexer_is_not_empty(lexer)) {
			char c = lexer->source[lexer->cur];
			switch(c) {
				case '\'': goto finished_lexing_char;
				case '\\': {
					Lexer_chop_char(lexer);
					Lexer_chop_char(lexer);
				} break;
				default: {
					Lexer_chop_char(lexer);
				}
			}
		}

		finished_lexing_char:

		if(Lexer_is_not_empty(lexer)) {
			Lexer_chop_char(lexer);
			return (Token) {
				.location = location,
				.type = TOKEN_CHAR,
				.value_sv = (StringView) {
					.data = lexer->source + start,
					.size = lexer->cur - start - 1
				}
			};
		}
	}

	if(isdigit(first)) {
		size_t start = lexer->cur;
		while(Lexer_is_not_empty(lexer) && isdigit(lexer->source[lexer->cur])) {
			Lexer_chop_char(lexer);
		}

		return (Token) {
			.location = location,
			.type = TOKEN_NUMBER,
			.value_int = atoi(lexer->source + start)
		};
	}

	{

		Token_Type type;
		size_t start = lexer->cur;

		switch(first) {
			case '+': {
				Lexer_chop_char(lexer);
				type = TOKEN_PLUS;
				if(Lexer_is_not_empty(lexer) && lexer->source[lexer->cur] == '=')
					type = TOKEN_PLUS_EQUALS;
				else if(Lexer_is_not_empty(lexer) && lexer->source[lexer->cur] == '+')
					type = TOKEN_PLUS_PLUS;
			} break;
			case '-': {
				Lexer_chop_char(lexer);
				type = TOKEN_MINUS;
				if(Lexer_is_not_empty(lexer) && lexer->source[lexer->cur] == '=')
					type = TOKEN_MINUS_EQUALS;
				else if(Lexer_is_not_empty(lexer) && lexer->source[lexer->cur] == '+')
					type = TOKEN_MINUS_MINUS;
				else if(Lexer_is_not_empty(lexer) && lexer->source[lexer->cur] == '>')
					type = TOKEN_ARROW;
			} break;
			case '*': {
				Lexer_chop_char(lexer);
				type = TOKEN_ASTERISK;
				if(Lexer_is_not_empty(lexer) && lexer->source[lexer->cur] == '=')
					type = TOKEN_TIMES_EQUALS;
			} break;
			case '/': {
				Lexer_chop_char(lexer);
				type = TOKEN_SLASH;
				if(Lexer_is_not_empty(lexer) && lexer->source[lexer->cur] == '=')
					type = TOKEN_DIVIDE_EQUALS;
			} break;
			case '&': {
				Lexer_chop_char(lexer);
				type = TOKEN_AMPERSTAND;
				if(Lexer_is_not_empty(lexer) && lexer->source[lexer->cur] == '=')
					type = TOKEN_AND_EQUALS;
				else if(Lexer_is_not_empty(lexer) && lexer->source[lexer->cur] == '&')
					type = TOKEN_LOGIC_AND;
			} break;
			case '|': {
				Lexer_chop_char(lexer);
				type = TOKEN_PIPE;
				if(Lexer_is_not_empty(lexer) && lexer->source[lexer->cur] == '=')
					type = TOKEN_OR_EQUALS;
				else if(Lexer_is_not_empty(lexer) && lexer->source[lexer->cur] == '|')
					type = TOKEN_LOGIC_OR;
			} break;
			case '^': {
				Lexer_chop_char(lexer);
				type = TOKEN_XOR;
				if(Lexer_is_not_empty(lexer) && lexer->source[lexer->cur] == '=')
					type = TOKEN_XOR_EQUALS;
			} break;
			case '%': {
				Lexer_chop_char(lexer);
				type = TOKEN_MOD;
				if(Lexer_is_not_empty(lexer) && lexer->source[lexer->cur] == '=')
					type = TOKEN_MOD_EQUALS;
			} break;
			default: goto not_operand;
		}

		return (Token) {
			.location = location,
			.type = type,
			.value_sv = (StringView) {
				.data = lexer->source + start
			}
		};

	}

	not_operand:

	loc_error(location, "unknown token starts with '%c'\n", first);
	return (Token) { 0 };
}

Token Lexer_expect_token(Lexer *lexer, ...) {
	Token token = Lexer_next_token(lexer);
	if(!token.type) {
		loc_error(token.location, "expected '");
		va_list val;
		va_start(val, lexer);
		printf("%s", Token_Type_to_string(va_arg(val, Token_Type)));
		Token_Type t = va_arg(val, Token_Type);
		while(t) {
			printf(" or %s", Token_Type_to_string(t));
			t = va_arg(val, Token_Type);
		}
		va_end(val);
		fprintf(stderr, "' but file ended\n");
	}

	{
		va_list val;
		Token_Type t;
		do {
			t = va_arg(val, Token_Type);
			if(t == token.type) return token;
		} while(t);
		va_end(val);
	}

	{
		loc_error(token.location, "expected '");
		va_list val;
		va_start(val, lexer);
		printf("%s", Token_Type_to_string(va_arg(val, Token_Type)));
		Token_Type t = va_arg(val, Token_Type);
		while(t) {
			printf(" or %s", Token_Type_to_string(t));
			t = va_arg(val, Token_Type);
		}
		va_end(val);
		fprintf(stderr, "' but got '%s'\n", Token_Type_to_string(token.type));

		return (Token) { 0 };
	}
}

// AST

typedef struct {

} CX_Expression;

typedef struct {
	CX_Expression *expressions;
	size_t expression_sz;
} CX_Program;

// Parser

// typedef struct {
// 	Lexer *lexer;
// } Parser;

// CX_Expression Parser_parse_expression(Parser *parser) {

// }

// CX_Program Parser_parse_program(Parser *parser) {
// 	CX_Program program = (CX_Program) { 0 };

// 	while(Lexer_is_not_empty(parser->lexer)) {
// 		CX_Expression expression = Parser_parse_expression(parser);

// 	}
// }

//

void usage(char *program_name) {
	printf("Usage: %s [options] <file.cx>\n", program_name);
	printf("Options:\n");
	printf("    -h    Print this message\n");

}

int main(int argc, char **argv) {
	char *program_name = consume_arg(&argc, &argv);
	char *source_filename = NULL;

	while (argc) {
		char *flag = consume_arg(&argc, &argv);
		if (strcmp(flag, "-h") == 0) {
			usage(program_name);
			exit(0);
		} else {
			if(source_filename) {
				error("At the moment CX does not support compiling multiple files at once\n");
				usage(program_name);
				exit(1);
			} else {
				source_filename = flag;
			}
		}
	}

	if(!source_filename) {
		error("no input file provided\n");
		usage(program_name);
		exit(1);
	}

	FILE *fp = fopen(source_filename, "r");

	char *source_code;
	long source_len;

	if(fp) {
		fseek(fp, 0, SEEK_END);
		source_len = ftell(fp);
		fseek(fp, 0, SEEK_SET);
		source_code = malloc(source_len);
		if(source_code) {
			source_len = fread(source_code, 1, source_len, fp);
			if(errno) {
				fclose(fp);
				free(source_code);
				panic("error reading file %s: %s\n", source_filename, strerror(errno));
			}
		} else {
			fclose(fp);
			panic("could not allocate %db of memory", source_len);
		}
		fclose(fp);
	} else {
		panic("could not open file: %s\n", source_filename);
	}

	info("Lexical analysis\n");

	Lexer lexer = {
		.file_path = source_filename,
		.source = source_code,
		.source_len = source_len
	};

	info("Parsing\n");

	printf("%d\n", Lexer_expect_token(&lexer, TOKEN_STRING, TOKEN_COMMA, 0).type);

	// Parser parser = {

	// }

	info("Semantic analysis\n");

	info("Optimization\n");

	info("Code generation\n");

	free(source_code);

	return 0;
}