/* Hey Emacs, this is code for lex to eat, treat it as -*-C-*- code 
ALPHA     [a-zA-Z]
ID        {ALPHA}+
ARG             [^=\n\r]+
ID              {ALPHA}({ALPHA}|{DIGIT})*
ARG             "=" .*

NAME            [A-Z]+
VALUE           [A-Za-z0-9@]+

OTHER [\!\@\#\г\д\$\%\&\/\{\(\[\)\]\}\?\+\\~\"\'\*\,\.\:]


\"{VALUE}\"     {yylval.str = strdup(yytext);  
                 return ARG;}


 */
%{
#include <stdio.h>
#include <string.h>

#include "parser.h"

%}

COMMENT         #.*
delim           [ \t\n\r]
WS              {delim}*
DIGIT           [0-9]
OTHER           [!@#гд\$%&\/\{\(\[\)\]\}?+\\~\'*,\.:]
ALPHA           [a-zA-Z_\-]

NAME            ^{WS}{ALPHA}+
VALUE           ({DIGIT}|{ALPHA}|{OTHER})+[;]*

%x VALUE_INPUT
%option yylineno
/* %option reentrant */
/* %option bison-bridge */
%option case-insensitive
%option nounput
/* %option noyywrap */
%%

{COMMENT}       {/* printf("%s", yytext); */}

{WS}            {/* printf("%s", yytext); */}

[sS][eE][tT]    {/* printf("SET "); */
                 return SET;}

=               {/* printf(" EQU "); */
                 return EQU;}

{NAME}          {yylval.str = strdup(yytext);  /*XXX - Check for NULL */
                 /* printf("ID(%s)", yylval.str); */
                 return ID;}

{VALUE}         {yylval.str = strdup(yytext); /* XXX - Check for NULL */
                 /* printf("ARG(%s)", yylval.str); */
                 return ARG;}

\"              {BEGIN(VALUE_INPUT);}
<VALUE_INPUT>.* {yylval.str = strdup(yytext);}
<VALUE_INPUT>\" {BEGIN(INITIAL); return ARG;}

%%

int yywrap (void)
{
  return 1;
}
