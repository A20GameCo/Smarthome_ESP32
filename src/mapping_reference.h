#pragma once

#include <ArduinoJson.h>
#include <cstring>
#include <sstream>
#include "system_settings.h"
#include "console_logger.h"

class Mapping_Reference {
protected:
  uint8_t code_count;
  unsigned long codes[MAPPING_MAX_CODES];
  char command_name[MAPPING_MAX_COMMAND_NAME_LEN];
public:
  Mapping_Reference();

  explicit Mapping_Reference(JsonArray data, const char *name);

  uint8_t getCodeCount() const;

  const char *getName();

  bool containsCode(unsigned long in_code);

  void printMapping();
};
