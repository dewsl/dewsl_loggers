#include "b64.h"
#include "sd.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const int validMsgIds[8] = {
    11,12,32,33,41,42,51,52
};

static data_type_params_t struct_dtype;

/* ===================================================== */

bool checkValidMsgId(int msgId)
{
    int size = sizeof(validMsgIds) / sizeof(validMsgIds[0]);

    for (int i = 0; i < size; i++) {
        if (msgId == validMsgIds[i]) {
            return true;
        }
    }

    return false;
}

/* ===================================================== */

void reverse_char_order(char *input, uint8_t length)
{
    char temp[16] = {0};
    char temp1[2] = {0};

    for (int i = length - 1; i >= 0; i--) {
        snprintf(temp1, sizeof(temp1), "%c", input[i]);
        strncat(temp, temp1, sizeof(temp) - strlen(temp) - 1);
    }

    strncpy(input, temp, length);
    input[length] = '\0';
}

/* ===================================================== */

void to_base64(int input, char *dest)
{
    int div_res;
    int mod_res;
    int in = input;

    char temp[2] = {0};

    div_res = in / 64;
    mod_res = in % 64;

    dest[0] = '\0';

    if (input == 0) {
        snprintf(temp, sizeof(temp), "%c", base64[0]);
        strcat(dest, temp);
    }

    while (((mod_res > 0) && (div_res >= 0)) ||
           ((mod_res == 0) && (div_res > 0))) {

        snprintf(temp, sizeof(temp), "%c", base64[mod_res]);
        strcat(dest, temp);

        in = div_res;
        mod_res = in % 64;
        div_res = in / 64;

        if ((mod_res == 0) && (div_res == 0)) {
            break;
        }
    }

    reverse_char_order(dest, strlen(dest));
}

/* ===================================================== */

void pad_b64(
    uint8_t length_of_output,
    char *input,
    char *dest
)
{
    int num_of_pads = 0;

    char temp[16] = {0};

    num_of_pads = length_of_output - strlen(input);

    if (num_of_pads > 0) {
        for (int i = 0; i < num_of_pads; i++) {
            strcat(temp, "A");
        }

        strcat(temp, input);

        strncpy(dest, temp, length_of_output);
        dest[length_of_output] = '\0';
    } else {
        strncpy(dest, input, length_of_output);
        dest[length_of_output] = '\0';
    }
}

/* ===================================================== */

data_type_params_t b64_identify_params(int msgid)
{
    char temp[3];
    char temp1[3];

    memset(&struct_dtype, 0, sizeof(struct_dtype));

    to_base64(msgid, temp);
    pad_b64(2, temp, temp1);

    temp1[2] = '\0';

    strncpy(struct_dtype.identifier, temp1, 2);

    switch (msgid) {

        case 255:
        case 11:
        case 12:
        case 32:
        case 33:
        case 41:
        case 42:
            struct_dtype.type_number = 1;
            struct_dtype.data_length = 9;
            struct_dtype.type_cutoff = 135;
            break;

        case 22:
        case 23:
        case 24:
            struct_dtype.type_number = 3;
            struct_dtype.data_length = 3;
            struct_dtype.type_cutoff = 120;
            break;

        case 10:
        case 13:
        case 110:
        case 111:
        case 112:
        case 113:
            struct_dtype.type_number = 2;
            struct_dtype.data_length = 4;
            struct_dtype.type_cutoff = 120;
            break;

        case 51:
        case 52:
            struct_dtype.type_number = 4;
            struct_dtype.data_length = 12;
            struct_dtype.type_cutoff = 132;
            break;

        default:
            struct_dtype.type_number = 0;
            struct_dtype.data_length = 0;
            struct_dtype.type_cutoff = 0;
            break;
    }

    return struct_dtype;
}

/* ===================================================== */

