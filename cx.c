#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// debug

#define DEBUG 1

#if DEBUG
#	define DEBUG_TRACE(...) fprintf(stderr, "[TRACE]: " __VA_ARGS__)
#else
#	define DEBUG_TRACE(...)
#endif

// helpers

// void info(char *format, ...) {
// 	fprintf(stderr, "INFO: ");
// 	va_list val;
// 	va_start(val, format);
// 	vfprintf(stderr, format, val);
// 	va_end(val);
// }

// void warn(char *format, ...) {
// 	fprintf(stderr, "WARNING: ");
// 	va_list val;
// 	va_start(val, format);
// 	vfprintf(stderr, format, val);
// 	va_end(val);
// }

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

// darray

#define DARRAY(T) darray_##T
#define DARRAY_INIT(T) darray_init_##T
#define DARRAY_PUSH(T) darray_push_##T
#define DARRAY_FREE(T) darray_free_##T
#define FORWARD_DECLARE_DARRAY(T)	\
typedef struct DARRAY(T) DARRAY(T);
#define DECLARE_DARRAY(T)										\
																\
struct DARRAY(T) {												\
	T *data;													\
	size_t len;													\
	size_t _allocated;											\
};																\
																\
void DARRAY_INIT(T)(DARRAY(T) *da, size_t n) {					\
	da->data = malloc(n * sizeof(T));							\
	da->len = 0;												\
	da->_allocated = n;											\
}																\
																\
void DARRAY_PUSH(T)(DARRAY(T) *da, T t) {						\
	if (da->len == da->_allocated) {							\
	  da->_allocated *= 2;										\
	  da->data = realloc(da->data, da->_allocated * sizeof(T));	\
	}															\
	da->data[da->len++] = t;									\
}																\
																\
void DARRAY_FREE(T)(DARRAY(T) *da) {							\
	free(da->data);												\
	da->data = NULL;											\
	da->len = da->_allocated = 0;								\
}

FORWARD_DECLARE_DARRAY(char)
DECLARE_DARRAY(char)

// StringView

typedef struct {
	char *data;
	size_t size;
} StringView;

#define PRIsv "%.*s"
#define PRIsv_arg(sv) (int) (sv).size, (sv).data

StringView sv_from_cstr(const char *str) {
	StringView s;
	s.data = (char*) str;
	s.size = strlen(str);
	return s;
}

bool sveq(StringView *a, StringView *b) {
	if(a->size != b->size) return false;
	for(size_t i = 0; i < a->size; ++i)
		if(a->data[i] != b->data[i])
			return false;
	return true;
}

// "HashMap"

FORWARD_DECLARE_DARRAY(StringView)
DECLARE_DARRAY(StringView)

typedef struct {
	DARRAY(StringView) _from, _to;
} HashMap;

void HashMap_init(HashMap *h) {
	DARRAY_INIT(StringView)(&h->_from, 1);
	DARRAY_INIT(StringView)(&h->_to, 1);
}

StringView *HashMap_at(HashMap *h, StringView from) {
	for(size_t i = 0; i < h->_from.len; ++i) {
		if(sveq(&h->_from.data[i], &from)) {
			return &h->_to.data[i];
		}
	}
	return NULL;
}

void HashMap_put(HashMap *h, StringView from, StringView to) {
	for(size_t i = 0; i < h->_from.len; ++i) {
		if(sveq(&h->_from.data[i], &from)) {
			h->_to.data[i] = to;
			return;
		}
	}
	DARRAY_PUSH(StringView)(&h->_from, from);
	DARRAY_PUSH(StringView)(&h->_to, to);
}

void HashMap_free(HashMap *h) {
	DARRAY_FREE(StringView)(&h->_from);
	DARRAY_FREE(StringView)(&h->_to);
}

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

