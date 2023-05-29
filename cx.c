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

void info(char *format, ...) {
	va_list val;
	va_start(val, format);
	fprintf(stderr, "INFO: ");
	vfprintf(stderr, format, val);
	va_end(val);
}

// void warn(char *format, ...) {
// 	va_list val;
// 	va_start(val, format);
// 	fprintf(stderr, "WARNING: ");
// 	vfprintf(stderr, format, val);
// 	va_end(val);
// }

void error(char *format, ...) {
	va_list val;
	va_start(val, format);
	fprintf(stderr, "ERROR: ");
	vfprintf(stderr, format, val);
	va_end(val);
}

void panic(char *format, ...) {
	va_list val;
	va_start(val, format);
	fprintf(stderr, "FATAL ERROR: ");
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

bool sveqp(StringView *a, StringView *b) {
	if(a->size != b->size) return false;
	for(size_t i = 0; i < a->size; ++i)
		if(a->data[i] != b->data[i])
			return false;
	return true;
}

bool sveq(StringView a, StringView b) {
	return sveqp(&a, &b);
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
		if(sveqp(&h->_from.data[i], &from)) {
			return &h->_to.data[i];
		}
	}
	return NULL;
}

void HashMap_put(HashMap *h, StringView from, StringView to) {
	for(size_t i = 0; i < h->_from.len; ++i) {
		if(sveqp(&h->_from.data[i], &from)) {
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

#define PRIloc "%s:%lu:%lu"
#define PRIloc_arg(loc) (loc).file_path, (unsigned long) (loc).line + 1, (unsigned long) (loc).row + 1

extern DARRAY(char) source_code;

void loc_error(Location location, char *format, ...) {
	va_list val;
	va_start(val, format);
	fprintf(stderr, PRIloc ": ERROR: ", PRIloc_arg(location));
	vfprintf(stderr, format, val);
	va_end(val);
}

void loc_error_cite(Location location) {
	// TODO: print file line, starting at location
	// (void) location;
	size_t line = 0, cur = 0;
	while(line < location.line) {
		if(source_code.data[cur++] == '\n') {
			++line;
		}
	}
	for(size_t i = 0; i < location.row; ++i) ++cur;
	fprintf(stderr, PRIloc ": ERROR: `", PRIloc_arg(location));
	while(source_code.data[cur] != '\n') putc(source_code.data[cur++], stderr);
	fprintf(stderr, "`\n");
}

void loc_panic(Location location, char *format, ...) {
	va_list val;
	va_start(val, format);
	fprintf(stderr, PRIloc ": FATAL ERROR: ", PRIloc_arg(location));
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
	TOKEN_EOF,
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
		case TOKEN_EOF:
			return "EOF";
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
	fprintf(stderr, PRIloc ": %s", PRIloc_arg(token.location), Token_Type_to_string(token.type));
	switch(token.type) {
		case TOKEN_NULL:
			printf(" \n");
			break;
		case TOKEN_NAME:
			printf(" '" PRIsv "'\n", PRIsv_arg(token.value_sv));
			break;
		case TOKEN_NUMBER:
			printf(" %d\n", token.value_int);
			break;
		case TOKEN_CHAR:
			printf(" '" PRIsv "'\n", PRIsv_arg(token.value_sv));
			break;
		case TOKEN_STRING:
			printf(" \"" PRIsv "\"\n", PRIsv_arg(token.value_sv));
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
		case TOKEN_PLUS:
		case TOKEN_MINUS:
		case TOKEN_ASTERISK:
		case TOKEN_SLASH:
		case TOKEN_AMPERSTAND:
		case TOKEN_PIPE:
		case TOKEN_XOR:
		case TOKEN_MOD:
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
		case TOKEN_EOF:
			printf("\n");
			break;
	}
}

// Lexer

typedef struct {
	char *file_path;
	char *source;
	size_t source_len;
	size_t cur, bol, row;
	bool eof;
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

	CX_AST_NODE_TYPE_TYPE_ID,
	CX_AST_NODE_TYPE_NAME_ID,

	CX_AST_NODE_TYPE_NUMBER_LIT,
	CX_AST_NODE_TYPE_STRING_LIT,

	CX_AST_NODE_TYPE_RETURN_STMT,
	CX_AST_NODE_TYPE_COMPOUND_STMT,

	CX_AST_NODE_TYPE_FUNCTION_DECL,
	// CX_AST_NODE_TYPE_VARIABLE, // name:NAME
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

		// Identifiers

		struct {
			Token value; // TODO: change type to AST when introducing macros
		} u_type_id;

		struct {
			Token value; // TODO: change type to AST when introducing macros
		} u_name_id;

		// Literals

		struct {
			Token value;
		} u_number_lit;

		struct {
			Token value;
		} u_string_lit;

		// Statements

		struct {
			CX_AST_Node *expr;
		} u_return_stmt;

		struct {
			CX_AST_Node *data;
			size_t len;
			size_t _allocated;
		} u_compound_stmt;

		// Declarations

		struct {
			CX_AST_Node *data_type;
			CX_AST_Node *name;
			// TODO: types
			// TODO: names
			CX_AST_Node *body;
		} u_function_decl;

	}; // end union
};

DECLARE_DARRAY(CX_AST_Node)

void CX_AST_Node_root(CX_AST_Node *root) {
	root->type = CX_AST_NODE_TYPE_ROOT;
	root->parent = NULL;
	DARRAY_INIT(CX_AST_Node)((DARRAY(CX_AST_Node)*) &root->u_root, 1);
}

// Identifier constructors

void CX_AST_Node_type_id(CX_AST_Node *parent, CX_AST_Node *new) {
	new->type = CX_AST_NODE_TYPE_TYPE_ID;
	new->parent = parent;
	new->u_type_id.value = (Token) { 0 };
}

void CX_AST_Node_name_id(CX_AST_Node *parent, CX_AST_Node *new) {
	new->type = CX_AST_NODE_TYPE_NAME_ID;
	new->parent = parent;
	new->u_name_id.value = (Token) { 0 };
}

// Literal constructors

void CX_AST_Node_number_lit(CX_AST_Node *parent, CX_AST_Node *new) {
	new->type = CX_AST_NODE_TYPE_NUMBER_LIT;
	new->parent = parent;
	new->u_number_lit.value = (Token) { 0 };
}

void CX_AST_Node_string_lit(CX_AST_Node *parent, CX_AST_Node *new) {
	new->type = CX_AST_NODE_TYPE_STRING_LIT;
	new->parent = parent;
	new->u_number_lit.value = (Token) { 0 };
}

// Statement constructors

void CX_AST_Node_return_stmt(CX_AST_Node *parent, CX_AST_Node *new) {
	new->type = CX_AST_NODE_TYPE_RETURN_STMT;
	new->parent = parent;
	new->u_return_stmt.expr = (CX_AST_Node*) malloc(sizeof(CX_AST_Node));
}

void CX_AST_Node_compound_stmt(CX_AST_Node *parent, CX_AST_Node *new) {
	new->type = CX_AST_NODE_TYPE_COMPOUND_STMT;
	new->parent = parent;
	DARRAY_INIT(CX_AST_Node)((DARRAY(CX_AST_Node)*) &new->u_compound_stmt, 1);
}

// Declaration constructors

void CX_AST_Node_function_decl(CX_AST_Node *parent, CX_AST_Node *new) {
	new->type = CX_AST_NODE_TYPE_FUNCTION_DECL;
	new->parent = parent;
	new->u_function_decl.data_type = (CX_AST_Node*) malloc(sizeof(CX_AST_Node));
	new->u_function_decl.name = (CX_AST_Node*) malloc(sizeof(CX_AST_Node));
	new->u_function_decl.body = (CX_AST_Node*) malloc(sizeof(CX_AST_Node));
}

// end CX_AST_Node constructors

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
		case CX_AST_NODE_TYPE_TYPE_ID:
			// no need to free
			break;
		case CX_AST_NODE_TYPE_NAME_ID:
			// no need to free
			break;
		case CX_AST_NODE_TYPE_NUMBER_LIT:
			// no need to free
			break;
		case CX_AST_NODE_TYPE_STRING_LIT:
			// no need to free
			break;
		case CX_AST_NODE_TYPE_RETURN_STMT:
			CX_AST_Node_free(*node.u_return_stmt.expr);
			break;
		case CX_AST_NODE_TYPE_COMPOUND_STMT:
			for(size_t i = 0; i < node.u_compound_stmt.len; ++i)
				CX_AST_Node_free(node.u_compound_stmt.data[i]);
			DARRAY_FREE(CX_AST_Node)((DARRAY(CX_AST_Node)*) &node.u_compound_stmt);
			break;
		case CX_AST_NODE_TYPE_FUNCTION_DECL:
			CX_AST_Node_free(*node.u_function_decl.data_type);
			CX_AST_Node_free(*node.u_function_decl.name);
			CX_AST_Node_free(*node.u_function_decl.body);
			break;
	}
}

