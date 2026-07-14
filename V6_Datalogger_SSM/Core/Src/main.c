/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : STM32 SSM main program body, patterned from cdrf_main.ino.
  ******************************************************************************
  */
/* USER CODE END Header */

#include "main.h"
#include "config.h"
#include "ssm_config.h"
#include "can.h"
#include "b64.h"
#include "sd.h"
#include "fatfs.h"

#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>
#include <stdlib.h>

CAN_HandleTypeDef hcan1;
SPI_HandleTypeDef hspi2;
UART_HandleTypeDef huart2;
UART_HandleTypeDef huart3;

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_CAN1_Init(void);
static void MX_SPI2_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_USART3_UART_Init(void);
static void Run_SSM_App(void);

#define OKSTR                 "OK\r\n"
#define ERRORSTR              "ERROR\r\n"
#define SARQ_OK_TIMEOUT_MS    15000
#define SARQ_SEND_RETRIES     3
#define RX_BUFFER_SIZE        256

static uint8_t ledState = 0;
static uint32_t previousMillis = 0;
static const uint32_t blink_interval_ms = 500;

// test lang
// test ulit
// magrereflect kaya?

/* ================= UART PRINT HELPERS ================= */

static void UART_Printf_Internal(UART_HandleTypeDef *huart, const char *fmt, va_list args)
{
    char buffer[512];

    vsnprintf(buffer, sizeof(buffer), fmt, args);
    HAL_UART_Transmit(huart, (uint8_t *)buffer, strlen(buffer), 1000);
}

void SSM_Printf(const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    UART_Printf_Internal(&huart3, fmt, args);
    va_end(args);
}

static void Debug_Printf(const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    UART_Printf_Internal(&huart2, fmt, args);
    va_end(args);
}

int __io_putchar(int ch)
{
    HAL_UART_Transmit(&huart2, (uint8_t *)&ch, 1, 1000);
    return ch;
}

static int UART_ReadLine(
    UART_HandleTypeDef *huart,
    char *buffer,
    int max_len,
    uint32_t timeout_per_char_ms
)
{
    uint8_t ch;
    int i = 0;

    while (i < max_len - 1) {
        if (HAL_UART_Receive(huart, &ch, 1, timeout_per_char_ms) == HAL_OK) {
            if (ch == '\n') {
                break;
            }

            if (ch != '\r') {
                buffer[i++] = (char)ch;
            }
        } else {
            break;
        }
    }

    buffer[i] = '\0';
    return i;
}

static int UART3_WaitForOK(uint32_t timeout_ms)
{
    char line[32];
    uint32_t start = HAL_GetTick();

    while ((HAL_GetTick() - start) < timeout_ms) {
        memset(line, 0, sizeof(line));

        if (UART_ReadLine(&huart3, line, sizeof(line), 100) > 0) {
            if (strstr(line, "OK") != NULL) {
                SSM_Printf("SARQ_ACK_OK\r\n");
                return 1;
            }
        }
    }

    SSM_Printf("SARQ_ACK_TIMEOUT\r\n");
    return 0;
}

static void uppercase_inplace(char *s)
{
    if (s == NULL) return;

    while (*s) {
        *s = (char)toupper((unsigned char)*s);
        s++;
    }
}

/* ================= CONFIG LOADING FROM SITE NAME ================= */

static int load_config_by_mastername(const char *site_name)
{
    char site[LOGGER_NAME_SIZE];

    if (site_name == NULL || strlen(site_name) == 0) {
        Debug_Printf("ERROR: Empty site name\r\n");
        return 0;
    }

    memset(site, 0, sizeof(site));
    strncpy(site, site_name, LOGGER_NAME_SIZE - 1);
    site[LOGGER_NAME_SIZE - 1] = '\0';
    uppercase_inplace(site);

    for (int i = 0; i < lib_LOGGER_COUNT; i++) {
    	if (strcmp(config_container[i].lib_mastername, site) == 0) {

    	    strncpy(g_mastername, config_container[i].lib_mastername, LOGGER_NAME_SIZE - 1);
    	    g_mastername[LOGGER_NAME_SIZE - 1] = '\0';

    	    g_num_of_nodes = config_container[i].lib_num_of_nodes;
    	    g_datalogger_version = config_container[i].lib_datalogger_version;
    	    g_sensor_version = config_container[i].lib_sensor_version;
    	    has_piezo = config_container[i].lib_has_piezo;
    	    g_turn_on_delay = config_container[i].lib_turn_on_delay;
    	    broad_timeout = config_container[i].lib_broad_timeout;
    	    g_sampling_max_retry = config_container[i].lib_sampling_max_retry;
    	    b64 = config_container[i].lib_b64;

    	    strncpy(column_id_holder, config_container[i].lib_column_ids, COLUMN_ID_BUF_SIZE - 1);
    	    column_id_holder[COLUMN_ID_BUF_SIZE - 1] = '\0';

    	    init_gids();
    	    parse_column_ids_from_library();

    	    Debug_Printf("Config loaded for %s\r\n", g_mastername);

    	    SSM_Printf(
    	        "CONFIG_LOADED:%s:%d\r\n",
    	        g_mastername,
    	        g_num_of_nodes
    	    );

    	    print_stored_config();

    	    return 1;
    	}
    }

    Debug_Printf("ERROR: No matching config for %s\r\n", site);
    SSM_Printf("ERROR:CONFIG_NOT_FOUND:%s\r\n", site);
    return 0;
}