void loc_panic(Location location, char *format, ...) {
	Location_print(location, stderr);
	fprintf(stderr, ": ERROR: ");
	va_list val;
	va_start(val, format);
	vfprintf(stderr, format, val);
	va_end(val);
	exit(1);
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
			return "NULL_TOKEN";
		case TOKEN_NAME:
			return "NAME";
		case TOKEN_OPEN_PARENTHESIS:
			return "OPEN_PARENTHESIS";
		case TOKEN_OPEN_CURLY:
			return "OPEN_CURLY";
		case TOKEN_CLOSE_PARENTHESIS:
			return "CLOSE_PARENTHESIS";
		case TOKEN_CLOSE_CURLY:
			return "CLOSE_CURLY";
		case TOKEN_OPEN_SQUARE:
			return "OPEN_SQUARE";
		case TOKEN_CLOSE_SQUARE:
			return "CLOSE_SQUARE";
		case TOKEN_DOT:
			return "DOT";
		case TOKEN_COMMA:
			return "COMMA";
		case TOKEN_SEMICOLON:
			return "SEMICOLON";
		case TOKEN_EQUALS:
			return "EQUALS";
		case TOKEN_NUMBER:
			return "NUMBER";
		case TOKEN_STRING:
			return "STRING";
		case TOKEN_PLUS:
			return "PLUS";
		case TOKEN_PLUS_EQUALS:
			return "PLUS_EQUALS";
		case TOKEN_PLUS_PLUS:
			return "PLUS_PLUS";
		case TOKEN_MINUS:
			return "MINUS";
		case TOKEN_MINUS_EQUALS:
			return "MINUS_EQUALS";
		case TOKEN_MINUS_MINUS:
			return "MINUS_MINUS";
		case TOKEN_ASTERISK:
			return "ASTERISK";
		case TOKEN_TIMES_EQUALS:
			return "TIMES_EQUALS";
		case TOKEN_SLASH:
			return "SLASH";
		case TOKEN_DIVIDE_EQUALS:
			return "DIVIDE_EQUALS";
		case TOKEN_LESS_THAN:
			return "LESS_THAN";
		case TOKEN_GREATER_THAN:
			return "GREATER_THAN";
		case TOKEN_NOT:
			return "NOT";
		case TOKEN_ARROW:
			return "ARROW";
		case TOKEN_CHAR:
			return "CHAR";
		case TOKEN_AMPERSTAND:
			return "AMPERSTAND";
		case TOKEN_AND_EQUALS:
			return "AND_EQUALS";
		case TOKEN_LOGIC_AND:
			return "LOGIC_AND";
		case TOKEN_PIPE:
			return "PIPE";
		case TOKEN_OR_EQUALS:
			return "OR_EQUALS";
		case TOKEN_LOGIC_OR:
			return "LOGIC_OR";
		case TOKEN_XOR:
			return "XOR";
		case TOKEN_XOR_EQUALS:
			return "XOR_EQUALS";
		case TOKEN_MOD:
			return "MOD";
		case TOKEN_MOD_EQUALS:
			return "MOD_EQUALS";
		case TOKEN_COLON:
			return "COLON";
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

FORWARD_DECLARE_DARRAY(Token)
DECLARE_DARRAY(Token)

void Token_print(Token token) {
	if(!token.type) return;
	Location_print(token.location, stderr);
	fprintf(stderr, ": %s ", Token_Type_to_string(token.type));
	switch(token.type) {
		case TOKEN_NULL: assert(false && "unreachable"); break;
		case TOKEN_NAME:
			printf("'" PRIsv "'\n", PRIsv_arg(token.value_sv));
			break;
		case TOKEN_NUMBER:
			printf("%d\n", token.value_int);
			break;
		case TOKEN_CHAR:
			printf("'" PRIsv "'\n", PRIsv_arg(token.value_sv));
			break;
		case TOKEN_STRING:
			printf("\"" PRIsv "\"\n", PRIsv_arg(token.value_sv));
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
			printf("\b\n");
			break;
		case TOKEN_PLUS:
		case TOKEN_MINUS:
		case TOKEN_ASTERISK:
		case TOKEN_SLASH:
		case TOKEN_AMPERSTAND:
		case TOKEN_PIPE:
		case TOKEN_XOR:
		case TOKEN_MOD:
			printf("\b\n");
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
			printf("\b\n");
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
		if(!(s[0] == '/' && s[1] == '/')) break;
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
	literal_tokens['.'] = TOKEN_DOT;
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

	loc_panic(location, "unknown token starts with '%c' = 0x%x = %d\n", first, first, first);
	return (Token) { 0 };
}

// AST

typedef enum {
	CX_AST_NODE_TYPE_NULL,
	CX_AST_NODE_TYPE_ROOT,
	// CX_AST_NODE_TYPE_NUMBER, // value:NUMBER
	// CX_AST_NODE_TYPE_STRING, // value:STRING
	// CX_AST_NODE_TYPE_DATA_TYPE, // name:NAME
	// CX_AST_NODE_TYPE_VARIABLE, // name:NAME
	CX_AST_NODE_TYPE_FUNCTION, // name:NAME, types:[NAME], names:[NAME], body:AST
	// CX_AST_NODE_TYPE_FUNCALL, // func:AST, args:[AST]
} CX_AST_Node_Type;

typedef struct CX_AST_Node CX_AST_Node;

FORWARD_DECLARE_DARRAY(CX_AST_Node)

struct CX_AST_Node {
	CX_AST_Node_Type type;
	CX_AST_Node *parent;
	union {
		struct {
			CX_AST_Node *data;
			size_t len;
			size_t _allocated;
		} u_root;
		// struct {
		// 	Token value;
		// } u_number;
		// struct {
		// 	Token name;
		// } u_data_type;
		struct {
			Token data_type;
			Token name;
			// TODO: types
			// TODO: names
			CX_AST_Node *body;
		} u_function;
	};
};

DECLARE_DARRAY(CX_AST_Node)

void CX_AST_Node_free(CX_AST_Node node) {
	switch(node.type) {
		case CX_AST_NODE_TYPE_NULL:
			assert(false && "unreachable");
			break;
		case CX_AST_NODE_TYPE_ROOT:
			for(size_t i = 0; i < node.u_root.len; ++i)
				CX_AST_Node_free(node.u_root.data[i]);
			DARRAY_FREE(CX_AST_Node)((DARRAY(CX_AST_Node)*) &node.u_root);
			break;
		// case CX_AST_NODE_TYPE_NUMBER:
		// case CX_AST_NODE_TYPE_DATA_TYPE:
		case CX_AST_NODE_TYPE_FUNCTION:
		default:
			break;
	}
}

void CX_AST_Node_print_json(CX_AST_Node *node, FILE *sink) {
	fprintf(sink, "{");
	if(node) switch(node->type) {
		case CX_AST_NODE_TYPE_NULL:
			assert(false && "unreachable");
			break;
		case CX_AST_NODE_TYPE_ROOT:
			fprintf(sink, "\"root\":{\"children\":[");
			if(node->u_root.len > 0)
				CX_AST_Node_print_json(&node->u_root.data[0], sink);
			for(size_t i = 1; i < node->u_root.len; ++i) {
				fprintf(sink, ",");
				CX_AST_Node_print_json(&node->u_root.data[i], sink);
			}
			fprintf(sink, "]}");
			break;
		case CX_AST_NODE_TYPE_FUNCTION:
			fprintf(sink, "\"function\":{\"type\":\"" PRIsv "\",\"name\":\"" PRIsv "\",\"body\":",
				PRIsv_arg(node->u_function.data_type.value_sv),
				PRIsv_arg(node->u_function.name.value_sv)
			);
			CX_AST_Node_print_json(node->u_function.body, sink);
			fprintf(sink, "}");
			break;
		default:
			assert(false && "unreachable");
			break;
	}
	fprintf(sink, "}");
}

void CX_AST_Node_root(CX_AST_Node *root) {
	root->type = CX_AST_NODE_TYPE_ROOT;
    root->parent = NULL;
	DARRAY_INIT(CX_AST_Node)((DARRAY(CX_AST_Node)*) &root->u_root, 1);
}

typedef struct {
	DARRAY(Token) *tokens;
	size_t cur;
} Parser;

Token Parser_next_token(Parser *parser) {
	if(parser->cur < parser->tokens->len) {
		return parser->tokens->data[parser->cur++];
	} else {
		return (Token) { 0 };
	}
}

Token __IMPL__Parser_expect_token(bool necessary, Parser *parser, ...) {
	Token token = Parser_next_token(parser);
	if(!token.type) {
		va_list val;
		va_start(val, parser);
		if(necessary) loc_error(token.location, "expected '%s", Token_Type_to_string(va_arg(val, Token_Type)));
		Token_Type t = va_arg(val, Token_Type);
		while(t) {
			if(necessary) fprintf(stderr, " or %s", Token_Type_to_string(t));
			t = va_arg(val, Token_Type);
		}
		va_end(val);
		if(necessary) fprintf(stderr, "' but file ended\n");
	}

	{
		va_list val;
		Token_Type t;
		va_start(val, parser);
		do {
			t = va_arg(val, Token_Type);
			if(t == token.type) return token;
		} while(t);
		va_end(val);
	}

	{
		va_list val;
		va_start(val, parser);
		if(necessary) loc_error(token.location, "expected '%s", Token_Type_to_string(va_arg(val, Token_Type)));
		Token_Type t = va_arg(val, Token_Type);
		while(t) {
			if(necessary) fprintf(stderr, " or %s", Token_Type_to_string(t));
			t = va_arg(val, Token_Type);
		}
		va_end(val);
		if(necessary) fprintf(stderr, "' but got '%s'\n", Token_Type_to_string(token.type));

		return (Token) { 0 };
	}
}

#define Parser_expect_token(n, p, ...) __IMPL__Parser_expect_token(n, p, __VA_ARGS__, 0)

bool Parser_next_function(Parser *parser, CX_AST_Node *node) {
	Token data_type = Parser_expect_token(false, parser, TOKEN_NAME);
	if(!data_type.type) return false;
	node->u_function.data_type = data_type;

	Token name = Parser_expect_token(false, parser, TOKEN_NAME);
	if(!name.type) return false;
	node->u_function.name = name;

	Token op = Parser_expect_token(false, parser, TOKEN_OPEN_PARENTHESIS);
	if(!op.type) return false;

	// TODO: parse parameters

	Token cp = Parser_expect_token(false, parser, TOKEN_CLOSE_PARENTHESIS);
	if(!cp.type) return false;

	Token oc = Parser_expect_token(false, parser, TOKEN_OPEN_CURLY);
	if(!oc.type) return false;

	node->u_function.body = NULL; // TODO: parse function body

	Token cc = Parser_expect_token(false, parser, TOKEN_CLOSE_CURLY);
	if(!cc.type) return false;

	node->type = CX_AST_NODE_TYPE_FUNCTION;
	return true;
}

void Parser_next_node(Parser *parser, CX_AST_Node *node) {
	node->type = 0;
	size_t saved_cur = parser->cur;
	
	{ // function
		if(Parser_next_function(parser, node)) return;
		parser->cur = saved_cur;
	}
}

// Semantic analysis

typedef struct {
	HashMap *data_types;
} SemanticStructure;

void analyse_semantics(CX_AST_Node *ast, SemanticStructure *semantic_structure) {
	if(ast) switch(ast->type) {
		case CX_AST_NODE_TYPE_NULL:
			assert(false && "unreachable");
			break;
		case CX_AST_NODE_TYPE_ROOT:
			for(size_t i = 0; i < ast->u_root.len; ++i)
				analyse_semantics(&ast->u_root.data[i], semantic_structure);
			break;
		case CX_AST_NODE_TYPE_FUNCTION:
			if(!HashMap_at(semantic_structure->data_types, ast->u_function.data_type.value_sv))
				loc_error(ast->u_function.data_type.location, "unknown data type: " PRIsv "\n", PRIsv_arg(ast->u_function.data_type.value_sv));
			analyse_semantics(ast->u_function.body, semantic_structure);
			break;
		default:
			break;
	}
}

//

bool streq(char *a, char *b) {
	return strcmp(a, b) == 0;
}

void usage(char *program_name, FILE *sink) {
	fprintf(sink, "Usage: %s [options] <file.cx>\n", program_name);
	fprintf(sink, "Options:\n");
	fprintf(sink, "    -h, --help Print this message\n");
	fprintf(sink, "    --dump-ast Print this message\n");
}

void alloc_file_content(DARRAY(char) *array, char *filename, const char *mode) {
	FILE *fp = fopen(filename, mode);

	if(fp) {
		fseek(fp, 0, SEEK_END);
		DARRAY_INIT(char)(array, ftell(fp) + 1);
		fseek(fp, 0, SEEK_SET);
		array->len = fread(array->data, 1, array->_allocated, fp);
		if(errno) {
			DARRAY_FREE(char)(array);
			fclose(fp);
			panic("error reading file %s: %s\n", filename, strerror(errno));
		}
		array->data[array->len] = 0;
	} else {
		panic("could not open file: %s\n", filename);
	}

	fclose(fp);
}

char *program_name = NULL;
char *source_filename = NULL;
bool dump_ast = false;

int main(int argc, char **argv) {
	program_name = consume_arg(&argc, &argv);

	while (argc) {
		char *flag = consume_arg(&argc, &argv);
		if (streq(flag, "-h") || streq(flag, "--help")) {
			usage(program_name, stderr);
			exit(0);
		} else if (streq(flag, "--dump-ast")) {
			dump_ast = true;
		} else {
			if(source_filename) {
				error("At the moment CX does not support compiling multiple files at once\n");
				usage(program_name, stderr);
				exit(1);
			} else {
				source_filename = flag;
			}
		}
	}

	if(!source_filename) {
		error("no input file provided\n");
		usage(program_name, stderr);
		exit(1);
	}

	{
		DARRAY(char) source_code;
		DARRAY(Token) tokens;
		CX_AST_Node root;
		HashMap data_type_translations;

		{
			DEBUG_TRACE("Lexical analysis\n");

			alloc_file_content(&source_code, source_filename, "r");
			
			Lexer lexer = {
				.file_path = source_filename,
				.source = source_code.data,
				.source_len = source_code.len
			};

			DARRAY_INIT(Token)(&tokens, 1);

			while(Lexer_is_not_empty(&lexer)) {
				Token token = Lexer_next_token(&lexer);
				DARRAY_PUSH(Token)(&tokens, token);
			}

		}

		{
			DEBUG_TRACE("Parsing\n");

			Parser parser = {
				.tokens = &tokens,
				.cur = 0
			};

			CX_AST_Node_root(&root);

			while(true) {
				CX_AST_Node node;
				Parser_next_node(&parser, &node);
				if(node.type) {
					DARRAY_PUSH(CX_AST_Node)((DARRAY(CX_AST_Node)*) &root.u_root, node);
				} else {
					break;
				}
			};

			if(dump_ast) CX_AST_Node_print_json(&root, stdout);

		}

		{
			DEBUG_TRACE("Semantic analysis\n");

			HashMap_init(&data_type_translations);

			HashMap_put(&data_type_translations, sv_from_cstr("b8"), sv_from_cstr("_Bool"));
			// HashMap_put(&data_type_translations, sv_from_cstr("b32"), sv_from_cstr("int"));
			HashMap_put(&data_type_translations, sv_from_cstr("i8"),  sv_from_cstr("signed char"));
			HashMap_put(&data_type_translations, sv_from_cstr("i16"), sv_from_cstr("signed short"));
			HashMap_put(&data_type_translations, sv_from_cstr("i32"), sv_from_cstr("signed int"));
			HashMap_put(&data_type_translations, sv_from_cstr("i64"), sv_from_cstr("signed long long"));
			HashMap_put(&data_type_translations, sv_from_cstr("u8"),  sv_from_cstr("unsigned char"));
			HashMap_put(&data_type_translations, sv_from_cstr("u16"), sv_from_cstr("unsigned short"));
			HashMap_put(&data_type_translations, sv_from_cstr("u32"), sv_from_cstr("unsigned int"));
			HashMap_put(&data_type_translations, sv_from_cstr("u64"), sv_from_cstr("unsigned long long"));
			HashMap_put(&data_type_translations, sv_from_cstr("f32"), sv_from_cstr("float"));
			HashMap_put(&data_type_translations, sv_from_cstr("f64"), sv_from_cstr("double"));

			SemanticStructure semantic_structure = {
				.data_types = &data_type_translations
			};

			analyse_semantics(&root, &semantic_structure);
		}

		{
			DEBUG_TRACE("Code generation\n");
		}

		HashMap_free(&data_type_translations);
		CX_AST_Node_free(root);
		DARRAY_FREE(Token)(&tokens);
		DARRAY_FREE(char)(&source_code);
	}

	return 0;
}