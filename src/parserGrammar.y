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
%token ERARROW
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
%left DTIMES
%left NOT COMPLEMENT
%left LBRACKET RBRACKET

%%
Program : Pipes
	;
Pipes : 
	| Pipe
	| Pipe SEMICOLON Pipes
	;
Pipe : Declaration
	| NonEmptyTerms
	;
Declaration : SimpleDecl
	| ThroughDecl
	;
SimpleDecl : ID EQUALS NonEmptyTerms
	;
ThroughDecl : ID ERARROW NonEmptyTerms
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
	| BracketedExp
	| Access
	| Compound
	| Send
	| Block
	;
SimpleCond : QUESTION Term
	;
OpenCond : QUESTION ClosedTerm COLON OpenTerm
	;
ClosedCond : QUESTION ClosedTerm COLON ClosedTerm
	;
BracketedExp : LBRACKET Exp RBRACKET
	;
Exp : ExpLeft ExpRight
	;
NonCastExp : NonCastExpLeft ExpRight
	;
ExpLeft : QualifiedIdentifier
	| NonCastExpLeft
	;
NonCastExpLeft : PrimLiteral
	| PrefixOrMultiOp ExpLeft
	| LBRACKET NonCastExp RBRACKET
	| LBRACKET QualifiedIdentifier RBRACKET
	| LBRACKET QualifiedIdentifier RBRACKET ExpLeft
	;
ExpRight : 
	| InfixOrMultiOp Exp
	;
Node : QualifiedIdentifier
	| NodeLiteral
	| PrimNode
	| PrimLiteral
	;
QualifiedIdentifier : ID
	| ID PERIOD QualifiedIdentifier
	;
PrimNode : PrefixOp
	| InfixOp
	| MultiOp
	;
PrefixOp : NOT
	| COMPLEMENT
	;
InfixOp : DOR
	| DAND
	| OR
	| XOR
	| AND
	| DEQUALS
	| NEQUALS
	| LT
	| GT
	| LE
	| GE
	| LS
	| RS
	| TIMES
	| DIVIDE
	| MOD
	| DTIMES
	;
MultiOp : PLUS
	| MINUS
	;
PrefixOrMultiOp : PrefixOp
	| MultiOp
	;
InfixOrMultiOp : InfixOp
	| MultiOp
	;
PrimLiteral : INUM
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
Compound : COMMA SimpleTerm
	;
Send : RARROW Node
	;
Block : LCURLY Pipes RCURLY
	;
%%
