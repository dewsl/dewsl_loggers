#include "can.h"
#include "config.h"
#include "b64.h"

#include <string.h>
#include <stdio.h>
#include <math.h>

extern CAN_HandleTypeDef hcan1;

CAN_AppFrame g_can_buffer[CAN_ARRAY_BUFFER_SIZE];

static int compute_axis_debug(int low, int high)
{
    if (g_sensor_version < 5) {
        if (high >= 240) {
            high = high - 240;
            return (low + (high * 256)) - 4095;
        } else {
            return (low + (high * 256));
        }
    }

    return (int16_t)((high << 8) | low);
}

static float compute_magnitude_debug(int x, int y, int z)
{
    return sqrtf(
        ((float)x * (float)x) +
        ((float)y * (float)y) +
        ((float)z * (float)z)
    );
}

static int get_accel_number_from_msgid(int msgid)
{
    if (msgid == 11 || msgid == 32 || msgid == 41 || msgid == 51) return 1;
    if (msgid == 12 || msgid == 33 || msgid == 42 || msgid == 52) return 2;
    return 0;
}

void CAN_App_Init(void)
{
#if defined(CAN1_STB_Pin) && defined(CAN1_STB_GPIO_Port)
    HAL_GPIO_WritePin(CAN1_STB_GPIO_Port, CAN1_STB_Pin, GPIO_PIN_RESET);
#endif

#if defined(CAN1_SHDN_Pin) && defined(CAN1_SHDN_GPIO_Port)
    HAL_GPIO_WritePin(CAN1_SHDN_GPIO_Port, CAN1_SHDN_Pin, GPIO_PIN_RESET);
#endif

    HAL_Delay(10);

    CAN_FilterTypeDef filter;
    memset(&filter, 0, sizeof(filter));

    filter.FilterBank = 0;
    filter.FilterMode = CAN_FILTERMODE_IDMASK;
    filter.FilterScale = CAN_FILTERSCALE_32BIT;

    filter.FilterIdHigh = 0x0000;
    filter.FilterIdLow = 0x0000;
    filter.FilterMaskIdHigh = 0x0000;
    filter.FilterMaskIdLow = 0x0000;

    filter.FilterFIFOAssignment = CAN_RX_FIFO0;
    filter.FilterActivation = ENABLE;

    if (HAL_CAN_ConfigFilter(&hcan1, &filter) != HAL_OK) {
        printf("CAN filter config failed\r\n");
    }

    if (HAL_CAN_Start(&hcan1) != HAL_OK) {
        printf("CAN start failed\r\n");
    }

    clear_can_buffer(g_can_buffer);
}

void clear_can_buffer(CAN_AppFrame can_buffer[])
{
    for (int i = 0; i < CAN_ARRAY_BUFFER_SIZE; i++) {
        can_buffer[i].id = 0;
        can_buffer[i].dlc = 0;
        memset(can_buffer[i].data, 0, sizeof(can_buffer[i].data));
    }
}

