//-------------------------------------------------------------------------------------------------
// Visualizer CLI 
//
// https://github.com/htminuslab/visualizer-cli            
//                                       
//-------------------------------------------------------------------------------------------------
//  Revision History:                                                        
//                                                                           
//  Date:        Revision    Author         
//  10 May 2025  0.1         HT-Lab/Claude Sonnet 4.6
//-------------------------------------------------------------------------------------------------

#include "vcc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#  include <ws2tcpip.h>
#else
#  include <sys/types.h>
#  include <netdb.h>
#  include <unistd.h>
#  include <arpa/inet.h>
#endif

/* ---- low-level recv wrapper: read exactly n bytes ---- */
static int recv_all(vcc_socket_t fd, char *buf, int n)
{
    int total = 0;
    while (total < n) {
        int r = recv(fd, buf + total, n - total, 0);
        if (r <= 0) return -1;
        total += r;
    }
    return 0;
}

/* ---- low-level send wrapper: send exactly n bytes ---- */
static int send_all(vcc_socket_t fd, const char *buf, int n)
{
    int total = 0;
    while (total < n) {
        int s = send(fd, buf + total, n - total, 0);
        if (s <= 0) return -1;
        total += s;
    }
    return 0;
}

/* ---- Send one VCC message ---- */
static int vcc_send(VCCConn *conn, const char *tcl)
{
    /* body: " <seq> {<tcl>}" */
    char body[VCC_REPLY_MAX];
    int body_len = snprintf(body, sizeof(body), " %d {%s}", conn->seq, tcl);
    if (body_len < 0 || body_len >= (int)sizeof(body)) {
        printf("{\"status\":\"error\",\"message\":\"Command too long\"}\n");
        return -1;
    }

    char header[11];
    snprintf(header, sizeof(header), "t %08X", (unsigned int)body_len);

    if (send_all(conn->fd, header, 10) != 0) {
        printf("{\"status\":\"error\",\"message\":\"Send failed (header)\"}\n");
        return -1;
    }
    if (send_all(conn->fd, body, body_len) != 0) {
        printf("{\"status\":\"error\",\"message\":\"Send failed (body)\"}\n");
        return -1;
    }
    conn->seq++;
    return 0;
}

/* ---- Receive one VCC message ----
 * Returns message type ('r','x','s','q') or -1 on error.
 * *out_seq is set from the parsed sequence number (-1 for signal/quit).
 * reply[] is populated with the braces-stripped content. */
static int vcc_recv(VCCConn *conn, int *out_seq, char *reply, int reply_sz)
{
    /* read 10-byte header */
    char hdr[11];
    if (recv_all(conn->fd, hdr, 10) != 0) {
        printf("{\"status\":\"error\",\"message\":\"Connection closed by server\"}\n");
        return -1;
    }
    hdr[10] = '\0';

    char type = hdr[0];

    if (type == 'q') {
        /* quit: no body */
        if (out_seq) *out_seq = -1;
        if (reply && reply_sz > 0) reply[0] = '\0';
        return 'q';
    }

    char size_hex[9];
    memcpy(size_hex, hdr + 2, 8);
    size_hex[8] = '\0';
    unsigned int body_len = (unsigned int)strtoul(size_hex, NULL, 16);

    if (body_len == 0) {
        if (out_seq) *out_seq = -1;
        if (reply && reply_sz > 0) reply[0] = '\0';
        return type;
    }

    char *body = malloc(body_len + 1);
    if (!body) {
        printf("{\"status\":\"error\",\"message\":\"Out of memory\"}\n");
        return -1;
    }

    if (recv_all(conn->fd, body, (int)body_len) != 0) {
        free(body);
        printf("{\"status\":\"error\",\"message\":\"Connection closed reading body\"}\n");
        return -1;
    }
    body[body_len] = '\0';

    /* signals have no seq number: body is " {content}" or "{content}" */
    if (type == 's') {
        if (out_seq) *out_seq = -1;
        /* strip leading space and braces */
        const char *p = body;
        while (*p == ' ') p++;
        if (*p == '{') p++;
        size_t clen = strlen(p);
        if (clen > 0 && p[clen-1] == '}') clen--;
        if (reply) {
            int copy = (int)clen < reply_sz - 1 ? (int)clen : reply_sz - 1;
            memcpy(reply, p, copy);
            reply[copy] = '\0';
        }
        free(body);
        return 's';
    }

    /* r or x: body is " <seq> {<content>}" */
    const char *p = body;
    while (*p == ' ') p++;
    /* parse seq */
    int seq = 0;
    while (*p >= '0' && *p <= '9') { seq = seq * 10 + (*p - '0'); p++; }
    if (out_seq) *out_seq = seq;
    while (*p == ' ') p++;

    /* strip braces */
    if (*p == '{') p++;
    size_t clen = strlen(p);
    if (clen > 0 && p[clen-1] == '}') clen--;

    if (reply) {
        int copy = (int)clen < reply_sz - 1 ? (int)clen : reply_sz - 1;
        memcpy(reply, p, copy);
        reply[copy] = '\0';
    }

    free(body);
    return type;
}

