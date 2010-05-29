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
%token ASLASH
%token DSLASH
%token DSSLASH
%token DASLASH
%token CQUOTE
%token SQUOTE
%token AT
%token DAT

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
	| LastPipe
	| Pipe NonEmptyPipes
	;
NonEmptyPipes : LastPipe
	| Pipe NonEmptyPipes
	;
LabeledPipes : StaticTerm COLON SimpleTerm
	| StaticTerm COLON SimpleTerm LabeledPipes
	| COLON SimpleTerm
	;
Pipe : Declaration
	| NonEmptyTerms SEMICOLON
	;
LastPipe : Declaration
	| NonEmptyTerms SEMICOLON
	| LastDeclaration
	| NonEmptyTerms
	;
Declaration : ID EQUALS TypedStaticTerm
	| ID EQUALS TypedStaticTerm SEMICOLON
	| ID ERARROW NonEmptyTerms SEMICOLON
	| AT SuffixedIdentifier
	| AT SuffixedIdentifier SEMICOLON
	| AT LSQUARE SuffixedIdentifier RSQUARE
	| AT LSQUARE SuffixedIdentifier RSQUARE SEMICOLON
	| DAT SuffixedIdentifier
	| DAT SuffixedIdentifier SEMICOLON
	| DAT LSQUARE SuffixedIdentifier RSQUARE
	| DAT LSQUARE SuffixedIdentifier RSQUARE SEMICOLON
	;
LastDeclaration : ID ERARROW NonEmptyTerms
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
	| Return
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
	| SingleAccessor SuffixedIdentifier
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
SuffixedIdentifier : ID IdentifierSuffix
	| DPERIOD IdentifierSuffix
	;
IdentifierSuffix :
	| PERIOD ID IdentifierSuffix
	| PERIOD ArrayAccess IdentifierSuffix
	;
NonArraySuffixedIdentifier : ID NonArrayIdentifierSuffix
	| DPERIOD NonArrayIdentifierSuffix
	;
NonArrayIdentifierSuffix :
	| PERIOD ID NonArrayIdentifierSuffix
	;
NodeInstantiation : LSQUARE InstantiableType RSQUARE
	| LSQUARE InstantiableType RSQUARE LARROW StaticTerm
	;
InstantiableType : NonArraySuffixedIdentifier TypeSuffix
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
	| ObjectType TypeSuffix
	;
TypeSuffix :
	| SLASH
	| LSQUARE RSQUARE
	| DSLASH
	| ArrayTypeSuffix
	| PoolTypeSuffix
	;
ArrayTypeSuffix : LSQUARE Exp RSQUARE
	| LSQUARE Exp RSQUARE ArrayTypeSuffix
	;
PoolTypeSuffix : SLASH LSQUARE Exp RSQUARE
	| SLASH LSQUARE Exp RSQUARE ArrayTypeSuffix
	;
FilterType : LSQUARE RSQUARE
	| LSQUARE TypeList RSQUARE
	| LSQUARE TypeList RetList RSQUARE
	| LSQUARE RetList RSQUARE
	;
ObjectType : LCURLY RCURLY
	| LCURLY ObjectTypeList RCURLY
	;
ObjectTypeList : ConstructorType
	| ConstructorType COMMA ObjectTypeList
	| MemberList
	;
ConstructorType : EQUALS
	| EQUALS LSQUARE RSQUARE
	| EQUALS LSQUARE TypeList RSQUARE
	;
MemberList : MemberType
	| MemberType COMMA MemberList
	;
MemberType : ID EQUALS Type
	;
TypeList : Type
	| Type COMMA TypeList
	;
Block : LCURLY Pipes RCURLY
	;
Object : LCURLY Constructors NonEmptyPipes RCURLY
	| LCURLY OnlyConstructors RCURLY
	;
Constructors : Constructor
	| Constructor Constructors
	;
OnlyConstructors : Constructor
	| Constructor Constructors
	| LastConstructor
	;
Constructor : EQUALS SEMICOLON
	| EQUALS LSQUARE RSQUARE SEMICOLON
	| EQUALS NonRetFilterHeader Block
	| EQUALS NonRetFilterHeader Block SEMICOLON
	;
LastConstructor : EQUALS
	| EQUALS LSQUARE RSQUARE
	;
Access : SingleAccessor Node
	| MultiAccessor Node
	;
SingleAccessor : SLASH
	| SSLASH
	| ASLASH
	;
MultiAccessor : DSLASH
	| DSSLASH
	| DASLASH
	;
Compound : COMMA StaticTerm
	;
Link : DCOLON StaticTerm
	;
Send : RARROW Node
	;
Swap : LRARROW Node
	;
Return : DRARROW
	;
%%
