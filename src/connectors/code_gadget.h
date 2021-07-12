#pragma once

//#include <Arduino.h>
//#include <ArduinoJson.h>
#include "../console_logger.h"
#include "../system_timer.h"
#include "code_command.h"
#include <memory>

class Code_Gadget {
protected:
  bool code_gadget_is_ready_;

  std::shared_ptr<CodeCommand> com_;

  bool has_news_;

  void setCommand(CodeType, unsigned long);

public:
  Code_Gadget();

  virtual void refresh() = 0;

  bool codeGadgetIsReady() const;

  bool hasNewCommand();

  std::shared_ptr<CodeCommand> getCommand();
};
