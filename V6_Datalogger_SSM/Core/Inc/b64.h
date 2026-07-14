#ifndef B64_H
#define B64_H

#include "main.h"
#include "can.h"
#include "config.h"

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    int type_number;
    int data_length;
    int type_cutoff;
    char identifier[3];
} data_type_params_t;

extern const int validMsgIds[8];

bool checkValidMsgId(int msgId);

void reverse_char_order(char *input, uint8_t length);

void to_base64(int input, char *dest);

void pad_b64(
    uint8_t length_of_output,
    char *input,
    char *dest
);

data_type_params_t b64_identify_params(int msgid);

void b64_write_frame_to_dump(
    CAN_AppFrame incoming,
    char *dump
);

char *b64_timestamp(char *timestamp);

void b64_process_g_temp_dump(
    char *dump,
    char *final_dump,
    char *no_gids_dump
);

void b64_build_text_msgs(
    char mode[],
    char *source,
    char *destination
);

#endif
