#ifndef __HTTP_H__
#define __HTTP_H__

#include "config.h"

#define PRELOGIN_MSG                                        \
  "GET %s HTTP/0.9\n"                                       \
  "Referer: http://%s\n"                                    \
  "Host: %s\n"                                              \
  "Accept: */*\n\n"


#define LOGIN_MSG                                           \
  "POST %s HTTP/0.9\n"                                      \
  "Host: %s\n"                                              \
  "User-Agent: " PACKAGE_NAME " v" PACKAGE_VERSION "\n"     \
  "Accept: */*\n"                                           \
  "Referer: http://%s%s\n"                                  \
  "Content-type: application/x-www-form-urlencoded\n"       \
  "Content-length: %d\n\n"                                  \
  "%s"

#define LOGOUT_MSG                                          \
  "GET %s HTTP/0.9\n"                                       \
  "Referer: http://%s%s\n"                                  \
  "Host: %s\n"                                              \
  "Accept: */*\n\n"                                         \
  
/* XXX - Should perhaps be configurable from configure... */
#define MAX_RETRIES 3

int http_pre_login       (config_data_t *config, int verbose);
int http_internet_login  (config_data_t *config, int verbose);
int http_internet_logout (config_data_t *config, int verbose);
int http_do_login        (config_data_t *config, int verbose);


static inline int 
http_test_if_logged_out (config_data_t *config)
{
  int result = -1;

  if (strstr (config->get_msg, config->logged_out_string) != NULL)
    {
      config->logged_in = 0;
      result = 0;
    }
  else
    {
      result = -1;
      /* Don't set logged_in we leave it in the state it was ... */
    }

  return result;
}

static inline int 
http_test_if_logged_in (config_data_t *config)
{
  int result = -1;

  if (strstr (config->get_msg, config->logged_in_string) != NULL)
    {
      config->logged_in = 1;
      result = 0;
    }
  else
    {
      config->logged_in = 0;
      result = -1;
    }

  return result;
}

           
#endif /* __HTTP_H__ */