/* ---- Public API ---- */

int vcc_connect(VCCConn *conn, const char *host, int port)
{
#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("{\"status\":\"error\",\"message\":\"WSAStartup failed\"}\n");
        return -1;
    }
#endif

    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    char port_str[16];
    snprintf(port_str, sizeof(port_str), "%d", port);

    if (getaddrinfo(host, port_str, &hints, &res) != 0) {
        printf("{\"status\":\"error\",\"message\":\"Cannot resolve host: %s\"}\n", host);
#ifdef _WIN32
        WSACleanup();
#endif
        return -1;
    }

    conn->fd  = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    conn->seq = 1;

    if (conn->fd == VCC_INVALID_SOCKET) {
        printf("{\"status\":\"error\",\"message\":\"socket() failed\"}\n");
        freeaddrinfo(res);
#ifdef _WIN32
        WSACleanup();
#endif
        return -1;
    }

#ifdef _WIN32
    if (connect(conn->fd, res->ai_addr, (int)res->ai_addrlen) != 0) {
#else
    if (connect(conn->fd, res->ai_addr, res->ai_addrlen) != 0) {
#endif
        printf("{\"status\":\"error\",\"message\":\"Cannot connect to %s:%d\"}\n", host, port);
#ifdef _WIN32
        closesocket(conn->fd);
        WSACleanup();
#else
        close(conn->fd);
#endif
        freeaddrinfo(res);
        return -1;
    }
    freeaddrinfo(res);

    /* register client */
    char reply[VCC_REPLY_MAX];
    if (vcc_send(conn, "vccRegisterClient visualizer-cli") != 0) {
        vcc_disconnect(conn);
        return -1;
    }
    int seq;
    int t = vcc_recv(conn, &seq, reply, sizeof(reply));
    if (t != 'r') {
        printf("{\"status\":\"error\",\"message\":\"vccRegisterClient failed: %s\"}\n", reply);
        vcc_disconnect(conn);
        return -1;
    }

    return 0;
}

int vcc_exec(VCCConn *conn, const char *tcl, char *reply, int reply_sz)
{
    int sent_seq = conn->seq;

    if (vcc_send(conn, tcl) != 0)
        return -1;

    /* drain any stale signal messages first */
    for (;;) {
        int rseq;
        int t = vcc_recv(conn, &rseq, reply, reply_sz);
        if (t == -1)  return -1;
        if (t == 'q') {
            printf("{\"status\":\"error\",\"message\":\"Server closed connection\"}\n");
            return -1;
        }
        if (t == 's') continue; /* skip signals */
        if (rseq == sent_seq) {
            return (t == 'r') ? 0 : -1;
        }
    }
}

void vcc_disconnect(VCCConn *conn)
{
    if (conn->fd != VCC_INVALID_SOCKET) {
#ifdef _WIN32
        closesocket(conn->fd);
#else
        close(conn->fd);
#endif
        conn->fd = VCC_INVALID_SOCKET;
    }
#ifdef _WIN32
    WSACleanup();
#endif
}
