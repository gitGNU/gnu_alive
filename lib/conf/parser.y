/* Hey Emacs, this is code for yacc to eat, treat it as -*-C-*- code */
%{
#include <stdio.h>

#include "conf.h"
#include "config.h"


extern int yylineno;

int yyerror (char *s)
{
  fprintf (stderr, "ERR‰OR: %s\n", s);

  return -1;
}

#define conf_set(key,value)				\
   if (conf_set_value (key, value))			\
     fprintf (stderr,					\
	      "Error on line %d in config file, "	\
	      "%s is not a valid identifier.\n",	\
	      yylineno, key)

%}

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

