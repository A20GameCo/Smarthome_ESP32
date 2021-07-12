#include "sh_gadget.h"

#include <utility>

GadgetMethod SH_Gadget::getMethodForCode(unsigned long code) {
  for (auto pair: code_mapping) {
    auto map_method = std::get<0>(pair);
    auto map_list = std::get<1>(pair);
    for (auto map_code: map_list) {
      if (code == map_code) {
        return map_method;
      }
    }
  }
  return GadgetMethod::err_type;
}

bool SH_Gadget::setMethodForCode(GadgetMethod method, unsigned long code) {
  for (auto pair: code_mapping) {
    auto map_method = std::get<0>(pair);
    if (map_method == method) {
      auto map_codes = std::get<1>(pair);
      map_codes.push_back(code);
      return true;
    }
  }
  mapping_pair new_pair = mapping_pair(method, std::vector<unsigned long>(code));
  code_mapping.push_back(new_pair);
  return true;
}

void SH_Gadget::printMapping() {
  logger_i(getName(), "Accessible Methods: %d", code_mapping.size());
  for (auto buf_mapping_pair: code_mapping) {
    auto method = std::get<0>(buf_mapping_pair);
    auto codes = std::get<1>(buf_mapping_pair);
    logger_i(getName(), "'%d':", (uint8_t) method);
    for (auto code: codes) {
      logger_i(getName(), "'%d'", code);
    }
  }
}

void SH_Gadget::updateCharacteristic(CharacteristicIdentifier characteristic, int value) {
  gadget_remote_callback(getName(), characteristic, value);
}


SH_Gadget::SH_Gadget::SH_Gadget(std::string name, const GadgetType type) :
    init_error(false),
    gadget_remote_ready(false),
    event_remote_ready(false),
    name(std::move(name)),
    has_changed(true),
//    main_controller_(nullptr),
    type(type) {}

void SH_Gadget::setGadgetRemoteCallback(std::function<void(std::string, CharacteristicIdentifier, int)> update_method) {
  gadget_remote_callback = std::move(update_method);
  logger_i(getName(), "Initialized Gadget Remote Callback");
  gadget_remote_ready = true;
}

void SH_Gadget::setEventRemoteCallback(std::function<void(std::string, EventType)> send_event) {
  event_remote_callback = std::move(send_event);
  logger_i(getName(), "Initialized Event Remote Callback");
  event_remote_ready = true;
}
//
//void SH_Gadget::setMainController(std::shared_ptr<MainSystemController> controller) {
//  main_controller_ = controller;
//}

GadgetType SH_Gadget::getType() {
  return type;
}

std::string SH_Gadget::getName() const {
  return name;
}

void SH_Gadget::handleCodeUpdate(unsigned long code) {
  GadgetMethod method = getMethodForCode(code);
  if (method != GadgetMethod::err_type) {
    logger_i(getName(), "Applying Method: '%d'", (uint8_t) method);
    handleMethodUpdate(method);
  }
}

void SH_Gadget::updateInitStatus(bool status_update) {
  init_error = init_error && status_update;
}

bool SH_Gadget::hasInitError() const {
  return init_error;
}

bool SH_Gadget::gadgetHasChanged() {
  bool buf = has_changed;
  has_changed = false;
  return buf;
}

void SH_Gadget::setGadgetHasChanged() {
  has_changed = true;
}

//IR Connector

bool SH_Gadget::sendRawIR(const uint16_t raw_data[], const uint8_t content_length) {
  if (hasIR())
    return ir_gadget_->sendRawIR(raw_data, content_length);
  logger_e(getName(), "Cannot send IR: not initialized");
  return false;
}

bool SH_Gadget::sendIR(const unsigned long command, const uint8_t com_type) {
  if (hasIR())
    return ir_gadget_->sendIR(command, com_type);
  logger_e(getName(), "Cannot send IR: not initialized");
  return false;
}

void SH_Gadget::setIR(const std::shared_ptr<IR_Gadget> &new_ir_gadget) {
  ir_gadget_ = new_ir_gadget;
}

bool SH_Gadget::hasIR() const {
  return ir_gadget_ != nullptr;
}

// Radio Connector

void SH_Gadget::setRadio(const std::shared_ptr<Radio_Gadget> &new_radio_gadget) {
  radio_gadget_ = new_radio_gadget;
}

bool SH_Gadget::hasRadio() const {
  return radio_gadget_ != nullptr;
}

void SH_Gadget::handleCharacteristicUpdate(CharacteristicIdentifier characteristic, int value) {
  logger_i(getName(), "Updating Characteristic: %d", int(characteristic));
  executeCharacteristicUpdate(characteristic, value);
}

void SH_Gadget::handleEvent(std::string sender, EventType event_type) {
  //TODO implement event handing
  logger_e(getName(), "Event handling isn't implemented");
}

void SH_Gadget::pauseAllTasks() {
//  if (main_controller_ != nullptr) {
//    main_controller_->pause_all_tasks_except_main();
//  }
}

void SH_Gadget::resumeTasks() {
//  if (main_controller_ != nullptr) {
//    main_controller_->resume_all_tasks();
//  }
}

DynamicJsonDocument SH_Gadget::serialized() {
  DynamicJsonDocument ser_doc(4000);

  ser_doc["type"] = int(type);
  ser_doc["name"] = name;
  ser_doc["characteristics"] = JsonArray();

  int counter = 0;
  for (auto characteristic_data: getCharacteristics()) {
    ser_doc["characteristics"].createNestedObject();
    ser_doc["characteristics"][counter]["type"] = int(characteristic_data.characteristic);
    ser_doc["characteristics"][counter]["max"] = characteristic_data.max;
    ser_doc["characteristics"][counter]["min"] = characteristic_data.min;
    ser_doc["characteristics"][counter]["step"] = characteristic_data.step;
    ser_doc["characteristics"][counter]["value"] = characteristic_data.value;
    counter++;
  }

  return ser_doc;
}
