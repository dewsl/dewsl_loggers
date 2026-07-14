#ifndef SD_APP_H
#define SD_APP_H

#include "main.h"
#include <stdint.h>
#include <stdbool.h>

int init_sd(void);
void SD_App_Init(void);
int SD_App_IsReady(void);
int SD_App_AppendLine(const char *filename, const char *line);

void init_gids(void);

int unsent_row(void);
void open_sdata(void);
void rename_sd(void);
int unsent_count(void);

void print_stored_config(void);
void print_stored_config2(void);

char *get_value_from_line(char *line);
int8_t writeData(const char *fname, const char *data);

bool startsWithIgnoreCase(const char *str, const char *prefix);
unsigned int process_config_line(char *one_line);
int process_column_ids(char *line);
void open_config(void);

void printDirectory(void);
void dumpSDtoPC(void);

#endif
