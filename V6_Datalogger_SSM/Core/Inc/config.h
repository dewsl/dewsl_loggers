#ifndef CONFIG_H
#define CONFIG_H

#include "main.h"
#include "ssm_config.h"
#include <stdint.h>
#include <stdbool.h>

#define LOGGER_NAME_SIZE     6
#define COLUMN_ID_BUF_SIZE   500
#define MAX_NODES            40
#define VDATASIZE            300
#define APP_BUFFER_SIZE      5000

typedef struct {
    char f_mastername[LOGGER_NAME_SIZE];
    int override_lib_config;
    int check;
} f_config;

/* Global variables shared across files */
extern uint8_t b64;
extern char g_mastername[LOGGER_NAME_SIZE];
extern int g_gids[MAX_NODES][2];
extern uint8_t g_num_of_nodes;
extern uint8_t g_sensor_version;
extern uint8_t g_datalogger_version;
extern uint16_t g_turn_on_delay;
extern int broad_timeout;
extern bool has_piezo;
extern int g_sampling_max_retry;
extern char column_id_holder[COLUMN_ID_BUF_SIZE];
extern char v1_check[LOGGER_NAME_SIZE];

extern const char base64[64];

extern char g_delim[2];
extern char g_timestamp[20];
extern char comm_mode[6];

extern char g_temp_dump[APP_BUFFER_SIZE];
extern char g_final_dump[APP_BUFFER_SIZE];
extern char g_no_gids_dump[APP_BUFFER_SIZE];
extern char text_message[APP_BUFFER_SIZE];

extern char g_build[200];
extern char g_build_final[500];

extern char print_buffer[250];
extern char num_buffer[20];

extern int g_sensor_type;

void init_char_arrays(void);

/* These should be defined in separate STM32 config library file if used */
extern const libConfig config_container[];
extern const uint16_t LOGGER_COUNT;

/* Functions from config.ino */
bool read_override(void);
void flash_fetch(void);
int flash_LED(void);
char *trim(char *str);
void name_entry_from_string(const char *input_name);
void parse_column_ids_from_library(void);

/* From sd.c / sd.ino */
void open_config(void);
void print_stored_config(void);
void print_stored_config2(void);

#endif
