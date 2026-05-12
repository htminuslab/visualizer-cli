#ifndef COMMANDS_H
#define COMMANDS_H

#include "vcc.h"
#include "config.h"

int cmd_help(void);
int cmd_status(VCCConn *conn, const VCCConfig *cfg);
int cmd_run(VCCConn *conn, int argc, char **argv);
int cmd_eval(VCCConn *conn, int argc, char **argv);
int cmd_step(VCCConn *conn, int argc, char **argv);
int cmd_run_status(VCCConn *conn);
int cmd_get_time(VCCConn *conn);
int cmd_add_wave(VCCConn *conn, int argc, char **argv);
int cmd_force(VCCConn *conn, int argc, char **argv);
int cmd_examine(VCCConn *conn, int argc, char **argv);

#endif
