/* core tokens */
%token ID
%token INUM
%token FNUM
%token PERIOD
%token LBRACKET
%token RBRACKET
%token LSLASH
%token DLSLASH
%token COMMA
%token EQUALS
%token SEMICOLON
%token QUESTION
%token COLON
%token LCURLY
%token RCURLY
%token LSQUARE
%token RSQUARE
%token DLSQUARE
%token DRSQUARE
%token CQUOTE
%token SQUOTE

/* arithmetic tokens */
%token DOR
%token DAND
%token OR
%token XOR
%token AND
%token DEQUALS
%token NEQUALS
%token LT
%token GT
%token LE
%token GE
%token LS
%token RS
%token PLUS
%token MINUS
%token TIMES
%token DIVIDE
%token MOD
%token DTIMES
%token NOT
%token COMPLEMENT

/* precedence rules */
%left COMMA
%left ID
%left QUESTION
%left COLON
/* arithmetic precedence */
%left DOR
%left DAND
%left OR
%left XOR
%left AND
%left DEQUALS NEQUALS
%left LT GT LE GE
%left LS RS
%left PLUS MINUS
%left TIMES DIVIDE MOD
%left POWER
%left NOT COMPLEMENT
%left LBRACKET RBRACKET

%%
Program : Nodes
	;
Nodes : ID
	| INUM FNUM
	;
%%