void CX_AST_Node_print_json(CX_AST_Node *node, FILE *sink) {
	fprintf(sink, "{");
	if(node) switch(node->type) {
		case CX_AST_NODE_TYPE_NULL:
			fprintf(sink, "null");
			break;
		case CX_AST_NODE_TYPE_ROOT:
			fprintf(sink, "\"u_root\":{\"children\":[");
			if(node->u_root.len > 0)
				CX_AST_Node_print_json(&node->u_root.data[0], sink);
			for(size_t i = 1; i < node->u_root.len; ++i) {
				fprintf(sink, ",");
				CX_AST_Node_print_json(&node->u_root.data[i], sink);
			}
			fprintf(sink, "]}");
			break;
		case CX_AST_NODE_TYPE_TYPE_ID:
			fprintf(sink, "\"" PRIsv "\"", PRIsv_arg(node->u_type_id.value.value_sv));
			break;
		case CX_AST_NODE_TYPE_NAME_ID:
			fprintf(sink, "\"" PRIsv "\"", PRIsv_arg(node->u_name_id.value.value_sv));
			break;
		case CX_AST_NODE_TYPE_NUMBER_LIT:
			fprintf(sink, "\"u_number_lit\":%d", node->u_number_lit.value.value_int);
			break;
		case CX_AST_NODE_TYPE_STRING_LIT:
			fprintf(sink, "\"u_string_lit\":\"" PRIsv "\"", PRIsv_arg(node->u_number_lit.value.value_sv));
			break;
		case CX_AST_NODE_TYPE_RETURN_STMT:
			fprintf(sink, "\"u_return_stmt\":");
			CX_AST_Node_print_json(node->u_return_stmt.expr, sink);
			break;
		case CX_AST_NODE_TYPE_COMPOUND_STMT:
			fprintf(sink, "\"u_compound_stmt\":{\"children\":[");
			if(node->u_compound_stmt.len > 0)
				CX_AST_Node_print_json(&node->u_compound_stmt.data[0], sink);
			for(size_t i = 1; i < node->u_compound_stmt.len; ++i) {
				fprintf(sink, ",");
				CX_AST_Node_print_json(&node->u_compound_stmt.data[i], sink);
			}
			fprintf(sink, "]}");
			break;
		case CX_AST_NODE_TYPE_FUNCTION_DECL:
			fprintf(sink, "\"u_function_decl\":{\"data_type\":");
			CX_AST_Node_print_json(node->u_function_decl.data_type, sink);
			fprintf(sink, ",\"name\":");
			CX_AST_Node_print_json(node->u_function_decl.name, sink);
			fprintf(sink, ",\"body\":");
			CX_AST_Node_print_json(node->u_function_decl.body, sink);
			fprintf(sink, "}");
			break;
	}
	fprintf(sink, "}");
}

