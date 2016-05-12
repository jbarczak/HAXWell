%option reentrant
%option noyywrap
%option yylineno
%option bison-bridge
%option bison-locations
%option extra-type="Parser*"
%option array
%option never-interactive

%{

#define _CRT_SECURE_NO_WARNINGS

#include "..\\GENAssembler_Parser.h"
#include "GENAssembler_Bison.hpp"
#include <stdlib.h>

using namespace GEN::Assembler::_INTERNAL;

#define YYSTYPE TokenStruct

#define YY_INPUT(buf,result,max_size) result = yyget_extra(yyscanner)->Read( buf, max_size );

#define YY_USER_ACTION yylloc->first_line = yylloc->last_line = yylineno; \
    yylloc->first_column = 0; yylloc->last_column = 0; yylval->LineNumber = yylineno;


%}

DIGIT   [0-9]
CHAR    [a-zA-Z_]
ID      [a-zA-Z_][a-zA-Z_0-9]*
INT     {DIGIT}+
HEX_INT "0x"[0-9a-fA-F]+
FLOAT   -?{DIGIT}+"."{DIGIT}+
FLOATF  {FLOAT}"f"

%%
[ \n\r\f\t]             /* skip whitespace */
[+*/(){};:,=&|!~^.%?]   { return yytext[0]; }
"-"|"<"|">"|"["|"]"     { return yytext[0]; }
"begin"                 { return T_KW_BEGIN; }
"end"                   { return T_KW_END; }
"curbe"                 { return T_KW_CURBE; }
"threads"               { return T_KW_THREADS; }
"reg"                   { return T_KW_REG; }
"send"                  { return T_KW_SEND; }
"bind"                  { return T_KW_BIND; }
"jmp"                   { return T_KW_JMP; }
"jmpif"                 { return T_KW_JMPIF; }
"pred"                  { return T_KW_PRED; }
"imm_uvec"              { return T_KW_IMM_UVEC; }
"imm_ivec"              { return T_KW_IMM_IVEC; }
"imm_fvec"              { return T_KW_IMM_FVEC; }
{HEX_INT}               { yylval->fields.Int    = strtoul(yytext,0,16); return T_UINT_LITERAL;   }
{INT}                   { yylval->fields.Int    = atoi(yytext);        return T_UINT_LITERAL;   }
"-"{INT}                { yylval->fields.Int    = atoi(yytext);        return T_INT_LITERAL;    }
{FLOATF}                { yylval->fields.Float  = atof(yytext);        return T_FLOAT_LITERAL;  }
{FLOAT}                 { yylval->fields.Double = strtod(yytext,0);    return T_DOUBLE_LITERAL; }
{ID}                    { yylval->fields.ID     = yyget_extra(yyscanner)->StoreString(yytext); return T_IDENTIFIER; }
. { 
    char buff[128]; 
    sprintf(buff,"Unknown character [%c]",yytext[0]); 
    yyget_extra(yyscanner)->Error( yylineno, buff );
    return T_UNKNOWN;    
}
%%