void b64_write_frame_to_dump(
    CAN_AppFrame incoming,
    char *dump
)
{
    char temp[8] = {0};
    char temp2[8] = {0};

    int gid;
    int msgid;

    int x;
    int y;
    int z;
    int v;

    int somsr;
    int tmp;

    gid = convert_uid_to_gid(incoming.id);

    msgid = incoming.data[0];

    data_type_params_t dtype =
        b64_identify_params(msgid);

    switch (dtype.type_number) {

        case 1:
        {
            x = compute_axis(
                incoming.data[1],
                incoming.data[2]
            );

            y = compute_axis(
                incoming.data[3],
                incoming.data[4]
            );

            z = compute_axis(
                incoming.data[5],
                incoming.data[6]
            );

            v = incoming.data[7];

            snprintf(temp2, sizeof(temp2), "%02X", msgid);
            strncat(dump, temp2, 2);

            to_base64(gid, temp);
            pad_b64(1, temp, temp2);
            strncat(dump, temp2, 1);

            to_base64(x, temp);
            pad_b64(2, temp, temp2);
            strncat(dump, temp2, 2);

            to_base64(y, temp);
            pad_b64(2, temp, temp2);
            strncat(dump, temp2, 2);

            to_base64(z, temp);
            pad_b64(2, temp, temp2);
            strncat(dump, temp2, 2);

            to_base64(v, temp);
            pad_b64(2, temp, temp2);
            strncat(dump, temp2, 2);

            break;
        }

        case 2:
        {
            somsr = compute_axis(
                incoming.data[1],
                incoming.data[2]
            );

            snprintf(temp2, sizeof(temp2), "%02X", msgid);
            strcat(dump, temp2);

            to_base64(gid, temp);
            pad_b64(1, temp, temp2);
            strncat(dump, temp2, 1);

            to_base64(somsr, temp);
            pad_b64(3, temp, temp2);
            strncat(dump, temp2, 3);

            break;
        }

        case 3:
        {
            tmp = compute_axis(
                incoming.data[4],
                incoming.data[3]
            );

            snprintf(temp2, sizeof(temp2), "%02X", msgid);
            strcat(dump, temp2);

            to_base64(gid, temp);
            pad_b64(1, temp, temp2);
            strncat(dump, temp2, 1);

            to_base64(tmp, temp);
            pad_b64(2, temp, temp2);
            strncat(dump, temp2, 2);

            break;
        }

        case 4:
        {
            x = compute_axis(
                incoming.data[1],
                incoming.data[2]
            );

            y = compute_axis(
                incoming.data[3],
                incoming.data[4]
            );

            z = compute_axis(
                incoming.data[5],
                incoming.data[6]
            );

            v = incoming.data[7];

            snprintf(temp2, sizeof(temp2), "%02X", msgid);
            strncat(dump, temp2, 2);

            to_base64(gid, temp);
            pad_b64(1, temp, temp2);
            strncat(dump, temp2, 1);

            to_base64(x, temp);
            pad_b64(3, temp, temp2);
            strncat(dump, temp2, 3);

            to_base64(y, temp);
            pad_b64(3, temp, temp2);
            strncat(dump, temp2, 3);

            to_base64(z, temp);
            pad_b64(3, temp, temp2);
            strncat(dump, temp2, 3);

            to_base64(v, temp);
            pad_b64(2, temp, temp2);
            strncat(dump, temp2, 2);

            break;
        }
    }

    strcat(dump, "-");
}

/* ===================================================== */

char *b64_timestamp(char *timestamp)
{
    static char b64_ts[8];

    char temp[3];
    char temp2[3];

    int value = 0;

    memset(b64_ts, 0, sizeof(b64_ts));

    for (int i = 0; i <= 5; i++) {

        strncpy(temp, timestamp + (i * 2), 2);
        temp[2] = '\0';

        value = atoi(temp);

        to_base64(value, temp);

        pad_b64(1, temp, temp2);

        strcat(b64_ts, temp2);
    }

    return b64_ts;
}

/* ===================================================== */

void b64_process_g_temp_dump(
    char *dump,
    char *final_dump,
    char *no_gids_dump
)
{
    char *token;
    char *last_char;

    char temp_msgid[3];
    char temp_data[16];

    int counter = 0;
    int msgid = 0;
    int dlength = 0;

    (void)no_gids_dump;

    token = strtok(dump, "-");

    while (token != NULL) {

        if (counter == 0) {

            strncpy(temp_msgid, token, 2);
            temp_msgid[2] = '\0';

            msgid = strtol(
                temp_msgid,
                &last_char,
                16
            );

            data_type_params_t dtype =
                b64_identify_params(msgid);

            dlength = dtype.data_length + 2;

            snprintf(temp_data, sizeof(temp_data), "%s", token);

            temp_data[dlength] = '\0';

            strncat(
                final_dump,
                temp_data,
                dlength
            );

            counter = 1;
        }
        else {

            strncpy(
                temp_data,
                token + 2,
                dlength
            );

            temp_data[dlength] = '\0';

            strncat(
                final_dump,
                temp_data,
                dlength - 2
            );
        }

        token = strtok(NULL, "-");
    }

    strncat(final_dump, g_delim, 1);
}

