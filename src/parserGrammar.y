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
%token RARROW
%token DRARROW
%token SLASH
%token DSLASH

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
Program : Pipes
	;
Pipes :
	| Pipe SEMICOLON Pipes
	;
Pipe : Declaration
	| NonEmptyTerms
	;
Declaration : ID EQUALS	NonEmptyTerms
	;
NonEmptyTerms : Term Terms
	;
Terms : 
	| Term Terms
	;
Term : OpenTerm
	| ClosedTerm
	;
OpenTerm : SimpleCond
	| OpenCond
	;
ClosedTerm : SimpleTerm
	| ClosedCond
	;
SimpleTerm : Node
	| Literal
	| Access
	| Send
	| Block
	;
SimpleCond : QUESTION Term
	;
OpenCond : QUESTION ClosedTerm COLON OpenTerm
	;
ClosedCond : QUESTION ClosedTerm COLON ClosedTerm
	;
Node : QualifiedIdentifier
	| NodeLiteral
	;
QualifiedIdentifier : ID
	| ID PERIOD QualifiedIdentifier
	;
Literal : INUM
	| FNUM
	| CQUOTE
	| SQUOTE
	;
NodeLiteral : NodeHeader Block
	;
NodeHeader : DLSQUARE DeclList RetList DRSQUARE
	;
DeclList : 
	| NonEmptyDeclList
	;
NonEmptyDeclList: Decl
	| Decl COMMA NonEmptyDeclList
	;
RetList : 
	| DRARROW NonEmptyTypeList
	;
Decl : Type ID
	;
Type : QualifiedIdentifier
	;
NonEmptyTypeList : Type
	| Type COMMA NonEmptyTypeList
	;
Access : PopAccess
	| StreamAccess
	;
PopAccess : SLASH Node
	;
StreamAccess : DSLASH Node
	;
Send : RARROW Node
	;
Block : LCURLY Pipes RCURLY
	;
%%
