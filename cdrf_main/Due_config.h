/*
  Due_config.h - Library of configurations
  2022.12.16
*/

#ifndef Due_config_h
#define Due_config_h

#include "Arduino.h"

struct lib_config {
  char* lib_mastername;
  char* lib_column_ids;
  uint8_t lib_num_of_nodes;
  uint8_t lib_sensor_version;
  uint8_t lib_datalogger_version;
  uint8_t lib_has_piezo;
  uint16_t lib_turn_on_delay;
  uint16_t lib_broad_timeout;
  uint8_t lib_sampling_max_retry;
  uint8_t lib_b64;
};

extern struct lib_config config_container[];
extern uint16_t lib_LOGGER_COUNT;
#endif