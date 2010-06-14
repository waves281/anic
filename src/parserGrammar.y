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
%token DSLASH
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
LastPipe : LastDeclaration
	| NonEmptyTerms
	;
Declaration : ID EQUALS TypedStaticTerm
	| ID EQUALS TypedStaticTerm SEMICOLON
	| ID EQUALS BlankInstantiation
	| ID EQUALS BlankInstantiation SEMICOLON
	| ID ERARROW NonEmptyTerms SEMICOLON
	| AT NonArrayedIdentifier
	| AT ArrayedIdentifier
	| AT NonArrayedIdentifier SEMICOLON
	| AT ArrayedIdentifier SEMICOLON
	| AT LSQUARE NonArrayedIdentifier RSQUARE
	| AT LSQUARE ArrayedIdentifier RSQUARE
	| AT LSQUARE NonArrayedIdentifier RSQUARE SEMICOLON
	| AT LSQUARE ArrayedIdentifier RSQUARE SEMICOLON
	| DAT NonArrayedIdentifier
	| DAT ArrayedIdentifier
	| DAT NonArrayedIdentifier SEMICOLON
	| DAT ArrayedIdentifier SEMICOLON
	| DAT LSQUARE NonArrayedIdentifier RSQUARE
	| DAT LSQUARE ArrayedIdentifier RSQUARE
	| DAT LSQUARE NonArrayedIdentifier RSQUARE SEMICOLON
	| DAT LSQUARE ArrayedIdentifier RSQUARE SEMICOLON
	;
LastDeclaration : ID ERARROW NonEmptyTerms
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
	| Access
	;
SwitchTerm : DQUESTION LCURLY LabeledTerms RCURLY
	;
LabeledTerms : LastLabeledTerm
	| LabeledTerm LabeledTerms
	;
LabeledTerm: StaticTerm COLON SimpleTerm
	| StaticTerm COLON SimpleTerm SEMICOLON
	;
LastLabeledTerm : COLON SimpleTerm
	| COLON SimpleTerm SEMICOLON
	;
DynamicTerm : Compound
	| Link
	| Send
	| Swap
	| Return
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
Primary : PrimaryBase
	| PrefixOrMultiOp Primary
	;
PrimaryBase : NonArrayedIdentifier
	| ArrayedIdentifier
	| SingleAccessor NonArrayedIdentifier
	| SingleAccessor ArrayedIdentifier
	| Instantiation
	| Filter
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
ArrayedIdentifier : ID ArrayedIdentifierSuffix
	| DPERIOD ArrayedIdentifierSuffix
	;
NonArrayedIdentifier : ID NonArrayedIdentifierSuffix
	| DPERIOD NonArrayedIdentifierSuffix
	;
NonArrayedIdentifierSuffix :
	| PERIOD ID NonArrayedIdentifierSuffix
	;
ArrayedIdentifierSuffix : PERIOD ID ArrayedIdentifierSuffix
	| PERIOD ArrayAccess IdentifierSuffix
	;
IdentifierSuffix :
	| PERIOD ID IdentifierSuffix
	| PERIOD ArrayAccess IdentifierSuffix
	;
BlankInstantiation : LSQUARE NonCopyInstantiationSource RSQUARE
	;
Instantiation : LSQUARE CopyInstantiationSource RSQUARE
	| LSQUARE NonCopyInstantiationSource RSQUARE LARROW StaticTerm
	;
NonCopyInstantiationSource : NonArrayedIdentifier TypeSuffix
	;
CopyInstantiationSource : SingleAccessor NonArrayedIdentifier
	| SingleAccessor ArrayedIdentifier
	| MultiAccessor NonArrayedIdentifier
	| MultiAccessor ArrayedIdentifier
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
	;
Type : NonArrayedIdentifier TypeSuffix
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
Object : LCURLY InstructedObjectPipes RCURLY
	;
InstructedObjectPipes : LastInstructor
	| Instructor
	| Instructor ObjectPipes
	| Outstructor InstructedObjectPipes
	| Pipe InstructedObjectPipes
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
	| EQUALS LSQUARE RSQUARE SEMICOLON
	| EQUALS NonRetFilterHeader Block
	| EQUALS NonRetFilterHeader Block SEMICOLON
	;
LastInstructor : EQUALS
	| EQUALS LSQUARE RSQUARE
	;
Outstructor : EQUALS RetFilterHeader Block
	| EQUALS RetFilterHeader Block SEMICOLON
	;
Access : SingleAccessor Node
	| MultiAccessor Node
	;
SingleAccessor : SLASH
	;
MultiAccessor : DSLASH
	| LSQUARE RSQUARE
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