/* ================= COLUMN CONTROL ================= */

static void columnOn(void)
{
#if defined(COLUMN_SWITCH_Pin) && defined(COLUMN_SWITCH_GPIO_Port)
    HAL_GPIO_WritePin(COLUMN_SWITCH_GPIO_Port, COLUMN_SWITCH_Pin, GPIO_PIN_SET);
#endif

#if defined(LED1_Pin) && defined(LED1_GPIO_Port)
    HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_SET);
#endif

    HAL_Delay(g_turn_on_delay);
}

static void columnOff(void)
{
#if defined(COLUMN_SWITCH_Pin) && defined(COLUMN_SWITCH_GPIO_Port)
    HAL_GPIO_WritePin(COLUMN_SWITCH_GPIO_Port, COLUMN_SWITCH_Pin, GPIO_PIN_RESET);
#endif

#if defined(LED1_Pin) && defined(LED1_GPIO_Port)
    HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET);
#endif

    HAL_Delay(g_turn_on_delay);
}

static float SSM_GetCurrent_mA(void)
{
    return 0.0f;
}

static float SSM_GetBusVoltage_V(void)
{
    return 0.0f;
}

/* ================= DEBUG MENU ================= */

static void due_command(void)
{
    Debug_Printf("======================================\r\n");
    Debug_Printf("KEY\tCOMMAND\r\n");
    Debug_Printf("======================================\r\n");
    Debug_Printf("[?]\tPrint stored config\r\n");
    Debug_Printf("[A]\tRead Sensor Data test\r\n");
    Debug_Printf("[B]\tRead Sensor Data and build SMS test\r\n");
    Debug_Printf("[C]\tPrint this menu\r\n");
    Debug_Printf("[D=NAME]\tSet SENSOR NAME to flash\r\n");
    Debug_Printf("[E]\tRead current and voltage\r\n");
    Debug_Printf("[F]\tFinal dump\r\n");
    Debug_Printf("======================================\r\n");
}

/* ================= COMMAND PARSING ================= */
/*
 * Supports both:
 *   Old: ARQCMD6T/260514151154
 *   New: ARQCMD6T/SARTA/260514151154
 *
 * New format is recommended because STM32 can load config from sARQ site name.
 */
