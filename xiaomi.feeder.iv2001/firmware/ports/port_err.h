/*
 * Port return codes — spec/40-architecture/ports.md
 */

#ifndef PORT_ERR_H
#define PORT_ERR_H

typedef enum {
    PORT_OK = 0,
    PORT_ERR_INVALID_ARG,
    PORT_ERR_NOT_FOUND,
    PORT_ERR_IO,
    PORT_ERR_NOT_SUPPORTED,
    PORT_ERR_BUSY,
} port_err_t;

const char *port_err_name(port_err_t err);

#endif /* PORT_ERR_H */
