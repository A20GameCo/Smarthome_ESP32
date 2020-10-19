#pragma once

//Normal Imports
#include "ArduinoJson.h"
#include <memory>
#include "../console_logger.h"
#include "system_storage.h"

// Gadget
#include "sh_gadget.h"

//Lamps
#include "sh_lamp_neopixel.h"
#include "sh_lamp_neopixel_basic.h"
#include "sh_lamp_basic.h"
#include "sh_doorbell_basic.h"

//Fan
#include "sh_fan_westinghouse_ir.h"
#include "sh_lamp_westinghouse_ir.h"

static std::shared_ptr<SH_Gadget> createGadget(GadgetIdentifier gadget_type, pin_set pins, const std::string& name, JsonObject gadget_config, JsonObject code_config) {
  switch (gadget_type) {
    case GadgetIdentifier::sh_lamp_neopixel_basic:
      return std::make_shared<SH_Lamp_NeoPixel_Basic>(gadget_config);

    case GadgetIdentifier::sh_lamp_basic:
      return std::make_shared<SH_Lamp_Basic>(gadget_config);

    case GadgetIdentifier::sh_fan_westinghouse_ir:
      return std::make_shared<SH_Fan_Westinghouse_IR>(gadget_config);

    case GadgetIdentifier::sh_lamp_westinghouse_ir:
      return std::make_shared<SH_Lamp_Westinghouse_IR>(gadget_config);

    default:
      return nullptr;
  }
}

//static SH_Gadget *createGadget(JsonObject gadget_json) {
//  const char *name = gadget_json["name"].as<const char *>();
//  const char *type = gadget_json["type"].as<const char *>();
//
//  SH_Gadget *new_gadget;
//
//  if (name != nullptr && type != nullptr) {
//    logger.print("Adding Gadget '");
//    logger.print(name);
//    logger.println("'");
//    logger.incIndent();
//
//    //sh_neopixel_basic.h
//    if (strcmp(type, "sh_lamp_neopixel_basic") == 0) {
//      new_gadget = new SH_Lamp_NeoPixel_Basic(gadget_json);
//    }
//
//    // sh_lamp_basic.h
//    else if (strcmp(type, "sh_lamp_basic") == 0) {
//      new_gadget = new SH_Lamp_Basic(gadget_json);
//    }
//
//    // sh_fan_westinghouse_ir.h
//    else if (strcmp(type, "sh_fan_westinghouse_ir") == 0) {
//      new_gadget = new SH_Fan_Westinghouse_IR(gadget_json);
//    }
//
//    // sh_lamp_westinghouse_ir.h
//    else if (strcmp(type, "sh_lamp_westinghouse_ir") == 0) {
//      new_gadget = new SH_Lamp_Westinghouse_IR(gadget_json);
//    }
//
//    // sh_fan_basic.h
//    else if (strcmp(type, "sh_doorbell_basic") == 0) {
//      new_gadget = new SH_Doorbell_Basic(gadget_json);
//    }
//
//    //unknown type
//    else {
//      logger.print(LOG_TYPE::ERR, "Cannot find Type '");
//      logger.print(type);
//      logger.println("'");
//      new_gadget = nullptr;
//    }
//    logger.decIndent();
//    return new_gadget;
//  }
//  logger.println(LOG_TYPE::ERR, "Information incomplete.");
//  return nullptr;
//}
