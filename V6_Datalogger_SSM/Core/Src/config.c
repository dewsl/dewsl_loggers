#include "config.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

/*
 * STM32 port of cdrf_main/config.ino.
 *
 * This keeps the old behavior:
 * - read config flag/name from flash
 * - if override == 99, use SD CONFIG.txt via open_config()
 * - else use stored mastername and look it up in config_container[]
 * - parse column IDs into g_gids[][]
 *
 * No CFG= flow.
 * No hardcoded SARTA.
 */

#define FLASH_CONFIG_ADDR ((uint32_t)0x080FF800U)
#define FLASH_CONFIG_PAGE 511U

char v1_check[LOGGER_NAME_SIZE] = {0};

uint8_t b64 = 1;

char g_mastername[LOGGER_NAME_SIZE] = "XXXXX";

int g_gids[MAX_NODES][2];

uint8_t g_num_of_nodes = 0;
uint8_t g_sensor_version = 3;
uint8_t g_datalogger_version = 4;

uint16_t g_turn_on_delay = 1000;

int broad_timeout = 3000;

bool has_piezo = false;

int g_sampling_max_retry = 3;

char column_id_holder[COLUMN_ID_BUF_SIZE];



const char base64[64] = {
    'A','B','C','D','E','F','G','H',
    'I','J','K','L','M','N','O','P',
    'Q','R','S','T','U','V','W','X',
    'Y','Z',
    'a','b','c','d','e','f','g','h',
    'i','j','k','l','m','n','o','p',
    'q','r','s','t','u','v','w','x',
    'y','z',
    '0','1','2','3','4','5','6','7',
    '8','9',
    '+','/'
};

char g_delim[2] = "~";
char g_timestamp[20] = "000000000000";
char comm_mode[6] = "LORA";

char g_temp_dump[APP_BUFFER_SIZE];
char g_final_dump[APP_BUFFER_SIZE];
char g_no_gids_dump[APP_BUFFER_SIZE];
char text_message[APP_BUFFER_SIZE];

char g_build[200];
char g_build_final[500];

char print_buffer[250];
char num_buffer[20];

int g_sensor_type = 1;

void init_char_arrays(void)
{
    memset(g_temp_dump, 0, sizeof(g_temp_dump));
    memset(g_final_dump, 0, sizeof(g_final_dump));
    memset(g_no_gids_dump, 0, sizeof(g_no_gids_dump));
    memset(text_message, 0, sizeof(text_message));
    memset(g_build, 0, sizeof(g_build));
    memset(g_build_final, 0, sizeof(g_build_final));
}



static void flash_read_config(f_config *cfg)
{
    memcpy(cfg, (const void *)FLASH_CONFIG_ADDR, sizeof(f_config));
}

static int flash_write_config(const f_config *cfg)
{
    HAL_StatusTypeDef status;
    FLASH_EraseInitTypeDef erase = {0};
    uint32_t page_error = 0;
    uint32_t addr = FLASH_CONFIG_ADDR;
    uint32_t remaining = sizeof(f_config);
    const uint8_t *src = (const uint8_t *)cfg;

    HAL_FLASH_Unlock();

    erase.TypeErase = FLASH_TYPEERASE_PAGES;
    erase.Banks = FLASH_BANK_1;
    erase.Page = FLASH_CONFIG_PAGE;
    erase.NbPages = 1;

    status = HAL_FLASHEx_Erase(&erase, &page_error);

    if (status != HAL_OK) {
        HAL_FLASH_Lock();
        return 0;
    }

    while (remaining > 0) {
        uint64_t data64 = 0xFFFFFFFFFFFFFFFFULL;
        uint32_t chunk = (remaining >= 8U) ? 8U : remaining;

        memcpy(&data64, src, chunk);

        status = HAL_FLASH_Program(
            FLASH_TYPEPROGRAM_DOUBLEWORD,
            addr,
            data64
        );

        if (status != HAL_OK) {
            HAL_FLASH_Lock();
            return 0;
        }

        src += chunk;
        addr += 8U;
        remaining -= chunk;
    }

    HAL_FLASH_Lock();

    return 1;
}

static void uppercase_inplace(char *str)
{
    if (str == NULL) return;

    while (*str) {
        *str = (char)toupper((unsigned char)*str);
        str++;
    }
}

bool read_override(void)
{
    int override_value;
    bool override_flag;
    f_config read_flash;

    flash_read_config(&read_flash);

    override_value = read_flash.override_lib_config;

    if (override_value == 99) {
        override_flag = true;
    } else {
        override_flag = false;
    }

    return override_flag;
}

