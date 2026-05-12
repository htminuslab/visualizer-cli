#ifndef CONFIG_H
#define CONFIG_H

typedef struct {
    char host[256];
    int  port;
    char cfg_path[512]; /* empty string if port was given directly */
} VCCConfig;

/* Reads VCC_CLI_INFO env var.
 * Returns 0 on success, -1 on error (prints JSON error to stdout). */
int config_load(VCCConfig *out);

#endif