void get_data(int cmd, int transmit_id, char *final_dump)
{
    int retry_count = 0;
    int respondents = 0;
    int count = 0;
    int uid = 0;

    if (cmd < 100) {
        for (retry_count = 0; retry_count < g_sampling_max_retry; retry_count++) {
            canSend(cmd, transmit_id);

            respondents = get_all_frames(
                broad_timeout,
                g_can_buffer,
                g_num_of_nodes
            );

            if (respondents == g_num_of_nodes) {
                printf("Complete frames! :)\r\n");
                break;
            } else {
                snprintf(
                    print_buffer,
                    sizeof(print_buffer),
                    "%d / %d received / expected frames.",
                    respondents,
                    g_num_of_nodes
                );

                printf("%s\r\n", print_buffer);
            }
        }

    } else if (cmd == 256) {
        for (int i = 0; i < g_num_of_nodes; i++) {
            uid = g_gids[i][0];

            version_1(uid);

            get_all_frames(
                broad_timeout,
                g_can_buffer,
                g_num_of_nodes
            );

            count = count_frames(g_can_buffer);

            for (int j = 0; j < count; j++) {
                if (g_can_buffer[j].id != 0) {
                    b64_write_frame_to_dump(g_can_buffer[j], g_temp_dump);
                }
            }
        }

    } else if ((cmd >= 100) && (cmd < 255)) {
        for (int i = 0; i < g_num_of_nodes; i++) {
            uid = g_gids[i][0];

            poll_command(cmd, uid);

            printf("Polling UID: %d", uid);

            for (retry_count = 0; retry_count < g_sampling_max_retry + 2; retry_count++) {
                printf(" .");

                if (get_one_frame(POLL_TIMEOUT, g_can_buffer, uid) == uid) {
                    printf(" OK\r\n");
                    break;
                }
            }
        }
    }

    count = count_frames(g_can_buffer);

    for (int i = 0; i < count; i++) {
        if (g_can_buffer[i].id != 0) {
            b64_write_frame_to_dump(g_can_buffer[i], g_temp_dump);
        }
    }

    strncat(
        g_temp_dump,
        g_delim,
        APP_BUFFER_SIZE - strlen(g_temp_dump) - 1
    );

    printf("g_temp_dump: %s\r\n", g_temp_dump);

    b64_process_g_temp_dump(
        g_temp_dump,
        final_dump,
        g_no_gids_dump
    );

    memset(g_temp_dump, 0, APP_BUFFER_SIZE);

    clear_can_buffer(g_can_buffer);

    printf("=================================\r\n");
}

int get_all_frames(int timeout_ms, CAN_AppFrame can_buffer[], int expected_frames)
{
    uint32_t timestart = HAL_GetTick();
    int i = 0;

    CAN_RxHeaderTypeDef rxHeader;
    uint8_t rxData[8];

    do {
        check_can_status();

        if (HAL_CAN_GetRxFifoFillLevel(&hcan1, CAN_RX_FIFO0) > 0) {
            if (HAL_CAN_GetRxMessage(&hcan1, CAN_RX_FIFO0, &rxHeader, rxData) == HAL_OK) {
                if (i < CAN_ARRAY_BUFFER_SIZE) {
                    if (g_sensor_version == 1) {
                        can_buffer[i].id =
                            ((rxHeader.IDE == CAN_ID_EXT)
                            ? rxHeader.ExtId
                            : rxHeader.StdId) / 8;
                    } else {
                        can_buffer[i].id =
                            (rxHeader.IDE == CAN_ID_EXT)
                            ? rxHeader.ExtId
                            : rxHeader.StdId;
                    }

                    can_buffer[i].dlc = rxHeader.DLC;

                    for (int b = 0; b < 8; b++) {
                        can_buffer[i].data[b] = rxData[b];
                    }

                    i++;

                    process_all_frames(g_can_buffer);

                    i = count_frames(g_can_buffer);

                    if (i >= expected_frames) {
                        break;
                    }
                }
            }
        }

    } while ((HAL_GetTick() - timestart) <= (uint32_t)timeout_ms);

    process_all_frames(g_can_buffer);

    int total_frames = count_frames(g_can_buffer);

    SSM_Printf(
        "CAN_FRAMES:%d/%d\r\n",
        total_frames,
        expected_frames
    );

    print_frames_sorted_by_gid(g_can_buffer);

    return total_frames;
}

