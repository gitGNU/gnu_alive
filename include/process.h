#ifndef __PROCESS_H__
#define __PROCESS_H__

#include "config.h"

enum op {LOGIN, LOGOUT, STATUS, NOP};

typedef enum op op_t;


int process (config_data_t *config, op_t operation, int verbose);

#endif  /* __PROCESS_H__ */
