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
%token RFLAG
%token LFLAG
%token RARROW
%token LARROW
%token DRARROW
%token LRARROW
%token SLASH
%token DSLASH
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
%left RSQUARE
%left LSQUARE
%left LBRACKET RBRACKET LCURLY RCURLY

%%
Program : Pipes
	;
Pipes :
	| LastPipe
	| Pipe
	| Pipe NonEmptyPipes
	;
NonEmptyPipes : LastPipe
	| Pipe
	| Pipe NonEmptyPipes
	;
Pipe : Declaration
	| NonEmptyTerms SEMICOLON
	;
LastPipe : NonEmptyTerms
	;
Declaration : ID EQUALS TypedStaticTerm
	| ID EQUALS TypedStaticTerm SEMICOLON
	| ID EQUALS BlankInstantiation
	| ID EQUALS BlankInstantiation SEMICOLON
	| AT ImportIdentifier
	| AT ImportIdentifier SEMICOLON
	| AT LSQUARE ImportIdentifier RSQUARE
	| AT LSQUARE ImportIdentifier RSQUARE SEMICOLON
	;
ImportIdentifier : NonArrayedIdentifier
	| ArrayedIdentifier
	| OpenIdentifier
	;
NonEmptyTerms : Term Terms
	;
Terms :
	| Term Terms
	;
Term : ClosedTerm
	| OpenTerm
	| DynamicTerm
	;
ClosedTerm : SimpleTerm
	| ClosedCondTerm
	;
OpenTerm : SimpleCondTerm
	| OpenCondTerm
	;
SimpleCondTerm : QUESTION Term
	;
ClosedCondTerm : QUESTION ClosedTerm COLON ClosedTerm
	;
OpenCondTerm : QUESTION ClosedTerm COLON OpenTerm
	;
SimpleTerm : StaticTerm
	| SwitchTerm
	;
StaticTerm : TypedStaticTerm
	| SingleAccess
	| MultiAccess
	;
SingleStaticTerm : TypedStaticTerm
	| SingleAccess
	;
SwitchTerm : DQUESTION LCURLY LabeledTerms RCURLY
	;
LabeledTerms : LastLabeledTerm
	| LabeledTerm LabeledTerms
	;
LabeledTerm: TypedStaticTerm COLON SimpleTerm
	| TypedStaticTerm COLON SimpleTerm SEMICOLON
	;
LastLabeledTerm : COLON SimpleTerm
	| COLON SimpleTerm SEMICOLON
	;
DynamicTerm : Compound
	| Pack
	| Unpack
	| Link
	| Loopback
	| Send
	| Swap
	| Return
	;
TypedStaticTerm : Node
	| BracketedExp
	;
BracketedExp : LBRACKET RBRACKET
	| LBRACKET ExpList RBRACKET
	;
CurlyBracketedExpList : CurlyBracketedExp
	| CurlyBracketedExp COMMA CurlyBracketedExpList
	;
CurlyBracketedExp : LCURLY ExpList RCURLY
	| LCURLY CurlyBracketedExpList RCURLY
	;
ExpList : Exp
	| Exp COMMA ExpList
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
Primary : PrimaryBase
	| PrefixOrMultiOp Primary
	;
PrimaryBase : NonArrayedIdentifier
	| ArrayedIdentifier
	| SingleAccessor NonArrayedIdentifier
	| SingleAccessor ArrayedIdentifier
	| Instantiation
	| ExplicitFilter
	| Object
	| PrimLiteral
	| BracketedExp
	| PrimaryBase PostfixOp
	;
Node : NonArrayedIdentifier
	| ArrayedIdentifier
	| Instantiation
	| Filter
	| Object
	| PrimOpNode
	| PrimLiteral
	;
NonArrayedIdentifier : ID NonArrayedIdentifierSuffix
	| DPERIOD NonArrayedIdentifierSuffix
	;
ArrayedIdentifier : ID ArrayedIdentifierSuffix
	| DPERIOD ArrayedIdentifierSuffix
	;
OpenIdentifier : ID OpenIdentifierSuffix
	| DPERIOD OpenIdentifierSuffix
	;
NonArrayedIdentifierSuffix :
	| PERIOD ID NonArrayedIdentifierSuffix
	;
ArrayedIdentifierSuffix : PERIOD ID ArrayedIdentifierSuffix
	| PERIOD ArrayAccess IdentifierSuffix
	;
OpenIdentifierSuffix : PERIOD TIMES
	| PERIOD ID OpenIdentifierSuffix
	;
IdentifierSuffix :
	| PERIOD ID IdentifierSuffix
	| PERIOD ArrayAccess IdentifierSuffix
	;
BlankInstantiation : LSQUARE BlankInstantiationSource RSQUARE
	;
Instantiation : LSQUARE SingleInitInstantiationSource RSQUARE
	| LSQUARE SingleInitInstantiationSource RSQUARE BracketedExp
	| LSQUARE SingleInitInstantiationSource RSQUARE CurlyBracketedExp
	| LSQUARE MultiInitInstantiationSource RSQUARE
	| LSQUARE MultiInitInstantiationSource RSQUARE BracketedExp
	| LSQUARE SingleFlowInitInstantiationSource RSQUARE
	| LSQUARE MultiFlowInitInstantiationSource RSQUARE
	| LSQUARE CopyInstantiationSource RSQUARE
	| LSQUARE CloneInstantiationSource RSQUARE
	;