typedef struct {
	DARRAY(Token) *tokens;
	size_t cur;
	bool eof, ok_so_far;
} Parser;

Token Parser_peek_token(Parser *parser) {
	if(parser->cur < parser->tokens->len) {
		return parser->tokens->data[parser->cur];
	} else {
		return (Token) { 0 };
	}
}

Token Parser_next_token(Parser *parser) {
	Token t = (parser->cur < parser->tokens->len) ? parser->tokens->data[parser->cur++] : (Token) { 0 };
	if(t.type == TOKEN_EOF) parser->eof = true;
	return t;
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

// Parser_next literals

bool Parser_next_number_lit(Parser *parser, CX_AST_Node *parent, CX_AST_Node *out) {
	size_t saved_cur = parser->cur;

	CX_AST_Node_number_lit(parent, out);

	Token number = Parser_next_token(parser);
	if(number.type != TOKEN_NUMBER) goto Parser_next_number_lit_cleanup;
	out->u_number_lit.value = number;	

	out->type = CX_AST_NODE_TYPE_NUMBER_LIT;
	return true;

Parser_next_number_lit_cleanup:
	parser->cur = saved_cur;
	CX_AST_Node_free(*out);
	return false;
}

bool Parser_next_string_lit(Parser *parser, CX_AST_Node *parent, CX_AST_Node *out) {
	size_t saved_cur = parser->cur;

	CX_AST_Node_string_lit(parent, out);

	Token string = Parser_next_token(parser);
	if(string.type != TOKEN_STRING) goto Parser_next_string_lit_cleanup;
	out->u_string_lit.value = string;

	out->type = CX_AST_NODE_TYPE_STRING_LIT;
	return true;

Parser_next_string_lit_cleanup:
	parser->cur = saved_cur;
	CX_AST_Node_free(*out);
	return false;
}

// Parser_next identifiers

bool Parser_next_type_id(Parser *parser, CX_AST_Node *parent, CX_AST_Node *out) {
	size_t saved_cur = parser->cur;

	CX_AST_Node_type_id(parent, out);

	Token data_type = Parser_next_token(parser);
	if(data_type.type != TOKEN_NAME) goto Parser_next_type_id_cleanup;
	out->u_type_id.value = data_type;	

	out->type = CX_AST_NODE_TYPE_TYPE_ID;
	return true;

Parser_next_type_id_cleanup:
	parser->cur = saved_cur;
	CX_AST_Node_free(*out);
	return false;
}

bool Parser_next_name_id(Parser *parser, CX_AST_Node *parent, CX_AST_Node *out) {
	size_t saved_cur = parser->cur;

	CX_AST_Node_name_id(parent, out);

	Token name = Parser_next_token(parser);
	if(name.type != TOKEN_NAME) goto Parser_next_name_id_cleanup;
	out->u_name_id.value = name;	

	out->type = CX_AST_NODE_TYPE_NAME_ID;
	return true;

Parser_next_name_id_cleanup:
	parser->cur = saved_cur;
	CX_AST_Node_free(*out);
	return false;
}

// Parser_next statements

bool Parser_next_return_stmt(Parser *parser, CX_AST_Node *parent, CX_AST_Node *out) {
	size_t saved_cur = parser->cur;

	CX_AST_Node_return_stmt(parent, out);

	Token return_keyword = Parser_next_token(parser);
	if(return_keyword.type != TOKEN_NAME) goto Parser_next_return_stmt_cleanup;
	if(!sveq(return_keyword.value_sv, sv_from_cstr("return"))) goto Parser_next_return_stmt_cleanup;

	if(!Parser_next_number_lit(parser, out, out->u_return_stmt.expr))  {
		parser->ok_so_far = false;
		loc_error(parser->tokens->data[parser->cur].location, "invalid expression\n");
		loc_error_cite(parser->tokens->data[parser->cur].location);
		goto Parser_next_return_stmt_cleanup;
	}

	Token semicolon = Parser_next_token(parser);
	if(semicolon.type != TOKEN_SEMICOLON) goto Parser_next_return_stmt_cleanup;

	out->type = CX_AST_NODE_TYPE_RETURN_STMT;
	return true;

Parser_next_return_stmt_cleanup:
	parser->cur = saved_cur;
	CX_AST_Node_free(*out);
	return false;
}

bool Parser_next_compound_stmt(Parser*, CX_AST_Node*, CX_AST_Node*); // Forward declaration

bool Parser_next_stmt(Parser *parser, CX_AST_Node *parent, CX_AST_Node *stmt) {
	if(Parser_next_return_stmt(parser, parent, stmt)) return true;
	if(Parser_next_compound_stmt(parser, parent, stmt)) return true;

	return false;
}

bool Parser_next_compound_stmt(Parser *parser, CX_AST_Node *parent, CX_AST_Node *out) {
	size_t saved_cur = parser->cur;

	CX_AST_Node_compound_stmt(parent, out);

	Token oc = Parser_next_token(parser);
	if(oc.type != TOKEN_OPEN_CURLY) goto Parser_next_compound_stmt_cleanup;

	CX_AST_Node stmt;

Parser_next_compound_stmt_more:

	if(Parser_next_stmt(parser, out, &stmt)) {
		DARRAY_PUSH(CX_AST_Node)((DARRAY(CX_AST_Node)*) &out->u_compound_stmt, stmt);
	} else {
		goto Parser_next_compound_stmt_cleanup;
	}

	Token peeked_token = Parser_peek_token(parser);
	if(peeked_token.type != TOKEN_CLOSE_CURLY) goto Parser_next_compound_stmt_more;

	Token cc = Parser_next_token(parser);
	if(cc.type != TOKEN_CLOSE_CURLY) goto Parser_next_compound_stmt_cleanup;

	out->type = CX_AST_NODE_TYPE_COMPOUND_STMT;
	return true;

Parser_next_compound_stmt_cleanup:
	parser->cur = saved_cur;
	CX_AST_Node_free(*out);
	return false;
}

// Parser_next declarations

bool Parser_next_function_decl(Parser *parser, CX_AST_Node *parent, CX_AST_Node *out) {
	size_t saved_cur = parser->cur;

	CX_AST_Node_function_decl(parent, out);

	if(!Parser_next_type_id(parser, out, out->u_function_decl.data_type)) goto Parser_next_function_decl_cleanup;

	if(!Parser_next_name_id(parser, out, out->u_function_decl.name)) goto Parser_next_function_decl_cleanup;

	Token op = Parser_next_token(parser);
	if(op.type != TOKEN_OPEN_PARENTHESIS) goto Parser_next_function_decl_cleanup;

	// TODO: parse parameters

	Token cp = Parser_next_token(parser);
	if(cp.type != TOKEN_CLOSE_PARENTHESIS) goto Parser_next_function_decl_cleanup;

	if(!Parser_next_compound_stmt(parser, out, out->u_function_decl.body)) goto Parser_next_function_decl_cleanup;

	out->type = CX_AST_NODE_TYPE_FUNCTION_DECL;
	return true;

Parser_next_function_decl_cleanup:
	parser->cur = saved_cur;
	CX_AST_Node_free(*out);
	return false;
}

// end Parser_next declarations

void Parser_next_root_child(Parser *parser, CX_AST_Node *parent, CX_AST_Node *out) {
	CX_AST_Node zero = { 0 };
	*out = zero;

	if(Parser_next_function_decl(parser, parent, out)) return;

	*out = zero;
}

// end Parser_nexts

// Semantic analysis

typedef struct {
	HashMap *data_type_translations;
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
		case CX_AST_NODE_TYPE_TYPE_ID:
			if(!HashMap_at(semantic_structure->data_type_translations, ast->u_type_id.value.value_sv))
				loc_error(ast->u_type_id.value.location, " unknown data type: " PRIsv "\n", PRIsv_arg(ast->u_type_id.value.value_sv));
			break;
		case CX_AST_NODE_TYPE_NAME_ID:
			// TODO: check validness and redeclaration
			break;
		case CX_AST_NODE_TYPE_NUMBER_LIT:
			// TODO
			break;
		case CX_AST_NODE_TYPE_STRING_LIT:
			// TODO
			break;
		case CX_AST_NODE_TYPE_RETURN_STMT:
			// TODO
			break;
		case CX_AST_NODE_TYPE_COMPOUND_STMT:
			for(size_t i = 0; i < ast->u_compound_stmt.len; ++i)
				analyse_semantics(&ast->u_compound_stmt.data[i], semantic_structure);
			break;
		case CX_AST_NODE_TYPE_FUNCTION_DECL:
			analyse_semantics(ast->u_function_decl.body, semantic_structure);
			break;
	}
}

