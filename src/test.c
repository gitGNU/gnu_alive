#include <stdlib.h>

#include "config.c"


/*
 *
 * TODO - make config_locate() depends only on input parameters,
 * not side effects like GLOBAL_CONF and USER_CONF.  If not, it will
 * be much easier to automate a test.
 *
 *
 * */

#define ok(arg) { if (!arg) {printf("%d\n", __LINE__);} }


int test_config_locate()
{
  char *dir;

  dir = mkdtemp(strdup("chroot-stuff-XXXXXX"));

  ok(0 == strcmp(config_locate("an-example-config-file.conf"), "an-example-config-file.conf"));


}
