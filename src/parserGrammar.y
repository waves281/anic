/* core tokens */
%token ID
%token INUM
%token FNUM
%token PERIOD
%token DPERIOD
%token LBRACKET
%token RBRACKET
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
%token LRARROW
%token SLASH
%token SSLASH
%token DSLASH
%token DSSLASH
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
	| AT SuffixedIdentifier
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
SwitchTerm : DQUESTION LCURLY LabeledPipes RCURLY
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
	| Swap
	;
StaticTerm : TypedStaticTerm
	| Access
	;
TypedStaticTerm : Node
	| LBRACKET Exp RBRACKET
	;
Exp : Primary
	| Exp DOR Exp
	| Exp DAND Exp
	| Exp OR Exp
	| Exp XOR Exp
	| Exp AND Exp
	| Exp DEQUALS Exp
	| Exp NEQUALS Exp
	| Exp LT Exp
	| Exp GT Exp
	| Exp LE Exp
	| Exp GE Exp
	| Exp LS Exp
	| Exp RS Exp
	| Exp TIMES Exp
	| Exp DIVIDE Exp
	| Exp MOD Exp
	| Exp PLUS Exp
	| Exp MINUS Exp
	;
Primary : SuffixedIdentifier
	| SLASH SuffixedIdentifier
	| PrimLiteral
	| PrefixOrMultiOp Primary
	| LBRACKET Exp RBRACKET
	;
Node : SuffixedIdentifier
	| NodeInstantiation
	| TypedNodeLiteral
	| PrimOpNode
	| PrimLiteral
	| Block
	;
SuffixedIdentifier : Identifier
	| Identifier PERIOD SuffixedIdentifier
	| Identifier ArraySuffix
	| Identifier ArraySuffix PERIOD SuffixedIdentifier
	;
Identifier : ID
	| DPERIOD
	;
NodeInstantiation : DLSQUARE NonEmptyTypeList DRSQUARE
	| DLSQUARE NonEmptyTypeList DRSQUARE LARROW StaticTerm
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
	| DPLUS
	| DMINUS
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
Type : Identifier TypeSuffix
	| NodeType TypeSuffix
	;
TypeSuffix : 
	| SLASH
	| ComplexTypeSuffix
	;
ComplexTypeSuffix : DSLASH
	| DSLASH ComplexTypeSuffix
	| LSQUARE RSQUARE
	| LSQUARE RSQUARE ComplexTypeSuffix
	;
NodeType : DLSQUARE TypeList RetList DRSQUARE
	;
TypeList : 
	| NonEmptyTypeList
	;
NonEmptyTypeList : Type
	| Type COMMA NonEmptyTypeList
	;
Block : LCURLY Pipes RCURLY
	;
Access : SLASH Node
	| SSLASH Node
	| DSLASH Node	
	| DSSLASH Node
	;
Compound : COMMA StaticTerm
	;
Link : DCOLON StaticTerm
	;
Send : RARROW Node
	| DRARROW
	;
Swap : LRARROW Node
	;
%%
