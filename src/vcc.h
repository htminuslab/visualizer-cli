#ifndef VCC_H
#define VCC_H

#ifdef _WIN32
#  include <winsock2.h>
   typedef SOCKET      vcc_socket_t;
#  define VCC_INVALID_SOCKET INVALID_SOCKET
#else
#  include <sys/socket.h>
   typedef int         vcc_socket_t;
#  define VCC_INVALID_SOCKET (-1)
#endif

#define VCC_REPLY_MAX 65536

typedef struct {
    vcc_socket_t fd;
    int          seq;
} VCCConn;

/* Connect to VCC server and send vccRegisterClient.
 * Returns 0 on success, -1 on error (prints JSON error). */
int vcc_connect(VCCConn *conn, const char *host, int port);

/* Send a TCL command and receive the reply.
 * reply[] is populated with the bare result text (NUL-terminated).
 * Returns 0 on 'r' (success), -1 on 'x' (error) or connection problem. */
int vcc_exec(VCCConn *conn, const char *tcl, char *reply, int reply_sz);

void vcc_disconnect(VCCConn *conn);

#endif
