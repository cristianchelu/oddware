/*
 * Port return codes — spec/40-architecture/ports.md
 */

#include "port_err.h"

const char *port_err_name(port_err_t err)
{
    switch (err) {
    case PORT_OK:
        return "ok";
    case PORT_ERR_INVALID_ARG:
        return "invalid_arg";
    case PORT_ERR_NOT_FOUND:
        return "not_found";
    case PORT_ERR_IO:
        return "io";
    case PORT_ERR_NOT_SUPPORTED:
        return "not_supported";
    case PORT_ERR_BUSY:
        return "busy";
    default:
        return "unknown";
    }
}