static int parse_cmd(char *command_string)
{
    char local[RX_BUFFER_SIZE];
    char command_part[32];
    char site_part[LOGGER_NAME_SIZE];
    char timestamp_part[20];

    char *token;
    char *cmd6;
    char cmd_type;

    memset(local, 0, sizeof(local));
    memset(command_part, 0, sizeof(command_part));
    memset(site_part, 0, sizeof(site_part));
    memset(timestamp_part, 0, sizeof(timestamp_part));

    strncpy(local, command_string, sizeof(local) - 1);
    local[sizeof(local) - 1] = '\0';

    token = strtok(local, "/");
    if (token == NULL) {
        Debug_Printf("wait command returned 0\r\n");
        return 0;
    }

    strncpy(command_part, token, sizeof(command_part) - 1);
    command_part[sizeof(command_part) - 1] = '\0';
    uppercase_inplace(command_part);

    cmd6 = strstr(command_part, "CMD6");
    if (cmd6 == NULL) {
        Debug_Printf("wait command returned 0\r\n");
        return 0;
    }

    cmd_type = *(cmd6 + strlen("CMD6"));

    token = strtok(NULL, "/");

    if (token != NULL) {
        char possible_second[40];

        memset(possible_second, 0, sizeof(possible_second));
        strncpy(possible_second, token, sizeof(possible_second) - 1);

        token = strtok(NULL, "/");

        if (token != NULL) {
            /*
             * New format:
             * ARQCMD6T/SARTA/260514151154
             */
            strncpy(site_part, possible_second, LOGGER_NAME_SIZE - 1);
            site_part[LOGGER_NAME_SIZE - 1] = '\0';

            strncpy(timestamp_part, token, 12);
            timestamp_part[12] = '\0';

            if (!load_config_by_mastername(site_part)) {
                return 0;
            }
        } else {
            /*
             * Old format:
             * ARQCMD6T/260514151154
             */
            strncpy(timestamp_part, possible_second, 12);
            timestamp_part[12] = '\0';
        }

        strncpy(g_timestamp, timestamp_part, 12);
        g_timestamp[12] = '\0';

        Debug_Printf("g_mastername: %s\r\n", g_mastername);
        Debug_Printf("g_timestamp: %s\r\n", g_timestamp);
    }

    if (cmd_type == 'S') return 2;
    if (cmd_type == 'T') return 1;
    if (cmd_type == 'G') return 3;
    if (cmd_type == 'V') return 4;
    if (cmd_type == 'C') return 5;

    if (cmd_type == 'D') {
        char *newName = strtok(command_string + 8, "/");

        if (newName != NULL) {
            name_entry_from_string(newName);
        }

        return 6;
    }

    return 0;
}

/* ================= SENSOR DATA FLOW ================= */

static void read_data_from_column(char *column_data, int sensor_version, int sensor_type)
{
    char vc_container[32];

    memset(g_build, 0, sizeof(g_build));

    strncpy(g_build, g_mastername, sizeof(g_build) - 1);
    strncat(g_build, "*m*", sizeof(g_build) - strlen(g_build) - 1);

    snprintf(vc_container, sizeof(vc_container), "%.4f", SSM_GetCurrent_mA());
    strncat(g_build, vc_container, sizeof(g_build) - strlen(g_build) - 1);
    strncat(g_build, "*", sizeof(g_build) - strlen(g_build) - 1);

    snprintf(vc_container, sizeof(vc_container), "%.4f", SSM_GetBusVoltage_V());
    strncat(g_build, vc_container, sizeof(g_build) - strlen(g_build) - 1);

    columnOn();

    strncat(g_build, "*", sizeof(g_build) - strlen(g_build) - 1);

    snprintf(vc_container, sizeof(vc_container), "%.4f", SSM_GetCurrent_mA());
    strncat(g_build, vc_container, sizeof(g_build) - strlen(g_build) - 1);
    strncat(g_build, "*", sizeof(g_build) - strlen(g_build) - 1);

    snprintf(vc_container, sizeof(vc_container), "%.4f", SSM_GetBusVoltage_V());
    strncat(g_build, vc_container, sizeof(g_build) - strlen(g_build) - 1);

    memset(column_data, 0, APP_BUFFER_SIZE);

    if (sensor_version == 2) {
        Debug_Printf("Accel data\r\n");
        get_data(32, 1, column_data);
        get_data(33, 1, column_data);
    } else if (sensor_version == 3) {
        Debug_Printf("Accel data\r\n");
        get_data(11, 1, column_data);
        get_data(12, 1, column_data);

        Debug_Printf("Temperature data\r\n");
        get_data(22, 1, column_data);

        if (sensor_type == 2) {
            Debug_Printf("Soil Moisture sensor data\r\n");
            get_data(10, 1, column_data);
            get_data(13, 1, column_data);
        }
    } else if (sensor_version == 4) {
        Debug_Printf("Accel data\r\n");
        get_data(41, 1, column_data);
        get_data(42, 1, column_data);

        Debug_Printf("Temperature data\r\n");
        get_data(22, 1, column_data);
    } else if (sensor_version == 5) {
        Debug_Printf("Accel data\r\n");
        get_data(51, 1, column_data);
        get_data(52, 1, column_data);

        Debug_Printf("Temperature data\r\n");
        get_data(22, 1, column_data);
        get_data(23, 1, column_data);
        get_data(24, 1, column_data);
    } else if (sensor_version == 1) {
        get_data(256, 1, column_data);
    }

    columnOff();
    HAL_Delay(1000);

    strncat(g_build, "*", sizeof(g_build) - strlen(g_build) - 1);

    snprintf(vc_container, sizeof(vc_container), "%.4f", SSM_GetCurrent_mA());
    strncat(g_build, vc_container, sizeof(g_build) - strlen(g_build) - 1);
    strncat(g_build, "*", sizeof(g_build) - strlen(g_build) - 1);

    snprintf(vc_container, sizeof(vc_container), "%.4f", SSM_GetBusVoltage_V());
    strncat(g_build, vc_container, sizeof(g_build) - strlen(g_build) - 1);

    Debug_Printf("%s\r\n", g_build);
}

