  
%locations 
%pure-parser
%error-verbose
%parse-param { GEN::Assembler::_INTERNAL::Parser* pParser }
%lex-param { YYLEX_PARAM }

%{
    #include "..\\GENAssembler_Parser.h"
    #include "GENAssembler_Bison.hpp"
    #include <stdio.h>
    
    using namespace GEN::Assembler::_INTERNAL;

    #define YYSTYPE TokenStruct
    #define YYLEX_PARAM     (pParser->m_scanner)

    
    int yylex( YYSTYPE *lvalp, YYLTYPE *llocp, yyscan_t scanner );
    void yyerror (YYLTYPE *locp, Parser* pParser, char const *msg )
    {
        pParser->Error( locp->first_line, msg );
    }
%}
%token T_UINT_LITERAL
%token T_INT_LITERAL
%token T_FLOAT_LITERAL
%token T_DOUBLE_LITERAL
%token T_IDENTIFIER
%token T_UNKNOWN
%token T_KW_BEGIN
%token T_KW_CURBE
%token T_KW_THREADS
%token T_KW_END
%token T_KW_REG
%token T_KW_BIND
%token T_KW_SEND
%token T_KW_JMP
%token T_KW_JMPIF
%token T_KW_PRED
%%

program: 
    preamble 
    instruction_list 
    end
;

preamble:
    begin 
|   preamble_decl preamble
;

preamble_decl:
     curbe_decl 
|    reg_decl
|    threads_decl
|    bind_decl
;

curbe_decl:
    curbe_begin                            { pParser->CURBEEnd(); } 
|   curbe_begin '=' '{' curbe_values '}'   { pParser->CURBEEnd(); }
|   curbe_begin '='  curbe_values    { pParser->CURBEEnd(); }
;

curbe_begin:
    T_KW_CURBE T_IDENTIFIER '[' T_UINT_LITERAL ']' { pParser->CURBEBegin( $2, $4.fields.Int ); }
;

curbe_values:
    curbe_value 
|   curbe_values ',' curbe_value
;

curbe_value:
    '{''}'                      { pParser->CURBEBreak( $1.LineNumber ); }
|   '{' curbe_literal_list '}'  { pParser->CURBEBreak( $1.LineNumber ); }
;

curbe_literal_list:
    curbe_literal 
|   curbe_literal_list ',' curbe_literal
;

curbe_literal:
    T_INT_LITERAL         { pParser->CURBEPush( $1.LineNumber, &$1.fields.Int   , sizeof($1.fields.Int    )); }
|   T_UINT_LITERAL        { pParser->CURBEPush( $1.LineNumber, &$1.fields.Int   , sizeof($1.fields.Int    )); }
|   T_FLOAT_LITERAL       { pParser->CURBEPush( $1.LineNumber, &$1.fields.Float , sizeof($1.fields.Float  )); }
|   T_DOUBLE_LITERAL      { pParser->CURBEPush( $1.LineNumber, &$1.fields.Double, sizeof($1.fields.Double )); }


threads_decl:
    T_KW_THREADS T_UINT_LITERAL { pParser->ThreadCount( $1 ); }
;

reg_decl:
    T_KW_REG T_IDENTIFIER                        { pParser->RegDeclaration( $2, 1 ); }
|   T_KW_REG T_IDENTIFIER '[' T_UINT_LITERAL ']' { pParser->RegDeclaration( $2, $4.fields.Int ); }


bind_decl:
    T_KW_BIND T_IDENTIFIER T_UINT_LITERAL { pParser->BindDeclaration( $2, $3.fields.Int ); }
;


begin:
    T_KW_BEGIN ':' { pParser->Begin( $1.LineNumber ); }
;


instruction_list:
    instruction 
|   instruction_list instruction;


instruction:
    label 
|   arith_instruction
|   send_instruction
|   jmp_instruction
|   predicate_block
;

label:
    T_IDENTIFIER ':'    { pParser->Label( $1 ); }
;

arith_instruction:
    unary_instruction
|   binary_instruction
|   ternary_instruction
;