// Code generation

typedef struct {
	HashMap *data_type_translations;
} CodeGenerator;

void __IMPL__generate_code(CodeGenerator *code_gen, CX_AST_Node *ast, FILE *sink, int indent_len) {
	const char indent = '\t';

	if(ast) switch(ast->type) {
		case CX_AST_NODE_TYPE_NULL:
			assert(false && "unreachable");
			break;
		case CX_AST_NODE_TYPE_ROOT:
			for(size_t i = 0; i < ast->u_root.len; ++i) {
				__IMPL__generate_code(code_gen, &ast->u_root.data[i], sink, indent_len);
				fprintf(sink, "\n");
			}
			break;
		case CX_AST_NODE_TYPE_TYPE_ID:
			fprintf(sink, PRIsv, PRIsv_arg(ast->u_type_id.value.value_sv));
			break;
		case CX_AST_NODE_TYPE_NAME_ID:
			fprintf(sink, PRIsv, PRIsv_arg(ast->u_name_id.value.value_sv));
			break;
		case CX_AST_NODE_TYPE_NUMBER_LIT:
			fprintf(sink, "%d", ast->u_number_lit.value.value_int);
			break;
		case CX_AST_NODE_TYPE_STRING_LIT:
			fprintf(sink, "\"" PRIsv "\"", PRIsv_arg(ast->u_string_lit.value.value_sv));
			break;
		case CX_AST_NODE_TYPE_RETURN_STMT:
			if(indent_len) fprintf(sink, "%*c", indent_len, indent);
			fprintf(sink, "return ");
			__IMPL__generate_code(code_gen, ast->u_return_stmt.expr, sink, indent_len);
			fprintf(sink, ";\n");
			break;
		case CX_AST_NODE_TYPE_COMPOUND_STMT:
			if(indent_len) fprintf(sink, "%*c", indent_len, indent);
			fprintf(sink, "{\n");
			for(size_t i = 0; i < ast->u_compound_stmt.len; ++i) {
				__IMPL__generate_code(code_gen, &ast->u_compound_stmt.data[i], sink, indent_len + 1);
			}
			if(indent_len) fprintf(sink, "%*c", indent_len, indent);
			fprintf(sink, "}\n");
			break;
		case CX_AST_NODE_TYPE_FUNCTION_DECL:
			if(indent_len) fprintf(sink, "%*c", indent_len, indent);
			fprintf(sink, PRIsv " " PRIsv "()\n", PRIsv_arg(ast->u_function_decl.data_type->u_type_id.value.value_sv), PRIsv_arg(ast->u_function_decl.name->u_name_id.value.value_sv));
			__IMPL__generate_code(code_gen, ast->u_function_decl.body, sink, indent_len);
			break;
	}
}

