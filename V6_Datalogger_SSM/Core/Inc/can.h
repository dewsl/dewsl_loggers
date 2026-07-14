#ifndef CAN_APP_H
#define CAN_APP_H

#include "main.h"
#include <stdint.h>

#define CAN_ARRAY_BUFFER_SIZE 100
#define POLL_TIMEOUT 5000

typedef struct {
    uint32_t id;
    uint8_t dlc;
    uint8_t data[8];
} CAN_AppFrame;

extern CAN_AppFrame g_can_buffer[CAN_ARRAY_BUFFER_SIZE];

void CAN_App_Init(void);

void clear_can_buffer(CAN_AppFrame can_buffer[]);
void get_data(int cmd, int transmit_id, char *final_dump);

int get_all_frames(int timeout_ms, CAN_AppFrame can_buffer[], int expected_frames);
int get_one_frame(int timeout_ms, CAN_AppFrame can_buffer[], int expected_uid);

void process_all_frames(CAN_AppFrame can_buffer[]);
int count_frames(CAN_AppFrame can_buffer[]);
void delete_repeating_frames(CAN_AppFrame can_buffer[]);
void print_frames_sorted_by_gid(CAN_AppFrame can_buffer[]);

int convert_uid_to_gid(int uid);
void interpret_frame(CAN_AppFrame incoming);
int compute_axis(int low, int high);

void check_can_status(void);
void canSend(int command, int transmit_id);
void poll_command(int command, int uid);
void version_1(int uid);

#endif
