#ifndef SSM_CONFIG_H
#define SSM_CONFIG_H

#include <stdint.h>

#define LIB_MASTERNAME_SIZE      6
#define LIB_COLUMN_IDS_SIZE      600

typedef struct {
    char lib_mastername[LIB_MASTERNAME_SIZE];
    char lib_column_ids[LIB_COLUMN_IDS_SIZE];
    uint8_t lib_num_of_nodes;
    uint8_t lib_sensor_version;
    uint8_t lib_datalogger_version;
    uint8_t lib_has_piezo;
    uint16_t lib_turn_on_delay;
    int lib_broad_timeout;
    int lib_sampling_max_retry;
    uint8_t lib_b64;
} libConfig;

extern const int lib_LOGGER_COUNT;
extern const libConfig config_container[];

#endif