void generate_code(CodeGenerator *code_gen, CX_AST_Node *ast, FILE *sink) {
	__IMPL__generate_code(code_gen, ast, sink, 0);
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
		DARRAY_INIT(char)(array, ftell(fp) + 2);
		fseek(fp, 0, SEEK_SET);
		array->len = fread(array->data, 1, array->_allocated, fp);
		if(errno) {
			DARRAY_FREE(char)(array);
			fclose(fp);
			panic("error reading file %s: %s\n", filename, strerror(errno));
		}
		array->data[array->len] = '\n';
		array->data[array->len + 1] = 0;
	} else {
		panic("could not open file: %s\n", filename);
	}

	fclose(fp);
}

char *program_name = NULL;
char *source_filename = NULL;
bool dump_ast = false;

DARRAY(char) source_code;
DARRAY(Token) tokens;
CX_AST_Node root;
HashMap data_type_translations;

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
		DEBUG_TRACE("Lexical analysis\n");

		alloc_file_content(&source_code, source_filename, "r");
		
		Lexer lexer = {
			.file_path = source_filename,
			.source = source_code.data,
			.source_len = source_code.len,
			.eof = false
		};

		DARRAY_INIT(Token)(&tokens, 1);

		while(Lexer_is_not_empty(&lexer)) {
			Token token = Lexer_next_token(&lexer);
			DARRAY_PUSH(Token)(&tokens, token);
		}

		Token eof = (Token) { 0 };
		eof.type = TOKEN_EOF;

		DARRAY_PUSH(Token)(&tokens, eof);

	}

	{
		DEBUG_TRACE("Parsing\n");

		Parser parser = {
			.tokens = &tokens,
			.cur = 0,
			.eof = false,
			.ok_so_far = true
		};

		CX_AST_Node_root(&root);

		while(!parser.eof) {
			CX_AST_Node node;
			Parser_next_root_child(&parser, &root, &node);
			if(node.type) {
				DARRAY_PUSH(CX_AST_Node)((DARRAY(CX_AST_Node)*) &root.u_root, node);
			}
		};

		if(!parser.ok_so_far) {
			info("Parsing failed, skipping next steps\n");
			goto main_cleanup;
		}

		if(dump_ast) {
			CX_AST_Node_print_json(&root, stdout);
			putc('\n', stdout);
			goto main_cleanup;
		}

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
			.data_type_translations = &data_type_translations
		};

		analyse_semantics(&root, &semantic_structure);
	}

	{
		DEBUG_TRACE("Code generation\n");

		CodeGenerator code_gen = {
			.data_type_translations = &data_type_translations
		};

		generate_code(&code_gen, &root, stdout);
	}

main_cleanup:

	HashMap_free(&data_type_translations);
	CX_AST_Node_free(root);
	DARRAY_FREE(Token)(&tokens);
	DARRAY_FREE(char)(&source_code);

	return 0;
}