BlankInstantiationSource : NonArrayedIdentifier BlankInstantiationTypeSuffix
	;
SingleInitInstantiationSource : NonArrayedIdentifier
	;
MultiInitInstantiationSource : NonArrayedIdentifier MultiInitInstantiationTypeSuffix
	;
SingleFlowInitInstantiationSource : RARROW SingleInitInstantiationSource
	;
MultiFlowInitInstantiationSource : RARROW MultiInitInstantiationSource
	;
CopyInstantiationSource : SingleAccessor NonArrayedIdentifier
	| SingleAccessor ArrayedIdentifier
	;
CloneInstantiationSource : RARROW SingleAccessor
	;
ArrayAccess : LSQUARE Exp RSQUARE
	| LSQUARE Exp COLON Exp RSQUARE
	;
PrimOpNode : PrefixOp
	| InfixOp
	| MultiOp
	| PostfixOp
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
	;
MultiOp : PLUS
	| MINUS
	;
PostfixOp : DPLUS
	| DMINUS
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
ExplicitFilter : FilterHeader Block
	;
FilterHeader : LSQUARE ParamList RSQUARE
	| LSQUARE ParamList RetList RSQUARE
	| LSQUARE RetList RSQUARE
	;
NonRetFilterHeader : LSQUARE RSQUARE
	| LSQUARE ParamList RSQUARE
	;
RetFilterHeader : LSQUARE RetList RSQUARE
	;
ParamList : Param
	| Param COMMA ParamList
	;
RetList : DRARROW TypeList
	;
Param : Type ID
	| QUESTION ID
	;
Type : NonArrayedIdentifier
	| FilterType
	| ObjectType
	| NonArrayedIdentifier TypeSuffix
	| FilterType TypeSuffix
	| ObjectType TypeSuffix
	;
TypeSuffix : SLASH
	| ListTypeSuffix
	| StreamTypeSuffix
	| ArrayTypeSuffix
	| PoolTypeSuffix
	;
BlankInstantiationTypeSuffix : SLASH
	| StreamTypeSuffix
	| PoolTypeSuffix
	;
MultiInitInstantiationTypeSuffix : ArrayTypeSuffix
	;
ListTypeSuffix : LSQUARE RSQUARE
	| LSQUARE RSQUARE ListTypeSuffix
	;
StreamTypeSuffix : DSLASH
	| DSLASH StreamTypeSuffix
	;
ArrayTypeSuffix : LSQUARE Exp RSQUARE
	| LSQUARE Exp RSQUARE ArrayTypeSuffix
	;
PoolTypeSuffix : SLASH LSQUARE Exp RSQUARE
	| SLASH LSQUARE Exp RSQUARE ArrayTypeSuffix
	;
FilterType : LSQUARE TypeList RSQUARE
	| LSQUARE TypeList RetList RSQUARE
	| LSQUARE RetList RSQUARE
	;
ObjectType : LCURLY RCURLY
	| LCURLY ObjectTypeList RCURLY
	;
ObjectTypeList : InstructorType
	| InstructorType COMMA ObjectTypeList
	| OutstructorType
	| OutstructorType COMMA ObjectTypeList
	| MemberType
	| MemberType COMMA ObjectTypeList
	;
InstructorType : EQUALS
	| EQUALS LSQUARE RSQUARE
	| EQUALS LSQUARE TypeList RSQUARE
	;
OutstructorType : EQUALS LSQUARE DRARROW TypeList RSQUARE
	;
MemberType : ID EQUALS Type
	;
TypeList : Type
	| Type COMMA TypeList
	;
Block : LCURLY Pipes RCURLY
	;
Object : LSQUARE RSQUARE LCURLY ObjectPipes RCURLY
	;
ObjectPipes : LastInstructor
	| Instructor
	| Instructor ObjectPipes
	| Outstructor
	| Outstructor ObjectPipes
	| LastPipe
	| Pipe
	| Pipe ObjectPipes
	;
Instructor : EQUALS SEMICOLON
	| EQUALS NonRetFilterHeader Block
	| EQUALS NonRetFilterHeader Block SEMICOLON
	;
LastInstructor : EQUALS
	;
Outstructor : EQUALS RetFilterHeader Block
	| EQUALS RetFilterHeader Block SEMICOLON
	;
SingleAccess : SingleAccessor Node
	;
MultiAccess : MultiAccessor Node
	;
SingleAccessor : SLASH
	;
MultiAccessor : DSLASH
	| LSQUARE RSQUARE
	;
Compound : COMMA SingleStaticTerm
	;
Pack : RFLAG
	;
Unpack : LFLAG
	;
Link : DCOLON StaticTerm
	;
Loopback : LARROW
	;
Send : RARROW Node
	;
Swap : LRARROW Node
	;
Return : DRARROW
	;
%%
