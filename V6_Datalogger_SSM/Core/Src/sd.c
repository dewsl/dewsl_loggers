#include "sd.h"
#include "config.h"
#include "fatfs.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

static int sd_ready = 0;

int init_sd(void)
{
    if (f_mount(&USERFatFS, USERPath, 1) != FR_OK) {
        printf("###  SD card initialization failed!  ###\r\n");
        sd_ready = 0;
        return -1;
    }

    sd_ready = 1;
    return 0;
}

void SD_App_Init(void)
{
    SSM_Printf("SD_App_Init called\r\n");

    if (init_sd() == 0) {
        SSM_Printf("SD INIT OK\r\n");

        if (SD_App_AppendLine("TEST.TXT", "SD CARD TEST OK")) {
            SSM_Printf("SD WRITE SUCCESS\r\n");
        } else {
            SSM_Printf("SD WRITE FAILED\r\n");
        }

    } else {
        SSM_Printf("SD INIT FAILED\r\n");
    }
}

int SD_App_IsReady(void)
{
    return sd_ready;
}

int SD_App_AppendLine(const char *filename, const char *line)
{
    FIL file;
    UINT bw;

    if (!sd_ready) {
        if (init_sd() != 0) return 0;
    }

    if (f_open(&file, filename, FA_OPEN_APPEND | FA_WRITE) != FR_OK) {
        return 0;
    }

    f_write(&file, line, strlen(line), &bw);
    f_write(&file, "\r\n", 2, &bw);
    f_close(&file);

    return 1;
}

void init_gids(void)
{
    for (int i = 0; i < g_num_of_nodes && i < MAX_NODES; i++) {
        g_gids[i][0] = 0;
        g_gids[i][1] = i + 1;
    }
}

int unsent_row(void)
{
    FIL file;
    char line[400];
    int totalrow = 0;

    if (!sd_ready) {
        if (init_sd() != 0) return 0;
    }

    if (f_open(&file, "unsent.txt", FA_READ) != FR_OK) {
        return 0;
    }

    while (f_gets(line, sizeof(line), &file)) {
        if (totalrow <= 50) {
            totalrow++;
            printf("%d\r\n", totalrow);
        } else {
            break;
        }
    }

    f_close(&file);
    return totalrow;
}

void open_sdata(void)
{
    /*
     * Patterned from sd.ino.
     * In STM32/V6 flow, sARQ handles final stacking/sending.
     * This function currently reads unsent.txt and prints rows for debug.
     */
    FIL file;
    char values[390];

    if (!sd_ready) {
        if (init_sd() != 0) return;
    }

    if (f_open(&file, "unsent.txt", FA_READ) != FR_OK) {
        printf("unsent.txt not found.\r\n");
        return;
    }

    while (f_gets(values, sizeof(values), &file)) {
        printf("%s", values);
    }

    f_close(&file);
}

void rename_sd(void)
{
    FIL src;
    FIL dst;
    UINT br;
    UINT bw;
    uint8_t buffer[128];

    if (!sd_ready) {
        if (init_sd() != 0) return;
    }

    f_unlink("unsent.txt");

    if (f_open(&src, "temp.txt", FA_READ) != FR_OK) {
        return;
    }

    if (f_open(&dst, "unsent.txt", FA_CREATE_ALWAYS | FA_WRITE) != FR_OK) {
        f_close(&src);
        return;
    }

    do {
        if (f_read(&src, buffer, sizeof(buffer), &br) != FR_OK) break;
        if (br > 0) f_write(&dst, buffer, br, &bw);
    } while (br > 0);

    f_close(&src);
    f_close(&dst);

    f_unlink("temp.txt");
}

int unsent_count(void)
{
    FIL file;
    char ch;
    UINT br;
    int counter = 0;

    if (!sd_ready) {
        if (init_sd() != 0) return -1;
    }

    if (f_open(&file, "unsent.txt", FA_READ) != FR_OK) {
        return -1;
    }

    while (f_read(&file, &ch, 1, &br) == FR_OK && br == 1) {
        if (ch == '\n') counter++;
    }

    f_close(&file);
    return counter;
}

