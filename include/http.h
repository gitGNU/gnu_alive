#ifndef __HTTP_H__
#define __HTTP_H__

#include "config.h"

#if 0 				/* From ciclogin, uses HTTP v0.9 */
#define PRELOGIN_MSG				\
  "GET /sd/init HTTP/1.1\n"			\
  "Host: 10.0.0.6\n"				\
  "Referer: http://10.0.0.6/sd/init\n"		\
  "Connection: keep-alive\n\n"
#endif                                                                           

/**
 * Needs the following args
 * INIT, SERVER, INIT, SERVER
 */
#define OLD_PRELOGIN_MSG							\
  "GET %s HTTP/1.0\n"						\
  "Referer: http://%s%s\n"					\
  "Connection: Keep-Alive\n"						\
  "User-Agent: Mozilla/4.73 [en] (X11; U; Linux 2.4.21 i686)\n"		\
  "Host: %s\n"								\
  "Accept: image/gif, image/x-xbitmap, image/jpeg, "			\
  "image/pjpeg, image/png, */*\n"					\
  "Accept-Encoding: gzip\n"						\
  "Accept-Language: en\n"						\
  "Accept-Charset: iso-8859-1,*,utf-8\n\n"

#define PRELOGIN_MSG							\
  "GET %s HTTP/1.1\n"						\
  "Referer: http://%s\n"					\
  "Host: %s\n"								\
  "Accept: */*\n\n"


#if 0 				/* From ciclogin, uses HTTP v0.9 */
#define LOGIN_MSG						\
  "POST /sd/login HTTP/1.1\n"				\
  "Host: 10.0.0.6\n"					\
  "Connection: close\n"					\
  "Referer: http://10.0.0.6/sd/init\n"			\
  "Content-Type: application/x-www-form-urlencoded\n"	\
  "Content-Length: 56\n\n"
#endif

/**
 * LOGIN
 * Args: LOGIN, SERVER, INIT, SERVER, 
 *       LENGTH
 * Telia
 *       "username=uXXX&password=ABC&submitForm=Logga+in"
 * Tiscali
 *       "username=uXXX&password=ABC&submitForm=Login"
 *       
 */
#define OLD_LOGIN_MSG                                               \
  "POST %s HTTP/1.0\n"						\
  "Referer: http://%s%s\n"					\
  "Connection: Keep-Alive\n"						\
  "User-Agent: Mozilla/4.73 [en] (X11; U; Linux 2.4.21 i686)\n"		\
  "Host: %s\n"								\
  "Accept: image/gif, image/x-xbitmap, image/jpeg, "			\
  "image/pjpeg, image/png, */*\n"					\
  "Accept-Encoding: gzip\n"						\
  "Accept-Language: en\n"						\
  "Accept-Charset: iso-8859-1,*,utf-8\n"				\
  "Content-type: application/x-www-form-urlencoded\n"			\
  "Content-length: %d\n\n"						\
  "%s\n\n\n\n\n"

#define LOGIN_MSG                                               \
  "POST %s HTTP/1.1\n"						\
  "Host: %s\n"								\
  "User-Agent: " PACKAGE_NAME " v" PACKAGE_VERSION "\n"		\
  "Accept: */*\n"					\
  "Referer: http://%s%s\n"					\
  "Content-type: application/x-www-form-urlencoded\n"			\
  "Content-length: %d\n\n"						\
  "%s"

/* XXX - Make this more generic, configuration parameter...
 * Perhaps even read it from /sd/init??
 */
#define OLD_LOGIN_STRING						\
  "username=%s&password=%s&submitForm=Logga+in"

#define LOGIN_STRING						\
  "username=%s&password=%s&submitForm=Login"

                                                                                
#if 0 				/* From ciclogin, uses HTTP v0.9 */
#define LOGOUT_MSG                              \
  "GET /sd/logout HTTP/1.1\n"			\
  "Host: 10.0.0.6\n"				\
  "Referer: http://10.0.0.6/sd/init\n"		\
  "Connection: close\n\n"
#endif

/* LOGOUT, SERVER, INIT, SERVER
 */
#define OLD_LOGOUT_MSG                                              \
  "GET %s HTTP/1.0\n"						\
  "Referer: http://%s%s\n"					\
  "Connection: Keep-Alive\n"						\
  "User-Agent: Mozilla/4.73 [en] (X11; U; Linux 2.4.21 i686)\n"		\
  "Host: %s\n"								\
  "Accept: image/gif, image/x-xbitmap, image/jpeg, "			\
  "image/pjpeg, image/png, */*\n"					\
  "Accept-Encoding: gzip\n"						\
  "Accept-Language: en\n"						\
  "Accept-Charset: iso-8859-1,*,utf-8\n\n"

#define LOGOUT_MSG                                              \
  "GET %s HTTP/0.9\n"						\
  "Referer: http://%s%s\n"					\
  "Host: %s\n"								\
  "Accept: */*\n\n"					      \

/* Should perhaps be configurable from configure... */
#define MAX_RETRIES 3

int open_server     (char *name, short port, int verbose);
int pre_login       (config_data_t *config, int verbose);
int internet_login  (config_data_t *config, int verbose);
int internet_logout (config_data_t *config, int verbose);
int log_login       (config_data_t *config, int verbose);
int new_login (config_data_t *config, int verbose);

                                                                                
#endif /* __HTTP_H__ */
