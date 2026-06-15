#ifndef CLI_H
#define CLI_H

#include <stdint.h>

typedef uint8_t (*cmd_fn_t)(uint8_t argc, char *argv[]);

typedef struct cmd {
    const char *cmd;
    const char *help;
    cmd_fn_t fn;
    struct cmd *sub;
} cmd_t;

#endif /* CLI_H */
