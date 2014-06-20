%{
#include <string>
#include "node.h"
#include "parser.hpp"
#define SAVE_TOKEN yylval.string = new std::string(yytext, yyleng)
#define TOKEN(t) (yylval.token = t)
int linenum = 1;
%}
%option noyywrap

%%

[ \t]					{printf("\t");}
[ \n]					{printf("\n"); ++linenum; } 
"return"				printf("TRETURN "); return TOKEN(TRETURN);
"if"					printf("TIF "); return TOKEN(TIF);
"else"					printf("TELSE "); return TOKEN(TELSE);
[a-zA-Z_][a-zA-Z0-9_]* 	printf("TIDENTIFIER "); SAVE_TOKEN; return TIDENTIFIER;
[0-9]+\.[0-9]* 			printf("TDOUBLE "); SAVE_TOKEN; return TDOUBLE;
[0-9]+					printf("TINTEGER "); SAVE_TOKEN; return TINTEGER;
":="					printf("TEQUAL "); return TOKEN(TEQUAL);
"=="					printf("TCEQ "); return TOKEN(TCEQ);
"!="					printf("TCNE "); return TOKEN(TCNE);
"<"						printf("TCLT "); return TOKEN(TCLT);
"<="					printf("TCLE "); return TOKEN(TCLE);
">"						printf("TCGT "); return TOKEN(TCGT);
">="					printf("TCGE "); return TOKEN(TCGE);
"("						printf("TLPAREN "); return TOKEN(TLPAREN);
")"						printf("TRPAREN ");return TOKEN(TRPAREN);
"{"						printf("TLBRACE "); return TOKEN(TLBRACE);
"}"						printf("TRBRACE "); return TOKEN(TRBRACE);
"."						printf("TDOT "); return TOKEN(TDOT);
","						printf("TCOMMA ");return TOKEN(TCOMMA);
"+"						printf("TPLUS "); return TOKEN(TPLUS);
"-"						printf("TMINUS "); return TOKEN(TMINUS);
"*"						printf("TMUL "); return TOKEN(TMUL);
"/"						printf("TDIV "); return TOKEN(TDIV);
";"						printf("TSEMICOL "); return TOKEN(TSEMICOL);
.						printf("Unknown token at line %d: %s\n", linenum, yytext); YY_FATAL_ERROR("");

%%