/* ===================================================== */

void b64_build_text_msgs(
    char mode[],
    char *source,
    char *destination
)
{
    char b64_ts[13] = {0};

    int token_length;
    int msgid;
    int cutoff;
    int num_text_per_dtype;
    int name_len;

    int c = 0;
    int char_cnt = 0;
    int num_text_to_send = 0;

    char *token1;
    char *token2;
    char *last_char;

    char temp_msgid[3] = {0};

    char dest[5000] = {0};

    char temp[8];

    char pad[13] = "___________";

    char master_name[9] = "";

    char identifier[3] = {0};

    char timestamp[13] = {0};

    memset(destination, 0, 5000);

    strcpy(b64_ts, b64_timestamp(g_timestamp));

    strcpy(timestamp, g_timestamp);

    token1 = strtok(source, g_delim);

    while (token1 != NULL) {

        token_length = strlen(token1) - 2;

        strncpy(temp_msgid, token1, 2);
        temp_msgid[2] = '\0';

        msgid = strtol(
            temp_msgid,
            &last_char,
            16
        );

        data_type_params_t dtype =
            b64_identify_params(msgid);

        strncpy(identifier, dtype.identifier, 2);

        cutoff = dtype.type_cutoff;

        num_text_per_dtype = (token_length / cutoff);

        if ((token_length % cutoff) != 0) {
            num_text_per_dtype++;
        }

        writeData(timestamp, token1);

        if (g_sensor_version == 1) {
            name_len = 8;

            strncpy(master_name, g_mastername, 4);
            strncat(master_name, "DUE", 4);
        } else {
            name_len = 6;

            strncpy(master_name, g_mastername, 6);
        }

        c = 0;

        for (int i = 0; i < num_text_per_dtype; i++) {

            if (strcmp(comm_mode, "LORA") == 0) {
                strncat(dest, pad, 2);
            } else {
                strncat(dest, pad, 11);
            }

            strncat(dest, master_name, name_len);

            strncat(dest, "*", 1);

            strncat(dest, identifier, 2);

            strncat(dest, "*", 1);

            for (int j = 0; j < cutoff; j++) {

                strncat(dest, token1 + 2, 1);

                c++;
                token1++;

                if (c == token_length) {
                    break;
                }
            }

            if (strcmp(comm_mode, "LORA") == 0) {
                strncat(dest, "*", 1);
                strncat(dest, timestamp, 12);
                strncat(dest, g_delim, 1);
            } else {
                strncat(dest, "<<", 2);
                strncat(dest, g_delim, 1);
            }
        }

        num_text_to_send += num_text_per_dtype;

        token1 = strtok(NULL, g_delim);
    }

    token2 = strtok(dest, g_delim);

    c = 0;

    while (token2 != NULL) {

        c++;

        if (strcmp(comm_mode, "LORA") == 0) {

            snprintf(pad, sizeof(pad), "%02s", ">>");

            strncpy(token2, pad, 2);

            strncat(
                destination,
                token2,
                strlen(token2)
            );

            strncat(destination, g_delim, 1);
        }
        else {

            char_cnt = strlen(token2) + name_len - 24;

            snprintf(pad, sizeof(pad), "%03d", char_cnt);

            strncat(pad, ">>", 2);

            snprintf(temp, sizeof(temp), "%02d/", c);
            strncat(pad, temp, 3);

            snprintf(temp, sizeof(temp), "%02d#", num_text_to_send);
            strncat(pad, temp, 3);

            strncpy(token2, pad, 11);

            strncat(
                destination,
                token2,
                strlen(token2)
            );

            strncat(destination, g_delim, 1);
        }

        token2 = strtok(NULL, g_delim);
    }
}