static int send_thru_sarq_wait_ok(const char *line)
{
    char tx_buffer[600];

    memset(tx_buffer, 0, sizeof(tx_buffer));

    snprintf(
        tx_buffer,
        sizeof(tx_buffer),
        "%s\r\n",
        line
    );

    /*
     * First send attempt
     */
    if (HAL_UART_Transmit(
            &huart3,
            (uint8_t *)tx_buffer,
            strlen(tx_buffer),
            1000
        ) == HAL_OK)
    {
        Debug_Printf("Sent to sARQ\r\n");
        HAL_Delay(300);
        return 1;
    }

    Debug_Printf("UART TX failed. Retrying...\r\n");

    HAL_Delay(200);

    /*
     * Retry only if UART TX failed
     */
    if (HAL_UART_Transmit(
            &huart3,
            (uint8_t *)tx_buffer,
            strlen(tx_buffer),
            1000
        ) == HAL_OK)
    {
        Debug_Printf("Retry success\r\n");
        HAL_Delay(300);
        return 1;
    }

    Debug_Printf("Retry failed\r\n");

    return 0;
}

static void send_current_voltage_thru_arq(void)
{
    char line[180];

    snprintf(
        line,
        sizeof(line),
        ">>1/1#%s*m*%.4f*%.4f*%.4f*%.4f*%s",
        g_mastername,
        SSM_GetCurrent_mA(),
        SSM_GetBusVoltage_V(),
        SSM_GetCurrent_mA(),
        SSM_GetBusVoltage_V(),
        g_timestamp
    );

    send_thru_sarq_wait_ok(line);
    SSM_Printf("STOPLORA\r\n");
}

static void operation(int sensor_type, char communication_mode[])
{
    if (sensor_type == 3) {
        open_sdata();
        return;
    }

    if (sensor_type == 4) {
        send_current_voltage_thru_arq();
        return;
    }

    if (sensor_type == 5) {
        SSM_Printf("OK\r\n");
        print_stored_config2();
        SSM_Printf("END OF CONFIG\r\n");
        return;
    }

    if (sensor_type == 6) {
        flash_fetch();
        SSM_Printf("END OF UPDATE\r\n");
        return;
    }

    if (strcmp(g_mastername, "XXXXX") == 0 || g_num_of_nodes == 0) {
        SSM_Printf(
            "CONFIG_STATE:%s:%d\r\n",
            g_mastername,
            g_num_of_nodes
        );

        SSM_Printf("ERROR:NO_CONFIG\r\n");
        SSM_Printf("STOPLORA\r\n");
        return;
    }

    if (g_mastername[3] == 'S') {
        g_sensor_type = 2;
    } else {
        g_sensor_type = 1;
    }

    init_char_arrays();

    read_data_from_column(g_final_dump, g_sensor_version, g_sensor_type);

    Debug_Printf("g_final_dump: %s\r\n", g_final_dump);

    b64_build_text_msgs(communication_mode, g_final_dump, text_message);

    Debug_Printf("%s\r\n", text_message);

    char temp_msg[APP_BUFFER_SIZE];

    strncpy(temp_msg, text_message, sizeof(temp_msg) - 1);
    temp_msg[sizeof(temp_msg) - 1] = '\0';

    char *token1 = strtok(temp_msg, g_delim);

    while (token1 != NULL) {
        Debug_Printf("Sending ::::%s\r\n", token1);
        send_thru_sarq_wait_ok(token1);
        token1 = strtok(NULL, g_delim);
    }

    if (strcmp(comm_mode, "LORA") == 0) {
        char unsent_value[10];

        snprintf(unsent_value, sizeof(unsent_value), "%d", unsent_count());

        memset(g_build_final, 0, sizeof(g_build_final));
        strncpy(g_build_final, ">>", sizeof(g_build_final) - 1);
        strncat(g_build_final, g_build, sizeof(g_build_final) - strlen(g_build_final) - 1);
        strncat(g_build_final, ">", sizeof(g_build_final) - strlen(g_build_final) - 1);
        strncat(g_build_final, unsent_value, sizeof(g_build_final) - strlen(g_build_final) - 1);
        strncat(g_build_final, "*", sizeof(g_build_final) - strlen(g_build_final) - 1);
        strncat(g_build_final, g_timestamp, sizeof(g_build_final) - strlen(g_build_final) - 1);

        send_thru_sarq_wait_ok(g_build_final);
    } else {
        send_thru_sarq_wait_ok(g_build_final);
        g_build_final[0] = '\0';
        g_build[0] = '\0';
    }

    SSM_Printf("STOPLORA\r\n");
}

