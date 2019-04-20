%option noyywrap
%option yylineno bison-locations
%option reentrant

%{

#include <string>
#include <iostream>
#include <AST.h>
#include "parser.hh"

//int yycolumn = 1;

#define YY_USER_ACTION         \
  yylloc->first_column = yycolumn; \
  yycolumn += yyleng;            \
  yylloc->last_line = yylineno; \
  yylloc->last_column = yycolumn; 

#define YY_NO_UNPUT

#define YYPARSE_PARAM yyscan_t scanner
#define YYLEX_PARAM scanner

#ifdef WIN32
#define YY_NO_UNISTD_H

#include <io.h> // _isatty
#define isatty _isatty
#endif

%}

%%

"break" return Break;
"goto" return Goto;
"do" return Do;
"end" return End;
"while" return While;
"repeat" return Repeat;
"if" return If;
"then" return Then;
"until" return Until;
"else" return Else;
"function" return Function;
"local" return Local;
"elseif" return Elseif;
"return" return Return;
"for" return For;
"extern" return Extern;

"meta" { return Meta; }

"nil" return Nil;
"true" { yylval->bval = true; return Bool; }
"false" { yylval->bval = false; return Bool; }

"operator" { return OperatorDef; }
"class" { return Class; }

"->" return ArrowRight;
"<-" return ArrowLeft;

">=" return GEQ;
"<=" return LEQ;
"~=" return NEQ;
"==" return EQ;
"..." return ThreeDot;

"'"."'" { yylval->cval = yytext[1]; return Char; }
L?\"(\\.|[^\\"])*\" { yylval->sval = new std::string(yytext + 1); yylval->sval->erase(yylval->sval->length() - 1); return LiteralString; }
[\*/\-\+=><][\*/\-\+=><]+|[§%&?#\|^€]* { yylval->sval = new std::string(yytext); return Operator; }

[a-zA-Z][a-zA-Z0-9_]* { yylval->sval = new std::string(yytext); return Name; }

(-?)[0-9]+"."[0-9]+ { yylval->fval = std::stof(yytext); return Number; }
(-?)[0-9]+ { yylval->ival = std::stoi(yytext); return Integer; }

\n yycolumn = 1;
"--".*\n { yycolumn = 1; }
[ \r\t] {}
. { return *yytext; }
%%

// [ \r\n\t]*|"--".*\n { /*yycolumn = 1;*/ }
