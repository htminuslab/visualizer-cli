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

#include <stdio.h>
#include <string.h>
#include "config.h"
#include "vcc.h"
#include "commands.h"

static void print_usage(void)
{
    fprintf(stderr,
        "Usage: visualizer-cli <command> [args...]\n"
        "Run 'visualizer-cli help' for a list of commands.\n"
        "Set VCC_CLI_INFO to a port number or path to vccserver.cfg.\n");
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        print_usage();
        return 1;
    }

    const char *cmd = argv[1];

    /* help does not need a connection */
    if (strcmp(cmd, "help") == 0)
        return cmd_help();

    /* all other commands need a connection */
    VCCConfig cfg;
    if (config_load(&cfg) != 0)
        return 1;

    VCCConn conn;
    if (vcc_connect(&conn, cfg.host, cfg.port) != 0)
        return 1;

    int rc = 0;
    /* argv[2..] are passed to command handlers as their own argc/argv */
    int  sub_argc = argc - 2;
    char **sub_argv = argv + 2;

    if (strcmp(cmd, "status") == 0) {
        rc = cmd_status(&conn, &cfg);
    } else if (strcmp(cmd, "run") == 0) {
        rc = cmd_run(&conn, sub_argc, sub_argv);
    } else if (strcmp(cmd, "eval") == 0) {
        rc = cmd_eval(&conn, sub_argc, sub_argv);
    } else if (strcmp(cmd, "step") == 0) {
        rc = cmd_step(&conn, sub_argc, sub_argv);
    } else if (strcmp(cmd, "run_status") == 0) {
        rc = cmd_run_status(&conn);
    } else if (strcmp(cmd, "get_time") == 0) {
        rc = cmd_get_time(&conn);
    } else if (strcmp(cmd, "add_wave") == 0) {
        rc = cmd_add_wave(&conn, sub_argc, sub_argv);
    } else if (strcmp(cmd, "force") == 0) {
        rc = cmd_force(&conn, sub_argc, sub_argv);
    } else if (strcmp(cmd, "examine") == 0) {
        rc = cmd_examine(&conn, sub_argc, sub_argv);
    } else {
        printf("{\"status\":\"error\",\"message\":\"Unknown command: %s\"}\n", cmd);
        rc = 1;
    }

    vcc_disconnect(&conn);
    return rc == 0 ? 0 : 1;
}
