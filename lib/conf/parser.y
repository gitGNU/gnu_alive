/* Hey Emacs, this is code for yacc to eat, treat it as -*-C-*- code */
%{
#include <stdio.h>

#include "conf.h"
#include "config.h"

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
  fprintf (stderr, "ERROR: %s\n", s);

  return -1;
}

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

tuple  : ID EQU ARG             {/* printf ("\nBison(ID:%s)(ARG:%s)", $1, $3); */
                                 conf_set ($1, $3);}
       | ID ARG                 {/* printf ("%s(%s)\n", $1, $2); */
                                 conf_set ($1, $2);}
       ;
 
%%

