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

#include "commands.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---- helpers ---- */

static void json_ok(const char *result)
{
    /* Escape backslashes and double-quotes in the result string */
    printf("{\"status\":\"ok\",\"result\":\"");
    for (const char *p = result; *p; p++) {
        if (*p == '\\')      printf("\\\\");
        else if (*p == '"')  printf("\\\"");
        else if (*p == '\n') printf("\\n");
        else if (*p == '\r') { /* skip */ }
        else                 putchar(*p);
    }
    printf("\"}\n");
}

static void json_err(const char *msg)
{
    printf("{\"status\":\"error\",\"message\":\"");
    for (const char *p = msg; *p; p++) {
        if (*p == '\\')      printf("\\\\");
        else if (*p == '"')  printf("\\\"");
        else if (*p == '\n') printf("\\n");
        else if (*p == '\r') { /* skip */ }
        else                 putchar(*p);
    }
    printf("\"}\n");
}

/* Execute one TCL command and emit JSON result */
static int exec_and_print(VCCConn *conn, const char *tcl)
{
    char reply[VCC_REPLY_MAX];
    int rc = vcc_exec(conn, tcl, reply, sizeof(reply));
    if (rc == 0)
        json_ok(reply);
    else
        json_err(reply);
    return rc;
}

/* ---- help ---- */

int cmd_help(void)
{
    printf("{\n");
    printf("  \"commands\": [\n");
    printf("    {\"name\":\"help\",        \"usage\":\"help\",                              \"description\":\"List all available commands\"},\n");
    printf("    {\"name\":\"status\",      \"usage\":\"status\",                            \"description\":\"Report VCC server status and connection info\"},\n");
    printf("    {\"name\":\"run\",         \"usage\":\"run [100ns|8us|-all|...]\",          \"description\":\"Advance simulation by given time (default: one step)\"},\n");
    printf("    {\"name\":\"eval\",        \"usage\":\"eval <tcl command...>\",             \"description\":\"Send any Tcl command verbatim to Visualizer\"},\n");
    printf("    {\"name\":\"step\",        \"usage\":\"step [N]\",                          \"description\":\"Single-step the simulator N delta cycles (default: 1)\"},\n");
    printf("    {\"name\":\"run_status\",  \"usage\":\"run_status\",                        \"description\":\"Return the current simulator run state\"},\n");
    printf("    {\"name\":\"get_time\",    \"usage\":\"get_time\",                          \"description\":\"Return the current simulation time\"},\n");
    printf("    {\"name\":\"add_wave\",    \"usage\":\"add_wave <path> [path...]\",         \"description\":\"Add one or more signals to the wave window\"},\n");
    printf("    {\"name\":\"force\",       \"usage\":\"force <path> <value> [-at <time>]\", \"description\":\"Force a signal to a value, optionally at a specific time\"},\n");
    printf("    {\"name\":\"examine\",     \"usage\":\"examine <path> [-at <time>]\",       \"description\":\"Read a signal value at current or specified time\"}\n");
    printf("  ]\n");
    printf("}\n");
    return 0;
}

/* ---- status ---- */

int cmd_status(VCCConn *conn, const VCCConfig *cfg)
{
    char reply[VCC_REPLY_MAX];
    int rc = vcc_exec(conn, "vccserver status", reply, sizeof(reply));

    printf("{\n");
    printf("  \"status\":\"%s\",\n", rc == 0 ? "ok" : "error");
    printf("  \"host\":\"%s\",\n", cfg->host);
    printf("  \"port\":%d,\n", cfg->port);

    if (cfg->cfg_path[0]) {
        printf("  \"cfg_path\":\"");
        for (const char *p = cfg->cfg_path; *p; p++) {
            if (*p == '\\') printf("\\\\");
            else putchar(*p);
        }
        printf("\",\n");
    } else {
        printf("  \"cfg_path\":null,\n");
    }

    printf("  \"vccserver_status\":\"");
    for (const char *p = reply; *p; p++) {
        if (*p == '\\')      printf("\\\\");
        else if (*p == '"')  printf("\\\"");
        else if (*p == '\n') printf("\\n");
        else if (*p == '\r') { /* skip */ }
        else                 putchar(*p);
    }
    printf("\"\n}\n");

    return rc;
}

/* ---- run ---- */