void print_stored_config(void)
{
    char desc[30];

    if (g_datalogger_version == 2) {
        strncpy(comm_mode, "ARQ", sizeof(comm_mode) - 1);
        comm_mode[sizeof(comm_mode) - 1] = '\0';
    } else if (g_datalogger_version == 4) {
        strncpy(comm_mode, "LORA", sizeof(comm_mode) - 1);
        comm_mode[sizeof(comm_mode) - 1] = '\0';
    } else if (
        g_datalogger_version == 1 ||
        g_datalogger_version == 3 ||
        g_datalogger_version == 5
    ) {
        printf("g_datalogger_version == %d (default to LORA)\r\n", g_datalogger_version);
        strncpy(comm_mode, "LORA", sizeof(comm_mode) - 1);
        comm_mode[sizeof(comm_mode) - 1] = '\0';
    }

    printf("Comms: %s\r\n", comm_mode);
    printf("======================================\r\n");

    sprintf(desc, "%-24s", "Sensor Name:");
    printf("%s%s\r\n", desc, g_mastername);

    sprintf(desc, "%-24s", "Datalogger Version:");
    printf("%s%d\r\n", desc, g_datalogger_version);

    sprintf(desc, "%-24s", "Sensor Version:");
    printf("%s%d\r\n", desc, g_sensor_version);

    sprintf(desc, "%-24s", "Broadcast Timeout:");
    printf("%s%d\r\n", desc, broad_timeout);

    sprintf(desc, "%-24s", "Sampling Max Retry:");
    printf("%s%d\r\n", desc, g_sampling_max_retry);

    sprintf(desc, "%-24s", "Turn On Delay:");
    printf("%s%d\r\n", desc, g_turn_on_delay);

    sprintf(desc, "%-24s", "Number of Nodes");
    printf("%s%d\r\n", desc, g_num_of_nodes);

    printf("======================================\r\n");
    printf("Geographic ID\t\tUnique ID\r\n");
    printf("======================================\r\n");

    for (int i = 0; i < g_num_of_nodes && i < MAX_NODES; i++) {
        printf("\t%2d\t\t%4d\r\n", g_gids[i][1], g_gids[i][0]);
    }

    printf("======================================\r\n");
}

void print_stored_config2(void)
{
    /*
     * Old sd.ino prints this to Serial2.
     * In STM32, huart3 is the sARQ UART via SSM_Printf().
     */
    char desc[30];

    SSM_Printf("======================================\r\n");

    sprintf(desc, "%-24s", "Sensor Name:");
    SSM_Printf("%s%s\r\n", desc, g_mastername);

    sprintf(desc, "%-24s", "Datalogger Version:");
    SSM_Printf("%s%d\r\n", desc, g_datalogger_version);

    sprintf(desc, "%-24s", "Sensor Version:");
    SSM_Printf("%s%d\r\n", desc, g_sensor_version);

    sprintf(desc, "%-24s", "Broadcast Timeout:");
    SSM_Printf("%s%d\r\n", desc, broad_timeout);

    sprintf(desc, "%-24s", "Sampling Max Retry:");
    SSM_Printf("%s%d\r\n", desc, g_sampling_max_retry);

    sprintf(desc, "%-24s", "Turn On Delay:");
    SSM_Printf("%s%d\r\n", desc, g_turn_on_delay);

    sprintf(desc, "%-24s", "Number of Nodes");
    SSM_Printf("%s%d\r\n", desc, g_num_of_nodes);

    SSM_Printf("======================================\r\n");
    SSM_Printf("Geographic ID\t\tUnique ID\r\n");
    SSM_Printf("======================================\r\n");

    for (int i = 0; i < g_num_of_nodes && i < MAX_NODES; i++) {
        SSM_Printf("\t%2d\t\t%4d\r\n", g_gids[i][1], g_gids[i][0]);
    }

    SSM_Printf("======================================\r\n");
}

char *get_value_from_line(char *line)
{
    static char result[1000];
    char *ptr;
    int line_length;

    memset(result, 0, sizeof(result));

    if (line == NULL) return NULL;

    line_length = strlen(line);

    for (int i = 0; i < line_length; i++) {
        if (line[i] == ' ') {
            memmove(&line[i], &line[i + 1], line_length - i);
            line_length--;
            i--;
        }
    }

    ptr = strrchr(line, '=');

    if (ptr != NULL) {
        strncpy(result, ptr + 1, sizeof(result) - 1);
        result[sizeof(result) - 1] = '\0';

        char *newline = strpbrk(result, "\r\n");
        if (newline) *newline = '\0';

        return result;
    }

    return NULL;
}

int8_t writeData(const char *fname, const char *data)
{
    FIL file;
    UINT bw;

    char filename[100] = {0};
    char logger_file_name[7] = {0};

    if (!sd_ready) {
        if (init_sd() != 0) {
            printf("SD.begin() Failed!\r\n");
            return -1;
        }
    }

    for (int i = 0; i < 6 && fname[i] != '\0'; i++) {
        logger_file_name[i] = fname[i];
    }

    logger_file_name[6] = '\0';

    strcpy(filename, logger_file_name);
    strcat(filename, ".TXT");

    printf("%s\r\n", filename);

    if (f_open(&file, filename, FA_OPEN_APPEND | FA_WRITE) != FR_OK) {
        printf("Can't write to file\r\n");
        return -1;
    }

    f_write(&file, g_mastername, strlen(g_mastername), &bw);
    f_write(&file, ",", 1, &bw);
    f_write(&file, data, strlen(data), &bw);
    f_write(&file, ",", 1, &bw);
    f_write(&file, g_timestamp, strlen(g_timestamp), &bw);
    f_write(&file, "\r\n", 2, &bw);

    f_close(&file);
    return 0;
}