int get_one_frame(int timeout_ms, CAN_AppFrame can_buffer[], int expected_uid)
{
    uint32_t timestart = HAL_GetTick();
    int i = count_frames(can_buffer);

    CAN_RxHeaderTypeDef rxHeader;
    uint8_t rxData[8];

    do {
        check_can_status();

        if (HAL_CAN_GetRxFifoFillLevel(&hcan1, CAN_RX_FIFO0) > 0) {
            if (HAL_CAN_GetRxMessage(&hcan1, CAN_RX_FIFO0, &rxHeader, rxData) == HAL_OK) {
                uint32_t incoming_id =
                    (rxHeader.IDE == CAN_ID_EXT)
                    ? rxHeader.ExtId
                    : rxHeader.StdId;

                if (i < CAN_ARRAY_BUFFER_SIZE) {
                    can_buffer[i].id = incoming_id;
                    can_buffer[i].dlc = rxHeader.DLC;

                    for (int b = 0; b < 8; b++) {
                        can_buffer[i].data[b] = rxData[b];
                    }

                    i++;
                }

                if ((int)incoming_id == expected_uid) {
                    process_all_frames(g_can_buffer);
                    print_frames_sorted_by_gid(g_can_buffer);
                    return expected_uid;
                }
            }
        }

    } while ((HAL_GetTick() - timestart) <= (uint32_t)timeout_ms);

    process_all_frames(g_can_buffer);
    print_frames_sorted_by_gid(g_can_buffer);

    return 0;
}

void process_all_frames(CAN_AppFrame can_buffer[])
{
    delete_repeating_frames(can_buffer);
}

int count_frames(CAN_AppFrame can_buffer[])
{
    int count = 0;

    for (int i = 0; i < CAN_ARRAY_BUFFER_SIZE; i++) {
        if (can_buffer[i].id != 0) {
            count++;
        }
    }

    return count;
}

void delete_repeating_frames(CAN_AppFrame can_buffer[])
{
    int frame_count = count_frames(can_buffer);

    for (int i0 = 0; i0 < frame_count; i0++) {
        for (int i1 = 0; i1 < frame_count; i1++) {
            if (
                can_buffer[i0].id != 0 &&
                can_buffer[i0].id == can_buffer[i1].id &&
                i0 != i1
            ) {
                can_buffer[i1].id = 0;
                can_buffer[i1].dlc = 0;
                memset(can_buffer[i1].data, 0, sizeof(can_buffer[i1].data));
            }
        }
    }
}

int convert_uid_to_gid(int uid)
{
    if ((uid == 0) || (uid == -1)) {
        return -1;
    }

    for (int i = 0; i < g_num_of_nodes; i++) {
        if (g_gids[i][0] == uid) {
            return g_gids[i][1];
        }
    }

    return 0;
}

void print_frames_sorted_by_gid(CAN_AppFrame can_buffer[])
{
    for (int gid_target = 1; gid_target <= g_num_of_nodes; gid_target++) {
        for (int i = 0; i < CAN_ARRAY_BUFFER_SIZE; i++) {
            if (can_buffer[i].id != 0 &&
                convert_uid_to_gid(can_buffer[i].id) == gid_target) {
                interpret_frame(can_buffer[i]);
            }
        }
    }
}

void interpret_frame(CAN_AppFrame incoming)
{
    int uid = incoming.id;
    int gid = convert_uid_to_gid(uid);

    int msgid = incoming.data[0];

    int d2 = incoming.data[1];
    int d3 = incoming.data[2];
    int d4 = incoming.data[3];
    int d5 = incoming.data[4];
    int d6 = incoming.data[5];
    int d7 = incoming.data[6];
    int d8 = incoming.data[7];

    int x, y, z;
    int accel_num;
    float batt;
    float mag;

    if (
        msgid == 11 || msgid == 12 ||
        msgid == 32 || msgid == 33 ||
        msgid == 41 || msgid == 42 ||
        msgid == 51 || msgid == 52
    ) {
        x = compute_axis_debug(d2, d3);
        y = compute_axis_debug(d4, d5);
        z = compute_axis_debug(d6, d7);

        batt = ((float)d8 + 200.0f) / 100.0f;
        mag = compute_magnitude_debug(x, y, z);

        accel_num = get_accel_number_from_msgid(msgid);

        SSM_Printf(
            "UID:%d GID:%d ACCEL:%d MSG:%d X:%d Y:%d Z:%d BATT:%.2fV MAG:%.2f\r\n",
            uid,
            gid,
            accel_num,
            msgid,
            x,
            y,
            z,
            batt,
            mag
        );
    }

    else if (msgid == 22 || msgid == 23 || msgid == 24) {
        int temp_raw;

        temp_raw = compute_axis_debug(
            incoming.data[4],
            incoming.data[3]
        );

        SSM_Printf(
            "UID:%d GID:%d MSG:%d TEMP:%d\r\n",
            uid,
            gid,
            msgid,
            temp_raw
        );
    }

    else if (
        msgid == 10 || msgid == 13 ||
        msgid == 110 || msgid == 111 ||
        msgid == 112 || msgid == 113
    ) {
        int somsr;

        somsr = compute_axis_debug(d2, d3);

        SSM_Printf(
            "UID:%d GID:%d MSG:%d SOMS:%d\r\n",
            uid,
            gid,
            msgid,
            somsr
        );
    }
}