int cmd_run(VCCConn *conn, int argc, char **argv)
{
    char tcl[256];
    if (argc == 0) {
        snprintf(tcl, sizeof(tcl), "run");
    } else {
        snprintf(tcl, sizeof(tcl), "run %s", argv[0]);
    }
    return exec_and_print(conn, tcl);
}

/* ---- eval ---- */

int cmd_eval(VCCConn *conn, int argc, char **argv)
{
    if (argc == 0) {
        json_err("eval requires a Tcl command argument");
        return -1;
    }
    /* join all remaining args with spaces */
    char tcl[4096];
    size_t pos = 0;
    for (int i = 0; i < argc; i++) {
        if (i > 0) {
            if (pos < sizeof(tcl) - 1) tcl[pos++] = ' ';
        }
        size_t len = strlen(argv[i]);
        if (pos + len >= sizeof(tcl)) {
            json_err("eval: command too long");
            return -1;
        }
        memcpy(tcl + pos, argv[i], len);
        pos += len;
    }
    tcl[pos] = '\0';
    return exec_and_print(conn, tcl);
}

/* ---- step ---- */

int cmd_step(VCCConn *conn, int argc, char **argv)
{
    char tcl[64];
    if (argc == 0) {
        snprintf(tcl, sizeof(tcl), "step");
    } else {
        snprintf(tcl, sizeof(tcl), "step %s", argv[0]);
    }
    return exec_and_print(conn, tcl);
}

/* ---- run_status ---- */

int cmd_run_status(VCCConn *conn)
{
    return exec_and_print(conn, "runStatus");
}

/* ---- get_time ---- */

int cmd_get_time(VCCConn *conn)
{
    return exec_and_print(conn, "simtime");
}

/* ---- add_wave ---- */

int cmd_add_wave(VCCConn *conn, int argc, char **argv)
{
    if (argc == 0) {
        json_err("add_wave requires at least one signal path");
        return -1;
    }

    printf("{\"status\":\"ok\",\"results\":[\n");
    int any_error = 0;
    for (int i = 0; i < argc; i++) {
        char tcl[1024];
        snprintf(tcl, sizeof(tcl), "wave add %s", argv[i]);
        char reply[VCC_REPLY_MAX];
        int rc = vcc_exec(conn, tcl, reply, sizeof(reply));
        if (rc != 0) any_error = 1;
        printf("  {\"signal\":\"");
        for (const char *p = argv[i]; *p; p++) {
            if (*p == '\\') printf("\\\\");
            else if (*p == '"') printf("\\\"");
            else putchar(*p);
        }
        printf("\",\"status\":\"%s\",\"result\":\"", rc == 0 ? "ok" : "error");
        for (const char *p = reply; *p; p++) {
            if (*p == '\\')      printf("\\\\");
            else if (*p == '"')  printf("\\\"");
            else if (*p == '\n') printf("\\n");
            else if (*p == '\r') { /* skip */ }
            else                 putchar(*p);
        }
        printf("\"}%s\n", i < argc - 1 ? "," : "");
    }
    printf("]}\n");
    return any_error ? -1 : 0;
}

/* ---- force ---- */

int cmd_force(VCCConn *conn, int argc, char **argv)
{
    /* usage: force <path> <value> [-at <time>] */
    if (argc < 2) {
        json_err("force requires <signal_path> <value> [-at <time>]");
        return -1;
    }

    char tcl[1024];
    if (argc >= 4 && strcmp(argv[2], "-at") == 0) {
        snprintf(tcl, sizeof(tcl), "force %s %s -at %s", argv[0], argv[1], argv[3]);
    } else {
        snprintf(tcl, sizeof(tcl), "force %s %s", argv[0], argv[1]);
    }
    return exec_and_print(conn, tcl);
}

/* ---- examine ---- */

int cmd_examine(VCCConn *conn, int argc, char **argv)
{
    /* usage: examine <path> [-at <time>] */
    if (argc < 1) {
        json_err("examine requires <signal_path> [-at <time>]");
        return -1;
    }

    char tcl[1024];
    if (argc >= 3 && strcmp(argv[1], "-at") == 0) {
        snprintf(tcl, sizeof(tcl), "examine -time %s %s", argv[2], argv[0]);
    } else {
        snprintf(tcl, sizeof(tcl), "examine %s", argv[0]);
    }
    return exec_and_print(conn, tcl);
}