bool startsWithIgnoreCase(const char *str, const char *prefix)
{
    if (str == NULL || prefix == NULL) return false;

    while (*prefix) {
        if (tolower((unsigned char)*str) != tolower((unsigned char)*prefix)) {
            return false;
        }

        str++;
        prefix++;
    }

    return true;
}

unsigned int process_config_line(char *one_line)
{
    char *str1 = one_line;
    char *value;
    int temp_int = 0;

    if (startsWithIgnoreCase(str1, "mastername") ||
        startsWithIgnoreCase(str1, "MasterName")) {

        value = get_value_from_line(str1);

        if (value != NULL) {
            strncpy(g_mastername, value, 5);
            g_mastername[5] = '\0';
        }

        return 0;

    } else if (startsWithIgnoreCase(str1, "turn_on_delay")) {

        value = get_value_from_line(str1);
        if (value != NULL) g_turn_on_delay = (uint16_t)atoi(value);

        return 0;

    } else if (startsWithIgnoreCase(str1, "PIEZO") ||
               startsWithIgnoreCase(str1, "Piezo")) {

        value = get_value_from_line(str1);
        if (value != NULL) temp_int = atoi(value);

        has_piezo = (temp_int == 1);

        return 0;

    } else if (startsWithIgnoreCase(str1, "dataloggerVersion")) {

        value = get_value_from_line(str1);
        if (value != NULL) g_datalogger_version = (uint8_t)atoi(value);

        return 0;

    } else if (startsWithIgnoreCase(str1, "sensorVersion")) {

        value = get_value_from_line(str1);
        if (value != NULL) g_sensor_version = (uint8_t)atoi(value);

        return 0;

    } else if (startsWithIgnoreCase(str1, "broadcastTimeout") ||
               startsWithIgnoreCase(str1, "broadcast_timeout")) {

        value = get_value_from_line(str1);
        if (value != NULL) broad_timeout = atoi(value);

        return 0;

    } else if (startsWithIgnoreCase(str1, "sampling_max_retry") ||
               startsWithIgnoreCase(str1, "sampling_max_num_of_retry")) {

        value = get_value_from_line(str1);
        if (value != NULL) g_sampling_max_retry = atoi(value);

        return 0;

    } else if (startsWithIgnoreCase(str1, "columnids") ||
               startsWithIgnoreCase(str1, "column1") ||
               startsWithIgnoreCase(str1, "columnIDs")) {

        g_num_of_nodes = (uint8_t)process_column_ids(str1);

        return 0;

    } else if (startsWithIgnoreCase(str1, "b64")) {

        value = get_value_from_line(str1);
        if (value != NULL) b64 = (uint8_t)atoi(value);

        return 0;

    } else if (startsWithIgnoreCase(str1, "endofconfig") ||
               startsWithIgnoreCase(str1, "ENDOFCONFIG")) {

        return 1;
    }

    return 0;
}

int process_column_ids(char *line)
{
    char *value;
    char local[1000];
    char *token;
    int i = 0;

    value = get_value_from_line(line);

    if (value == NULL) return 0;

    memset(local, 0, sizeof(local));
    strncpy(local, value, sizeof(local) - 1);

    token = strtok(local, ",");

    while (token != NULL && i < MAX_NODES) {
        g_gids[i][0] = atoi(token);
        g_gids[i][1] = i + 1;

        i++;

        token = strtok(NULL, ",");
    }

    return i;
}

void open_config(void)
{
    FIL file;
    char one_line[1000];

    if (!sd_ready) {
        if (init_sd() != 0) return;
    }

    if (f_open(&file, "CONFIG.txt", FA_READ) != FR_OK &&
        f_open(&file, "CONFIG.TXT", FA_READ) != FR_OK) {

        printf("CONFIG.txt not found.\r\n");
        return;
    }

    memset(one_line, 0, sizeof(one_line));

    while (f_gets(one_line, sizeof(one_line), &file)) {
        if (process_config_line(one_line)) {
            break;
        }

        memset(one_line, 0, sizeof(one_line));
    }

    printf("Finished config processing\r\n");
    print_stored_config();

    f_close(&file);
}

void printDirectory(void)
{
    printf("printDirectory() not fully implemented for FatFS yet.\r\n");
}

void dumpSDtoPC(void)
{
    printf("Open terminal application to dump data to file\r\n");
    printf("dumpSDtoPC() not fully implemented for FatFS yet.\r\n");
}
