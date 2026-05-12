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

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static int all_digits(const char *s)
{
    if (!s || !*s) return 0;
    while (*s) {
        if (!isdigit((unsigned char)*s)) return 0;
        s++;
    }
    return 1;
}

/* Parse "port@hostname" line from vccserver.cfg */
static int parse_cfg_line(const char *line, VCCConfig *out)
{
    /* format: portnumber@hostname  (e.g. "14001@vismach08") */
    const char *at = strchr(line, '@');
    if (!at) {
        printf("{\"status\":\"error\",\"message\":\"vccserver.cfg: bad format, expected port@hostname\"}\n");
        return -1;
    }

    char port_str[16];
    size_t port_len = (size_t)(at - line);
    if (port_len == 0 || port_len >= sizeof(port_str)) {
        printf("{\"status\":\"error\",\"message\":\"vccserver.cfg: port field invalid\"}\n");
        return -1;
    }
    memcpy(port_str, line, port_len);
    port_str[port_len] = '\0';

    /* strip trailing whitespace from hostname */
    const char *host_start = at + 1;
    char host_buf[256];
    size_t h = 0;
    while (*host_start && !isspace((unsigned char)*host_start) && h < sizeof(host_buf) - 1)
        host_buf[h++] = *host_start++;
    host_buf[h] = '\0';

    if (h == 0) {
        printf("{\"status\":\"error\",\"message\":\"vccserver.cfg: hostname empty\"}\n");
        return -1;
    }

    out->port = atoi(port_str);
    if (out->port <= 0) {
        printf("{\"status\":\"error\",\"message\":\"vccserver.cfg: port number invalid\"}\n");
        return -1;
    }
    strncpy(out->host, host_buf, sizeof(out->host) - 1);
    out->host[sizeof(out->host) - 1] = '\0';
    return 0;
}

int config_load(VCCConfig *out)
{
    memset(out, 0, sizeof(*out));

    const char *val = getenv("VCC_CLI_INFO");
    if (!val || !*val) {
        printf("{\"status\":\"error\",\"message\":\"VCC_CLI_INFO environment variable not set\"}\n");
        return -1;
    }

    if (all_digits(val)) {
        /* numeric port → localhost */
        out->port = atoi(val);
        if (out->port <= 0 || out->port > 65535) {
            printf("{\"status\":\"error\",\"message\":\"VCC_CLI_INFO: port number out of range\"}\n");
            return -1;
        }
        strncpy(out->host, "127.0.0.1", sizeof(out->host) - 1);
        out->cfg_path[0] = '\0';
        return 0;
    }

    /* treat as path to vccserver.cfg */
    strncpy(out->cfg_path, val, sizeof(out->cfg_path) - 1);
    out->cfg_path[sizeof(out->cfg_path) - 1] = '\0';

    FILE *f = fopen(val, "r");
    if (!f) {
        printf("{\"status\":\"error\",\"message\":\"Cannot open vccserver.cfg: %s\"}\n", val);
        return -1;
    }

    char line[512];
    if (!fgets(line, sizeof(line), f)) {
        fclose(f);
        printf("{\"status\":\"error\",\"message\":\"vccserver.cfg is empty: %s\"}\n", val);
        return -1;
    }
    fclose(f);

    /* strip newline */
    line[strcspn(line, "\r\n")] = '\0';

    return parse_cfg_line(line, out);
}
