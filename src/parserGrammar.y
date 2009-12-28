/* core tokens */
%token ID
%token UNDERSCORE
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
%token LARROW
%token SLASH
%token DSLASH
%token AT
%token DOLLAR

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
%token DPLUS
%token MINUS
%token DMINUS
%token TIMES
%token DIVIDE
%token MOD
%token DTIMES
%token NOT
%token COMPLEMENT

/* arithmetic precedence rules */
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
Declaration : ID EQUALS StaticTerm
	| ID ERARROW NonEmptyTerms
	| AT QualifiedIdentifier
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
SimpleTerm : StaticTerm
	| Compound
	| Send
	;
StaticTerm : Node
	| BracketedExp
	| Access
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
	| QualifiedIdentifier ArraySuffix
	| NonCastExpLeft
	;
NonCastExpLeft : PrimLiteral
	| PrefixOrMultiOp ExpLeft
	| LBRACKET NonCastExp RBRACKET
	| LBRACKET QualifiedIdentifier RBRACKET
	| LBRACKET QualifiedIdentifier ArraySuffix RBRACKET
	| LBRACKET QualifiedIdentifier RBRACKET ExpLeft
	;
ExpRight : 
	| InfixOrMultiOp Exp
	;
Node : QualifiedIdentifier
	| NodeInstantiation
	| NodeLiteral
	| PrimNode
	| PrimLiteral
	;
QualifiedIdentifier : UNDERSCORE
	| UNDERSCORE PERIOD QualifiedIdentifier
	| ID
	| ID PERIOD QualifiedIdentifier
	;
NodeInstantiation : DLSQUARE NonEmptyTypeList DRSQUARE
	| DLSQUARE NonEmptyTypeList DRSQUARE LARROW StaticTerm
	;
LatchSuffix : SLASH
	;
StreamSuffix : DSLASH
	| DSLASH StreamSuffix
	| LSQUARE Exp RSQUARE
	| LSQUARE Exp RSQUARE StreamSuffix
	;
ArraySuffix : LSQUARE Exp RSQUARE
	| LSQUARE Exp RSQUARE ArraySuffix
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
	| DPLUS
	| DMINUS
	| TIMES
	| DIVIDE
	| MOD
	| DTIMES
	| DOLLAR
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
	| Block
	;
NodeHeader : DLSQUARE ParamList RetList DRSQUARE
	;
ParamList : 
	| NonEmptyParamList
	;
NonEmptyParamList: Param
	| Param COMMA NonEmptyParamList
	;
RetList : 
	| DRARROW NonEmptyTypeList
	;
Param : Type ID
	;
Type : Node
	| Node LatchSuffix
	| Node StreamSuffix
	;
NonEmptyTypeList : Type
	| Type COMMA NonEmptyTypeList
	;
Block : LCURLY Pipes RCURLY
	;
Access : SLASH Node
	| DSLASH Node
	;
Compound : COMMA StaticTerm
	;
Send : RARROW Node
	| DRARROW
	;
%%
