/* Hey Emacs, this is code for lex to eat, treat it as -*-C-*- code 
ALPHA     [a-zA-Z]
ID        {ALPHA}+
ARG             [^=\n\r]+
ID              {ALPHA}({ALPHA}|{DIGIT})*
ARG             "=" .*

NAME            [A-Z]+
VALUE           [A-Za-z0-9@]+


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
OTHER           [!\"@#гд\$%&\/\{\(\[\)\]\}?+\\~\'*,\.:]
ALPHA           [a-zA-Z_\-]

NAME            ^{WS}{ALPHA}+
VALUE           ({DIGIT}|{ALPHA}|{OTHER})+

%option yylineno
%%

{COMMENT}       {/* printf("%s", yytext); */}

{WS}            {/* printf("%s", yytext); */}

[sS][eE][tT]    {/* printf("SET "); */
                 return SET;}

=               {/* printf(" EQU "); */
                 return EQU;}

{NAME}          {yylval.str = strdup(yytext);  /* XXX - Check for NULL */
                 /* printf("ID(%s)", yylval.str); */
                 return ID;}

{VALUE}[;]*     {yylval.str = strdup(yytext);  /* XXX - Check for NULL */
                 /* printf("ARG(%s)", yylval.str); */
                 return ARG;}

\"{VALUE}\"[;]* {yylval.str = strdup(yytext);  /* XXX - Check for NULL */
                 /* printf("ARG(%s)", yylval.str); */
                 return ARG;}

%%

int yywrap (void)
{
  return 1;
}

