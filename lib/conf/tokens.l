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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <string.h>

#include "parser.h"

#ifdef DEBUG
#define log(args...) fprintf (stderr, args);
#else
#define log(args...)
#endif

%}

COMMENT         #.*
delim           [ \t\n\r]
WS              {delim}*
DIGIT           [0-9]
OTHER           [!@#гд\$%&\/\{\(\[\)\]\}?+\\~\'*,\.:]
ALPHA           [a-zA-Z_\-]

NAME            ^{WS}{ALPHA}+
VALUE           ({DIGIT}|{ALPHA}|{OTHER})+[;]*

%x ARG_MODE
%option yylineno
/* %option reentrant */
/* %option bison-bridge */
%option case-insensitive
%option nounput
/* %option noyywrap */
%%

{COMMENT}       {log ("%s", yytext);}

{WS}            {log ("%s", yytext);}

[sS][eE][tT]    {log ("SET ");
                 return SET;}

=               {log (" EQU ");
                 return EQU;}

{NAME}          {yylval.str = strdup(yytext);  /*XXX - Check for NULL */
                 log ("ID(%s)", yylval.str);
                 return ID;}

{VALUE}         {yylval.str = strdup(yytext); /* XXX - Check for NULL */
                 log ("ARG(%s)", yylval.str);
                 return ARG;}

\"               {BEGIN(ARG_MODE); yylval.str = NULL;}
<ARG_MODE>[^\"]+ {yylval.str = strdup(yytext);
                  log ("ARG(%s)", yylval.str);}
<ARG_MODE>\"     {BEGIN(INITIAL); if (!yylval.str) yylval.str = strdup(""); return ARG;}

%%

int yywrap (void)
{
  return 1;
}