/* ================= DEBUG COMMANDS ================= */

static void handle_debug_command(char *cmd)
{
    uppercase_inplace(cmd);

    if (cmd[0] == '?') {
        print_stored_config();
    } else if (cmd[0] == 'A') {
        char dummy[40];

        snprintf(
            dummy,
            sizeof(dummy),
            "ARQCMD6T/%s/260424080003",
            g_mastername
        );

        operation(parse_cmd(dummy), comm_mode);
        Debug_Printf(OKSTR);
    } else if (cmd[0] == 'B') {
        if (g_mastername[3] == 'S') {
            g_sensor_type = 2;
        } else {
            g_sensor_type = 1;
        }

        read_data_from_column(g_final_dump, g_sensor_version, g_sensor_type);
        Debug_Printf("%s\r\n", g_final_dump);
        b64_build_text_msgs(comm_mode, g_final_dump, text_message);
        Debug_Printf(OKSTR);
    } else if (cmd[0] == 'C') {
        due_command();
    } else if (cmd[0] == 'D') {
        char *eq = strchr(cmd, '=');

        if (eq != NULL && eq[1] != '\0') {
            name_entry_from_string(eq + 1);
            flash_fetch();
            Debug_Printf(OKSTR);
        } else {
            Debug_Printf("Use D=SARTA\r\n");
        }
    } else if (cmd[0] == 'E') {
        Debug_Printf("Current: %.4f mA\r\n", SSM_GetCurrent_mA());
        Debug_Printf("Bus Voltage: %.4f V\r\n", SSM_GetBusVoltage_V());
    } else if (cmd[0] == 'F') {
        Debug_Printf("g_final_dump: %s\r\n", g_final_dump);
        Debug_Printf(OKSTR);
    } else {
        Debug_Printf(ERRORSTR);
    }
}

static void handle_sarq_command(char *cmd)
{
    if (cmd[0] == '\0') return;

    SSM_Printf("CMD_RX:%s\r\n", cmd);

    int parsed_cmd = parse_cmd(cmd);

    SSM_Printf(
        "PARSED:%d:%s:%d:%s\r\n",
        parsed_cmd,
        g_mastername,
        g_num_of_nodes,
        g_timestamp
    );

    operation(parsed_cmd, comm_mode);
}

/* ================= MAIN ================= */

int main(void)
{
    HAL_Init();
    SystemClock_Config();

    MX_GPIO_Init();
    MX_CAN1_Init();
    MX_SPI2_Init();
    MX_USART2_UART_Init();
    MX_USART3_UART_Init();
    __HAL_UART_FLUSH_DRREGISTER(&huart2);
    __HAL_UART_FLUSH_DRREGISTER(&huart3);

    MX_FATFS_Init();
    SD_App_Init();
    SSM_Printf("AFTER_SD_INIT\r\n");

    Run_SSM_App();

    while (1) {}
}

static void Run_SSM_App(void)
{
    char rcvdChars[RX_BUFFER_SIZE];

    init_char_arrays();
    init_gids();
    init_sd();

    /*
     * Keep flash_fetch for backward compatibility.
     * If no STM32 flash config is set, sARQ command with site name will load config dynamically.
     */
    flash_fetch();

    CAN_App_Init();

    Debug_Printf("======================================\r\n");
    Debug_Printf("Firmware version: STM32-SSM-01\r\n");
    due_command();

    while (1) {
        uint32_t now = HAL_GetTick();

        if (now - previousMillis >= blink_interval_ms) {
            previousMillis = now;
            ledState = !ledState;

#if defined(LED1_Pin) && defined(LED1_GPIO_Port)
            HAL_GPIO_WritePin(
                LED1_GPIO_Port,
                LED1_Pin,
                ledState ? GPIO_PIN_SET : GPIO_PIN_RESET
            );
#endif
        }

        if (__HAL_UART_GET_FLAG(&huart3, UART_FLAG_RXNE)) {
            memset(rcvdChars, 0, sizeof(rcvdChars));
            UART_ReadLine(&huart3, rcvdChars, sizeof(rcvdChars), 1000);
            handle_sarq_command(rcvdChars);
        }

        if (__HAL_UART_GET_FLAG(&huart2, UART_FLAG_RXNE)) {
            memset(rcvdChars, 0, sizeof(rcvdChars));
            UART_ReadLine(&huart2, rcvdChars, sizeof(rcvdChars), 1000);
            handle_debug_command(rcvdChars);
        }
    }
}

