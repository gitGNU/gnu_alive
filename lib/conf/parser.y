/* Hey Emacs, this is code for yacc to eat, treat it as -*-C-*- code */
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
 */
#define YYPARSE_PARAM list

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

/* XXX - Add filename to syntax error message. */
#define conf_set(key,value)				\
  if (conf_set_value ((param_t *)list, key, value))	\
    fprintf (stderr,					\
	     "Error on line %d in config file, "	\
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

