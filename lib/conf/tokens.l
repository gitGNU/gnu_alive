/* Hey Emacs, this is code for lex to eat, treat it as -*-C-*- code 
 *
 * Copyright (C) 2003, 2004 Joachim Nilsson <joachim!nilsson()member!fsf!org>
 *
 * qADSL is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License,
 * or (at your option) any later version.
 *
 * qADSL is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
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
