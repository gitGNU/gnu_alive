/* conf.h - .conf file parser and user data holder
 */
#ifndef __CONF_H__
#define __CONF_H__

#include "config.h"

#ifndef bool
#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE  1
#endif
typedef enum {false = 0, true = 1} bool;
#endif



struct parameter
{
  /* Possible names for a tuple.
   * Primary name is first in the list, 
   * followed by any and all aliases.
   */
  char *names[10];    /* XXX - Hard limits are never good... */

  /* Parameter value */
  char *value;

  /* Default value */
  char *default_value;
};

typedef struct parameter param_t;


int conf_read_file (param_t *parameter_list, char *file);
int   conf_set_value (param_t *parameter_list, char *key, char *value);
char *conf_get_value (param_t *parameter_list, char *key);
bool  conf_get_bool  (param_t *parameter_list, char *key);


#endif /* __CONF_H__ */
