/* core tokens */
%token ID
%token INUM
%token FNUM
%token PERIOD
%token DPERIOD
%token LBRACKET
%token RBRACKET
%token LSLASH
%token DLSLASH
%token COMMA
%token EQUALS
%token SEMICOLON
%token QUESTION
%token DQUESTION
%token COLON
%token DCOLON
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
LabeledPipes : StaticTerm COLON SimpleTerm
	| StaticTerm COLON SimpleTerm LabeledPipes
	| COLON SimpleTerm
	;
Pipe : Declaration
	| NonEmptyTerms
	;
Declaration : ID EQUALS TypedStaticTerm
	| ID ERARROW NonEmptyTerms
	| AT Identifier
	;
NonEmptyTerms : Term Terms
	;
Terms : 
	| Term Terms
	;
Term : OpenTerm
	| ClosedTerm
	;
OpenTerm : SimpleCondTerm
	| OpenCondTerm
	;
ClosedTerm : SimpleTerm
	| ClosedCondTerm
	;
SimpleCondTerm : QUESTION Term
	;
SwitchTerm : DQUESTION SwitchBlock
	;
OpenCondTerm : QUESTION ClosedTerm COLON OpenTerm
	;
ClosedCondTerm : QUESTION ClosedTerm COLON ClosedTerm
	;
SimpleTerm : DynamicTerm
	| SwitchTerm
	;
DynamicTerm : StaticTerm
	| Compound
	| Link
	| Send
	;
StaticTerm : TypedStaticTerm
	| Delatch
	| Block
	;
TypedStaticTerm : Node
	| LBRACKET Exp RBRACKET
	;
Exp : ExpLeft
	| ExpLeft ExpRight
	;
NonCastExp : NonCastExpLeft ExpRight
	;
ExpLeft : Identifier
	| SLASH Identifier
	| NonCastExpLeft
	;
NonCastExpLeft : PrimLiteral
	| PrefixOrMultiOp ExpLeft
	| LBRACKET NonCastExp RBRACKET
	| LBRACKET Identifier RBRACKET
	| LBRACKET SLASH Identifier RBRACKET
	| LBRACKET Identifier ExpRight RBRACKET
	| LBRACKET SLASH Identifier ExpRight RBRACKET
	| LBRACKET Identifier RBRACKET ExpLeft
	;
ExpRight : InfixOrMultiOp Exp
	;
Node : Identifier
	| NodeInstantiation
	| TypedNodeLiteral
	| PrimOpNode
	| PrimLiteral
	;
Identifier : ID
	| ID PERIOD Identifier
	| ID ArraySuffix
	| ID ArraySuffix PERIOD Identifier
	| DPERIOD
	| DPERIOD PERIOD Identifier
	| DPERIOD ArraySuffix
	| DPERIOD ArraySuffix PERIOD Identifier
	;
NodeInstantiation : DLSQUARE NonEmptyTypeList DRSQUARE
	| DLSQUARE NonEmptyTypeList DRSQUARE LARROW StaticTerm
	;
LatchTypeSuffix : SLASH
	;
StreamTypeSuffix : DSLASH
	| DSLASH StreamTypeSuffix
	;
ArrayAccess : LSQUARE Exp RSQUARE
	| LSQUARE Exp DCOLON Exp RSQUARE
	;
ArraySuffix : ArrayAccess
	| ArrayAccess ArraySuffix
	;
PrimOpNode : PrefixOp
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
TypedNodeLiteral : NodeHeader Block
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
	| Node LatchTypeSuffix
	| Node StreamTypeSuffix
	;
NonEmptyTypeList : Type
	| Type COMMA NonEmptyTypeList
	;
Block : LCURLY Pipes RCURLY
	;
SwitchBlock : LCURLY LabeledPipes RCURLY
	;
Delatch : SLASH Node
	| DSLASH Node
	;
Compound : COMMA StaticTerm
	;
Link : DCOLON StaticTerm
	;
Send : RARROW Node
	| DRARROW
	;
%%
