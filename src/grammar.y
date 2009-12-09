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
%left POWER ROOT
%left NOT COMPLEMENT
%left LBRACKET RBRACKET
/* never used in expressions */
%left UMINUS

%%
Program : Nodes
	;
Nodes : 
	| Node Nodes
	;
Node : TypedNode
	| PrimitiveNode
	;
TypedNode : Type ID LBRACKET ArgList RBRACKET LCURLY RegularPipes RCURLY
	;
PrimitiveNode : NonInhPrimNode
	| InhPrimNode
	;
NonInhPrimNode : ID LBRACKET ArgList RBRACKET LCURLY RegularPipes RCURLY
	;
InhPrimNode : ID COLON RegTypeList LBRACKET ArgList RBRACKET LCURLY RegularPipes RCURLY
	;
Type : RegType
	| ListType
	| FuncType
	;
RegType : ID
	;
ListType : ID Dimensions
	;
Dimensions : LSQUARE RSQUARE
	| LSQUARE RSQUARE Dimensions
FuncType : PrimFuncType
	| CompFuncType
	;
PrimFuncType : LBRACKET TypeList RBRACKET
	;
CompFuncType : ID LBRACKET TypeList RBRACKET
	;
TypeList : Type
	| Type COMMA TypeList
	;
RegTypeList : RegType
	| RegType COMMA RegTypeList
ArgList : 
	| NonEmptyArgList
	;
NonEmptyArgList : Type ID
	| Type ID COMMA NonEmptyArgList
	;
RegularPipes : 
	| RegularPipe SEMICOLON RegularPipes
	;
ThroughPipes : ThroughPipe
	| RegularPipe SEMICOLON ThroughPipes
	;
RegularPipe : Pipe
	;
ThroughPipe : Pipe
	;
Pipe : Term Terms
	;
Terms : 
	| Term Terms
	;
Term : TriggerTerm
	| Exp
	| Cast
	| FilterIn
	| FilterOut
	| Concat
	| LevelUp
	| Parallel
	| Send
	| Conditional
	| Fork
	;
TriggerTerm : LSLASH NonInitIdentifier
	;
Exp : IdOrLit
	| InfixExpHead
	;
Cast : AT NonInitIdentifier
	;
FilterIn : LSQUARE Range RSQUARE
	;
FilterOut : RSQUARE Range LSQUARE
	;
Concat : APPOSTROPHE IdLitOrInfix
	;
LevelUp : APPOSTROPHE APPOSTROPHE
	;
Parallel : COMMA IdLitOrInfix
	;
Send : EQUALS PossInitIdentifier
	;
Conditional : QUESTION Term
	| QUESTION Term COLON Term
	;
Fork : LCURLY ThroughPipes RCURLY
	| LCURLY RegularPipes RCURLY
	;
InfixExpHead : LBRACKET InfixExp RBRACKET
	;
InfixExp : IdOrLit
	| InfixExp NonInitIdentifier InfixExp %prec LBRACKET
	| MINUS InfixExp %prec UMINUS
/* NEVER used in final tree */
	| InfixExp DOR InfixExp
	| InfixExp DAND InfixExp
	| InfixExp OR InfixExp
	| InfixExp XOR InfixExp
	| InfixExp AND InfixExp
	| InfixExp DEQUALS InfixExp
	| InfixExp NEQUALS InfixExp
	| InfixExp LT InfixExp
	| InfixExp GT InfixExp
	| InfixExp LE InfixExp
	| InfixExp GE InfixExp
	| InfixExp LS InfixExp
	| InfixExp RS InfixExp
	| InfixExp PLUS InfixExp
	| InfixExp MINUS InfixExp
	| InfixExp TIMES InfixExp
	| InfixExp DIVIDE InfixExp
	| InfixExp MOD InfixExp
	| InfixExp POWER InfixExp
	| InfixExp ROOT InfixExp
	| InfixExp NOT InfixExp
	| InfixExp COMPLEMENT InfixExp
/* END never used in final tree */
	| LBRACKET InfixExp RBRACKET
	;
Range : IdNumOrInfix
	| IdNumOrInfix MINUS IdNumOrInfix
	;
IdOrLit : NonInitIdentifier
/* NEVER used in final tree */
	| DOR
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
	| PLUS
	| MINUS
	| TIMES
	| DIVIDE
	| MOD
	| POWER
	| ROOT
	| NOT
	| COMPLEMENT
/* END never used in final tree */
	| Literal
	;
IdLitOrInfix : NonInitIdentifier
/* NEVER used in final tree */
	| DOR
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
	| PLUS
	| MINUS
	| TIMES
	| DIVIDE
	| MOD
	| POWER
	| ROOT
	| NOT
	| COMPLEMENT
/* END never used in final tree */
	| Literal
	| InfixExpHead
	;
IdNumOrInfix : NonInitIdentifier
	| NumLiteral
	| InfixExpHead
	;
PossInitIdentifier : InitIdentifier
	| NonInitIdentifier
	;
InitIdentifier : LBRACKET Type ID RBRACKET
	;
NonInitIdentifier : ID
	| ID PERIOD NonInitIdentifier
	;
Literal : NumLiteral
	| StringLiteral
	;
NumLiteral : INT
	| FLOAT
	;
StringLiteral : QUOTEDLITERAL
	;
%%
