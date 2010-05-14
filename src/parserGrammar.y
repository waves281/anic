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
%token RARROW
%token DRARROW
%token ERARROW
%token LARROW
%token LRARROW
%token SLASH
%token SSLASH
%token DSLASH
%token DSSLASH
%token CQUOTE
%token SQUOTE
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
	| Pipe Pipes
	;
LabeledPipes : StaticTerm COLON SimpleTerm
	| StaticTerm COLON SimpleTerm LabeledPipes
	| COLON SimpleTerm
	;
Pipe : Declaration
	| NonEmptyTerms SEMICOLON
	;
Declaration : ID EQUALS TypedStaticTerm
	| ID EQUALS TypedStaticTerm SEMICOLON
	| ID ERARROW NonEmptyTerms SEMICOLON
	| AT SuffixedIdentifier
	| AT SuffixedIdentifier SEMICOLON
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
	| BracketedExp
	;
BracketedExp : LBRACKET Exp RBRACKET
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
	| BracketedExp
	;
Node : SuffixedIdentifier
	| NodeInstantiation
	| Filter
	| Object
	| PrimOpNode
	| PrimLiteral
	;
SuffixedIdentifier : ID
	| ID PERIOD SuffixedIdentifier
	| ID PERIOD ArrayAccess
	| ID PERIOD ArrayAccess PERIOD SuffixedIdentifier
	| DPERIOD
	| DPERIOD PERIOD SuffixedIdentifier
	| DPERIOD PERIOD ArrayAccess
	| DPERIOD PERIOD ArrayAccess PERIOD SuffixedIdentifier
	;
NonArraySuffixedIdentifier : ID
	| ID PERIOD NonArraySuffixedIdentifier
	| DPERIOD
	| DPERIOD PERIOD NonArraySuffixedIdentifier
	;
NodeInstantiation : LSQUARE TypeList RSQUARE
	| LSQUARE TypeList RSQUARE LARROW StaticTerm
	;
ArrayAccess : LSQUARE Exp RSQUARE
	| LSQUARE Exp COLON Exp RSQUARE
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
Filter : Block
	| FilterHeader Block
	;
FilterHeader : LSQUARE RSQUARE
	| LSQUARE ParamList RSQUARE
	| LSQUARE ParamList RetList RSQUARE
	| LSQUARE RetList RSQUARE
	;
NonRetFilterHeader : LSQUARE RSQUARE
	| LSQUARE ParamList RSQUARE
	;
ParamList : Param
	| Param COMMA ParamList
	;
RetList : DRARROW TypeList
	;
Param : Type ID
	;
Type : NonArraySuffixedIdentifier TypeSuffix
	| FilterType TypeSuffix
	;
TypeSuffix :
	| SLASH
	| StreamTypeSuffix
	| ArrayTypeSuffix
	;
StreamTypeSuffix : DSLASH
	| DSLASH StreamTypeSuffix
	;
ArrayTypeSuffix : LSQUARE RSQUARE
	| LSQUARE RSQUARE ArrayTypeSuffix
	;
ComplexTypeSuffixNoArray : DSLASH
	| DSLASH ComplexTypeSuffixNoArray
	;
FilterType : LSQUARE RSQUARE
	| LSQUARE TypeList RSQUARE
	| LSQUARE TypeList RetList RSQUARE
	| LSQUARE RetList RSQUARE
	;
TypeList : Type
	| Type COMMA TypeList
	;
Block : LCURLY Pipes RCURLY
	;
Object : LCURLY Constructors Pipes RCURLY
	;
Constructors : Constructor
	| Constructor Constructors
	;
Constructor : EQUALS LSQUARE RSQUARE SEMICOLON
	| EQUALS NonRetFilterHeader Block
	| EQUALS NonRetFilterHeader Block SEMICOLON
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