/* Keep your existing CubeMX-generated functions below this point:
   MX_CAN1_Init()
   MX_SPI2_Init()
   MX_USART2_UART_Init()
   MX_USART3_UART_Init()
   MX_GPIO_Init()
   SystemClock_Config()
   Error_Handler()
*/

static void MX_CAN1_Init(void)
{
    hcan1.Instance = CAN1;
    hcan1.Init.Prescaler = 10;
    hcan1.Init.Mode = CAN_MODE_NORMAL;
    hcan1.Init.SyncJumpWidth = CAN_SJW_1TQ;
    hcan1.Init.TimeSeg1 = CAN_BS1_8TQ;
    hcan1.Init.TimeSeg2 = CAN_BS2_1TQ;
    hcan1.Init.TimeTriggeredMode = DISABLE;
    hcan1.Init.AutoBusOff = DISABLE;
    hcan1.Init.AutoWakeUp = DISABLE;
    hcan1.Init.AutoRetransmission = ENABLE;
    hcan1.Init.ReceiveFifoLocked = DISABLE;
    hcan1.Init.TransmitFifoPriority = DISABLE;

    if (HAL_CAN_Init(&hcan1) != HAL_OK) {
        Error_Handler();
    }
}

static void MX_SPI2_Init(void)
{
    hspi2.Instance = SPI2;
    hspi2.Init.Mode = SPI_MODE_MASTER;
    hspi2.Init.Direction = SPI_DIRECTION_2LINES;
    hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
    hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
    hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
    hspi2.Init.NSS = SPI_NSS_SOFT;
    hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
    hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
    hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    hspi2.Init.CRCPolynomial = 7;
    hspi2.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
    hspi2.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;

    if (HAL_SPI_Init(&hspi2) != HAL_OK) {
        Error_Handler();
    }
}

static void MX_USART2_UART_Init(void)
{
    huart2.Instance = USART2;
    huart2.Init.BaudRate = 9600;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
    huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;

    if (HAL_UART_Init(&huart2) != HAL_OK) {
        Error_Handler();
    }
}

static void MX_USART3_UART_Init(void)
{
    huart3.Instance = USART3;
    huart3.Init.BaudRate = 9600;
    huart3.Init.WordLength = UART_WORDLENGTH_8B;
    huart3.Init.StopBits = UART_STOPBITS_1;
    huart3.Init.Parity = UART_PARITY_NONE;
    huart3.Init.Mode = UART_MODE_TX_RX;
    huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart3.Init.OverSampling = UART_OVERSAMPLING_16;
    huart3.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
    huart3.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;

    if (HAL_UART_Init(&huart3) != HAL_OK) {
        Error_Handler();
    }
}

static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    HAL_GPIO_WritePin(GPIOB, SPI2_SDCS_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(CAN1_STB_GPIO_Port, CAN1_STB_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(CAN1_SHDN_GPIO_Port, CAN1_SHDN_Pin, GPIO_PIN_RESET);

    GPIO_InitStruct.Pin = SPI2_SDCS_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(SPI2_SDCS_GPIO_Port, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = CAN1_STB_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(CAN1_STB_GPIO_Port, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = CAN1_SHDN_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(CAN1_SHDN_GPIO_Port, &GPIO_InitStruct);
}

void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK) {
        Error_Handler();
    }

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI;
    RCC_OscInitStruct.MSIState = RCC_MSI_ON;
    RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_6;
    RCC_OscInitStruct.MSICalibrationValue = RCC_MSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;

    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }

    RCC_ClkInitStruct.ClockType =
        RCC_CLOCKTYPE_HCLK |
        RCC_CLOCKTYPE_SYSCLK |
        RCC_CLOCKTYPE_PCLK1 |
        RCC_CLOCKTYPE_PCLK2;

    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_MSI;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK) {
        Error_Handler();
    }
}

void Error_Handler(void)
{
    __disable_irq();

    while (1) {}
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
    (void)file;
    (void)line;
}
#endif