void flash_fetch(void)
{
    bool valid_flag = false;
    char flash_mastername[LOGGER_NAME_SIZE];
    int flash_check;
    int override_check;
    f_config fetch_config;

    flash_read_config(&fetch_config);

    memset(flash_mastername, 0, sizeof(flash_mastername));
    strncpy(flash_mastername, fetch_config.f_mastername, LOGGER_NAME_SIZE - 1);
    flash_mastername[LOGGER_NAME_SIZE - 1] = '\0';

    flash_check = fetch_config.check;
    override_check = fetch_config.override_lib_config;

    if (override_check == 99) {
        printf("SD CARD CONFIG IN USE\r\n");
        open_config();
    } else {
        if (flash_check != 99) {
            printf("NO SENSOR NAME SET FOR CONFIG!\r\n");
        } else {
            strcpy(g_mastername, flash_mastername);
            g_mastername[5] = '\0';

            for (int i = 0; i <= (lib_LOGGER_COUNT - 1); i++) {
                if (strcmp(config_container[i].lib_mastername, g_mastername) == 0) {
                    strcpy(g_mastername, config_container[i].lib_mastername);
                    g_mastername[5] = '\0';

                    strcpy(column_id_holder, config_container[i].lib_column_ids);
                    column_id_holder[strlen(column_id_holder)] = '\0';

                    parse_column_ids_from_library();

                    g_num_of_nodes = config_container[i].lib_num_of_nodes;
                    g_datalogger_version = config_container[i].lib_datalogger_version;
                    g_sensor_version = config_container[i].lib_sensor_version;
                    has_piezo = config_container[i].lib_has_piezo;
                    g_turn_on_delay = config_container[i].lib_turn_on_delay;
                    broad_timeout = config_container[i].lib_broad_timeout;
                    g_sampling_max_retry = config_container[i].lib_sampling_max_retry;
                    b64 = config_container[i].lib_b64;

                    valid_flag = true;
                }
            }

            if (valid_flag) {
                print_stored_config();
                print_stored_config2();
            } else {
                printf("No matching datalogger configuration found in library.\r\n");
            }
        }
    }
}

int flash_LED(void)
{
    int flash_check;
    f_config fetch_config;

    flash_read_config(&fetch_config);

    flash_check = fetch_config.check;

    return flash_check;
}

char *trim(char *str)
{
    while (isspace((unsigned char)*str)) {
        str++;
    }

    if (*str == '\0') {
        return str;
    }

    char *end = str + strlen(str) - 1;

    while (end > str && isspace((unsigned char)*end)) {
        end--;
    }

    *(end + 1) = '\0';

    return str;
}

void name_entry_from_string(const char *input_name)
{
    f_config flash_config;
    char serial_input[20];

    if (input_name == NULL || strlen(input_name) == 0) {
        printf("Input datalogger name is empty\r\n");
        return;
    }

    memset(&flash_config, 0, sizeof(flash_config));
    memset(serial_input, 0, sizeof(serial_input));

    strncpy(serial_input, input_name, sizeof(serial_input) - 1);
    serial_input[sizeof(serial_input) - 1] = '\0';

    uppercase_inplace(serial_input);

    strncpy(
        flash_config.f_mastername,
        serial_input,
        sizeof(flash_config.f_mastername) - 1
    );

    flash_config.f_mastername[sizeof(flash_config.f_mastername) - 1] = '\0';

    strncpy(v1_check, flash_config.f_mastername, sizeof(v1_check) - 1);
    v1_check[sizeof(v1_check) - 1] = '\0';
    v1_check[4] = '\0';

    if (
        strcmp("HUMB", v1_check) == 0 ||
        strcmp("LABT", v1_check) == 0 ||
        strcmp("LABB", v1_check) == 0
    ) {
        flash_config.f_mastername[sizeof(flash_config.f_mastername) - 2] = '\0';
    }

    flash_config.check = 99;
    flash_config.override_lib_config = 0;

    if (flash_write_config(&flash_config)) {
        printf("%s saved to flash\r\n", flash_config.f_mastername);
    } else {
        printf("flash write failed\r\n");
    }

    HAL_Delay(1000);
}

void parse_column_ids_from_library(void)
{
    int id_counter = 0;
    char local_holder[COLUMN_ID_BUF_SIZE];

    memset(local_holder, 0, sizeof(local_holder));

    strncpy(local_holder, column_id_holder, sizeof(local_holder) - 1);
    local_holder[sizeof(local_holder) - 1] = '\0';

    char *id_token = strtok(local_holder, ",");

    while (id_token != NULL && id_counter < MAX_NODES) {
        g_gids[id_counter][0] = atoi(id_token);
        id_counter++;
        id_token = strtok(NULL, ",");
    }
}