int compute_axis(int low, int high)
{
    if (g_sensor_version < 5) {
        int value = 5000;

        if (!b64) {
            if (high >= 240) {
                high = high - 240;
                value = (low + (high * 256)) - 4095;
            } else {
                value = (low + (high * 256));
            }

            return value;

        } else {
            if (high >= 240) {
                high = high - 240;
            }

            value = (low + (high * 256));
            return value;
        }
    }

    if (g_sensor_version >= 5) {
        if (!b64) {
            int16_t value;
            value = (int16_t)((high << 8) | low);
            return value;
        } else {
            int value;
            value = (high << 8) | low;
            return value;
        }
    }

    return 0;
}

void check_can_status(void)
{
    uint32_t error = HAL_CAN_GetError(&hcan1);

    if (error != HAL_CAN_ERROR_NONE) {
        printf("CAN error: 0x%08lX\r\n", (unsigned long)error);
    }
}

void canSend(int command, int transmit_id)
{
    CAN_TxHeaderTypeDef txHeader;
    uint32_t mailbox;
    uint8_t txData[8] = {0};

    txHeader.StdId = 0;
    txHeader.ExtId = transmit_id;
    txHeader.IDE = CAN_ID_EXT;
    txHeader.RTR = CAN_RTR_DATA;
    txHeader.DLC = 1;
    txHeader.TransmitGlobalTime = DISABLE;

    txData[0] = (uint8_t)command;

    HAL_CAN_AddTxMessage(&hcan1, &txHeader, txData, &mailbox);
}

void poll_command(int command, int uid)
{
    CAN_TxHeaderTypeDef txHeader;
    uint32_t mailbox;
    uint8_t txData[8] = {0};

    txHeader.StdId = 0;
    txHeader.ExtId = 1;
    txHeader.IDE = CAN_ID_EXT;
    txHeader.RTR = CAN_RTR_DATA;
    txHeader.DLC = 3;
    txHeader.TransmitGlobalTime = DISABLE;

    txData[0] = (uint8_t)command;
    txData[1] = (uint8_t)(uid >> 8);
    txData[2] = (uint8_t)(uid & 0xFF);

    HAL_CAN_AddTxMessage(&hcan1, &txHeader, txData, &mailbox);
}

void version_1(int uid)
{
    CAN_TxHeaderTypeDef txHeader;
    uint32_t mailbox;
    uint8_t txData[8] = {0};

    txHeader.StdId = (uint32_t)(uid * 8);
    txHeader.ExtId = 0;
    txHeader.IDE = CAN_ID_STD;
    txHeader.RTR = CAN_RTR_DATA;
    txHeader.DLC = 2;
    txHeader.TransmitGlobalTime = DISABLE;

    txData[0] = 0x00;
    txData[1] = 0x00;

    HAL_CAN_AddTxMessage(&hcan1, &txHeader, txData, &mailbox);

    HAL_Delay(2000);

    HAL_CAN_AddTxMessage(&hcan1, &txHeader, txData, &mailbox);
}
