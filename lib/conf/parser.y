/* Hey Emacs, this is code for yacc to eat, treat it as -*-C-*- code
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

#include "conf.h"
#include "config.h"

#ifdef DEBUG
#define log(args...) fprintf (stderr, args);
#else
#define log(args...)
#endif

extern int yylex (void);

/* Defines needed for a pure reentrant parser/lexer.
   These are dsiabled for now due to failure to realize.
   #define YYLEX_PARAM   foo
#define YYPARSE_PARAM list
 */

#if 0 /* Not yet reentrant ... 
       * reverting in the meantime for the sake of cygwin port 
       */
extern int yyget_lineno (void);
#define yylineno      yyget_lineno ()
#else
extern int yylineno;
#endif

int yyerror (char *s)
{
  /* XXX - Add filename to syntax error message. */
  fprintf (stderr, "<filename>:%d -- %s\n", yylineno, s);

  return -1;
}

/* XXX - Add filename to syntax error message.
  Use NULL, we are not yet reentrant.
  otherwise use: if (conf_set_value ((param_t *)list, key, value))
*/

#define conf_set(key,value)				\
  if (conf_set_value ((param_t *)NULL, key, value))     \
    log ("Error on line %d in config file, "		\
	 "%s is not a valid identifier.\n",		\
	 yylineno, key)
 
%}

/* %pure-parser */
%union 
{
	char *str;
}
%token ID ARG EQU SET
%type  <str> ID ARG
%start line

%%

line   : line tuples
       | tuples
       ;

tuples : tuple
       | SET tuple
       ;

tuple  : ID EQU ARG             {log ("\nBison(ID:%s)(ARG:%s)", $1, $3);
                                 conf_set ($1, $3);}
       | ID ARG                 {log ("%s(%s)\n", $1, $2);
                                 conf_set ($1, $2);}
       ;
 
%%

