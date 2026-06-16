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

typedef struct cli_history {
    char **history;
    char *input;
    char *parse_token;
    int history_max;
    int line_max;
    int index;
    int position;
    int full;
} cli_history_t;

typedef struct cli {
    int state;
    int echo;
    int (*get)(void);
    int (*put)(int);
    cmd_t *cmd;
    cli_history_t history;
} cli_t;

void cli_init(cli_t *cb);

#ifdef HOST_TEST
cli_t *cli_host_active(void);
#endif

#endif /* CLI_H */