unary_instruction:
    operation dest_reg ',' src_reg_or_literal   { pParser->Unary( $1.fields.node, $2.fields.node, $4.fields.node ); }
;

binary_instruction:
    operation dest_reg ',' src_reg ',' src_reg_or_literal  { pParser->Binary( $1.fields.node, $2.fields.node, $4.fields.node, $6.fields.node ); }
;

ternary_instruction:
    operation dest_reg ',' src_reg ',' src_reg ',' src_reg_or_literal  { pParser->Ternary( $1.fields.node, $2.fields.node, $4.fields.node, $6.fields.node, $8.fields.node ); }
;

send_instruction:
    T_KW_SEND T_IDENTIFIER '(' T_IDENTIFIER ')' ',' dest_reg ',' dest_reg { pParser->Send( $2, $4, $7.fields.node, $9.fields.node ); }
;

jmp_instruction:
    T_KW_JMP T_IDENTIFIER               { pParser->Jmp( $2 ); }
|   T_KW_JMPIF flag_ref T_IDENTIFIER    { pParser->JmpIf( $3, $2.fields.node ); }
;


predicate_block:
     predicate_block_header '{' block_instruction_list '}' { pParser->EndPredBlock(); }
;

predicate_block_header:
    T_KW_PRED flag_ref  { pParser->BeginPredBlock( $2.fields.node ); }
;

block_instruction_list:
    block_instruction
|   block_instruction_list block_instruction
;

block_instruction:
    label
|   arith_instruction
|   send_instruction
|   jmp_instruction
;



dest_reg:
    reg_identifier '.' sub_reg                       { $$.fields.node = pParser->DestReg( $1.fields.node, 1, $3.fields.node ); }
|   reg_identifier '.' sub_reg '<' T_UINT_LITERAL '>' { $$.fields.node = pParser->DestReg( $1.fields.node, $4.fields.Int, $3.fields.node ); }
;


src_reg:
    reg_identifier '.' sub_reg                { $$.fields.node = pParser->SourceReg( $1.fields.node, 0, $3.fields.node ); }
|   reg_identifier '.' sub_reg source_region  { $$.fields.node = pParser->SourceReg( $1.fields.node, $4.fields.node, $3.fields.node ); }
;

source_region:
    '<' T_UINT_LITERAL ',' T_UINT_LITERAL ',' T_UINT_LITERAL '>' { $$.fields.node = pParser->RegRegion( $1.LineNumber, $2.fields.Int, $4.fields.Int, $6.fields.Int ); }
;

reg_identifier:
    T_IDENTIFIER                                        { $$.fields.node = pParser->DirectRegReference( $1 ); }
|   T_IDENTIFIER '[' T_IDENTIFIER '.' T_UINT_LITERAL ']' { $$.fields.node = pParser->IndirectRegReference( $1, $3, $5.fields.Int ); }
;



sub_reg:
    T_IDENTIFIER { $$.fields.node = pParser->SubReg($1); }
;


src_reg_or_literal:
    src_reg         { $$ = $1; }
|   literal         { $$ = $1; }
;


literal:
    T_INT_LITERAL   { $$.fields.node = pParser->IntLiteral( $1.LineNumber, $1.fields.Int ); }
|   T_UINT_LITERAL  { $$.fields.node = pParser->IntLiteral( $1.LineNumber, $1.fields.Int ); }
|   T_FLOAT_LITERAL { $$.fields.node = pParser->FloatLiteral($1.LineNumber,  $1.fields.Float ); }
;


operation: 
    T_IDENTIFIER '(' T_UINT_LITERAL ')' optional_flag_ref { $$.fields.node = pParser->Operation( $1, $3.fields.Int, $5.fields.node ); }
;

optional_flag_ref:
    /* empty */ { $$.fields.node = 0; }
|    flag_ref   { $$ = $1; }
;

flag_ref:
    '(' T_IDENTIFIER '.' T_UINT_LITERAL ')' { $$.fields.node = pParser->FlagReference( $2, $4 ); }
;

end:
    T_KW_END { pParser->End(); }
;

%%