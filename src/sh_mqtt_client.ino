/**
 * Main file for the Smarthome ESP32 project
 */

//region IMPORTS

// Settings
#include "user_settings.h"
#include "system_settings.h"

// Config Writing
#include "config_writing/config_keys.h"

// Network Gadgets
#include "network_library.h"
#include "protocol_paths.h"

// Gadget-Lib
#include "gadgets/gadget_library.h"
#include "gadgets/gadget_characteristic_settings.h"

// Code-Buffer
#include "connectors/code_command_buffer.h"

// Tools
#include "console_logger.h"
#include "system_timer.h"
#include "boot_mode.h"
#include "static_info.h"

// External Dependencies
#include "Client.h"

#include "gadget_collection.h"
#include "storage/static_storage.h"
#include "storage/eeprom_storage.h"
#include "storage/system_storage.h"

#include "pin_profile.h"
#include "color.h"

#include "connectors/mqtt_gadget.h"
#include "connectors/serial_gadget.h"

#include "main_system_controller.h"

// External imports
#include <string>
#include <cstdlib>
#include "ArduinoJson.h"

//endregion

//region STATIC METHODS

static void waitFor(unsigned int delay) {
  unsigned long end_timestamp = millis() + delay;
  while (millis() < end_timestamp) {}
}

/**
 * Reboots the chip and prints out the given message
 * @param reason The reason to print to the terminal
 */
static void rebootChip(const std::string &reason, uint8_t delay = 5) {
  if (!reason.empty()) {
    logger_i("System", "Rebooting Chip because of '%s' in:", reason.c_str());
  } else {
    logger_i("System", "Rebooting Chip in:");
  }
  for (byte k = delay; k > 0; k--) {
    logger_i("System", "%d", k);
    waitFor(1000);
  }
  ESP.restart();
}

/**
 * Method to check if request payload contains all of the selected keys. responds a false ack if any of them misses
 * @param req Request to ckeck payload off
 * @param key_list The list of all the keys that need to be present
 * @return Whether all keys were present
 */
static bool checkPayloadForKeys(std::shared_ptr<Request> req, const std::vector<std::string> &key_list) {
  DynamicJsonDocument json_body = req->getPayload();

  for (const auto &key: key_list) {
    if (!json_body.containsKey(key)) {
      logger_e("System", "'%s' missing in request", key.c_str());
      std::stringstream s_str;
      s_str << "Key missing in payload: '" << key << "'." << std::endl;
      req->respond(false, s_str.str());
      return false;
    }
  }
  return true;
}

//endregion

//region GLOBAL VARIABLES

// Name of the client to be identified in the network
std::string client_id_;

// Runtime id, number generated at startup to identify reboots to network partners
int runtime_id_;

// System config management object
std::shared_ptr<SystemStorage> system_storage_ = nullptr;

// Config used by the system
std::shared_ptr<Config> system_config_ = nullptr;

// Mode how the system should operate
BootMode system_mode_ = BootMode::Unknown_Mode;

// Infrared-gadget receiving and/or sending infrared codes
std::shared_ptr<IR_Gadget> ir_gadget;

// Radio-gadget receiving and/or sending 433mhz-codes
std::shared_ptr<Radio_Gadget> radio_gadget;

// Network-gadget sending and receiving requests via mqtt
std::shared_ptr<RequestGadget> network_gadget;

// Controller for the main system. Given to the gadgets tio access the main system
std::shared_ptr<MainSystemController> main_controller;

// Container to contain all of the gadgets
Gadget_Collection gadgets;

// Container to handle all of the incoming ir/radio codes
CodeCommandBuffer codes;

// Main task, handling all of the gadgets and the main system
TaskHandle_t main_task;

// Network task, receiving and sending requests via the network gadget
TaskHandle_t network_task;

// Heartbeat task, sending a heartbeat request every 5 seconds
TaskHandle_t heartbeat_task;

// Whether gadget updates are locked or not (prevents gadget from sending an
// update to the bridge which it has just received from the bridge)
bool lock_gadget_updates = false;

//endregion

// region CONFIG HELPER METHODS

/**
 * Generates a unique random int
 * @return Random int
 */
static int gen_req_id() {
  return int(millis());
}

// endregion

//region SYNC AND HANDLE GADGETS AND CHARACTERISTICS

/**
 * Prevents gadgets from getting updates
 */
void lockGadgetUpdates() {
  logger_i("System", "Locking Updates");
  lock_gadget_updates = true;
}

/**
 * Lets gadgets receive characteristic updates
 */
void unlockGadgetUpdates() {
  logger_i("System", "Unlocking Updates");
  lock_gadget_updates = false;
}

/**
 * Returns whether gadgets are currently allowed to receive updates
 * @return Whether gadgets are currently allowed to receive updates
 */
bool gadgetUpdatesAreLocked() { return lock_gadget_updates; }

/**
 * Sends a request updating a characteristic on the bridge
 * @param gadget_name Name of the gadget to be updated
 * @param characteristic Characteristic to be updated
 * @param value New value of the characteristic
 */
void updateCharacteristicOnBridge(const std::string &gadget_name, CharacteristicIdentifier characteristic, int value) {
  if (gadgetUpdatesAreLocked()) return;
  auto target_gadget = gadgets.getGadget(gadget_name);

  if (!target_gadget) {
    return;
  }

  DynamicJsonDocument req_doc(2000);

  req_doc["name"] = gadget_name;
  req_doc["type"] = int(target_gadget->getType());
  req_doc["characteristic"] = int(characteristic);
  req_doc["value"] = value;

  auto out_req = std::make_shared<Request>(PATH_CHARACTERISTIC_UPDATE_TO_BRIDGE,
                                           gen_req_id(),
                                           client_id_,
                                           PROTOCOL_BRIDGE_NAME,
                                           req_doc);

  network_gadget->sendRequest(out_req);
}

//endregion

//region SYNC AND HANDLE EVENTS

/**
 * forwards an event to all gadgets
 * @param event
 */
void forwardEvent(const std::shared_ptr<Event> &event) {
  for (int i = 0; i < gadgets.getGadgetCount(); i++) {
    gadgets[i]->handleEvent(event->getSender(), event->getType());
  }
}


void updateEventOnBridge(const std::string &sender, EventType type) {
  if (sender.empty()) {
    logger_i("System", "No sender specified, no event send");
    return;
  }

  auto timestamp = system_timer.getTime();

  auto event_buf = std::make_shared<Event>(sender,
                                           timestamp,
                                           type);

  DynamicJsonDocument req_doc(2000);

  req_doc["name"] = event_buf->getSender();
  req_doc["timestamp"] = event_buf->getTimestamp();
  req_doc["event_type"] = int(event_buf->getType());

  auto out_req = std::make_shared<Request>(PATH_EVENT_UPDATE_TO_BRIDGE,
                                           gen_req_id(),
                                           client_id_,
                                           PROTOCOL_BRIDGE_NAME,
                                           req_doc);

  network_gadget->sendRequest(out_req);

  forwardEvent(event_buf);
}

//endregion

//region SYNC AND HANDLE CODES

void addCodeToBuffer(const std::shared_ptr<CodeCommand> &code) {
  if (codes.addCode(code)) {
    logger_i("System", "Code added to buffer");
    return;
  }
  logger_e("System", "Ignoring: Double Code");
}

void forwardCodeToGadgets(const std::shared_ptr<CodeCommand> &code) {
  logger_e("System", "Forwarding code %d to %d gadgets", code->getCode(), gadgets.getGadgetCount());
  for (int i = 0; i < gadgets.getGadgetCount(); i++) {
    gadgets[i]->handleCodeUpdate(code->getCode());
  }
}

void forwardAllCodes() {
  while (codes.hasNewCode()) {
    forwardCodeToGadgets(codes.getCode());
  }
}

void sendCodeToRemote(const std::shared_ptr<CodeCommand> &code) {
  if (!network_gadget) {
    return;
  }

  if (code->getType() == IR_UNKNOWN_C || code->getType() == UNKNOWN_C) {
    return;
  }

  unsigned long ident = micros();
  DynamicJsonDocument doc(2000);

  doc["request_id"] = ident;
  doc["type"] = int(code->getType());
  doc["code"] = code->getCode();
  doc["timestamp"] = code->getTimestamp();

  network_gadget->sendRequest(std::make_shared<Request>(PATH_CODE_UPDATE_TO_BRIDGE,
                                                        ident,
                                                        client_id_,
                                                        PROTOCOL_BRIDGE_NAME,
                                                        doc));
}

void handleNewCodeFromConnector(const std::shared_ptr<CodeCommand> &code) {
  addCodeToBuffer(code);
  sendCodeToRemote(code);
  forwardAllCodes();
}

void handleNewCodeFromRequest(const std::shared_ptr<CodeCommand> &code) {
  addCodeToBuffer(code);
  forwardAllCodes();
}

//endregion

//region HANDLING OF SPECIFIC REQUESTS

/**
 * Handles a request to receive a characteristic update
 * @param req Request that contains the characteristic update information
 */
void handleGadgetCharacteristicUpdateRequest(std::shared_ptr<Request> req) {

  // Check payload for missing keys
  if (!checkPayloadForKeys(req, {"name", "characteristic", "value"})) {
    return;
  }

  auto req_body = req->getPayload();

  logger_i("System", "Received characteristic update");
  auto target_gadget = gadgets.getGadget(req_body["name"]);
  if (target_gadget != nullptr) {
    auto characteristic = getCharacteristicIdentifierFromInt(req_body["characteristic"].as<int>());
    if (characteristic != CharacteristicIdentifier::err_type) {
      lockGadgetUpdates();
      int value = req_body["value"].as<int>();
      target_gadget->handleCharacteristicUpdate(characteristic, value);
      unlockGadgetUpdates();
    } else {
      logger_e("System", "Illegal err_characteristic 0");
    }
  } else {
    logger_e("System", "Unknown Gadget");
  }
}

/**
 * Handles a request to receive a event from the bridge
 * @param req Request that contains the event information
 */
void handleEventUpdateRequest(std::shared_ptr<Request> req) {

  // Check payload for missing keys
  if (!checkPayloadForKeys(req, {"name", "timestamp", "event_type"})) {
    return;
  }

  auto req_body = req->getPayload();

  logger_i("System", "Received event_type update");
  auto sender = req_body["name"].as<std::string>();
  auto timestamp = req_body["timestamp"].as<unsigned long long>();
  auto type = EventType(req_body["event_type"].as<int>());
  auto event_buf = std::make_shared<Event>(sender, timestamp, type);
  forwardEvent(event_buf);
}

/**
 * Handles a request to receive a code from the bridge
 * @param req Request that contains the code information
 */
void handleCodeUpdateRequest(std::shared_ptr<Request> req) {

  // Check payload for missing keys
  if (!checkPayloadForKeys(req, {"type", "code", "timestamp"})) {
    return;
  }

  auto req_payload = req->getPayload();

  auto newCode = std::make_shared<CodeCommand>(CodeType(req_payload["type"].as<int>()),
                                               req_payload["code"].as<unsigned long>(),
                                               req_payload["timestamp"].as<unsigned long long>());
  handleNewCodeFromRequest(newCode);
}

/**
 * Handles a broadcast request
 * @param req Request that contains the broadcast request information
 */
void handleBroadcastRequest(std::shared_ptr<Request> req) {
  DynamicJsonDocument doc(100);
  doc["runtime_id"] = runtime_id_;
  req->respond("smarthome/broadcast/res", doc);
}

/**
 * Handles a request that contains system control information
 * @param req Request that contains system control information
 */
void handleSystemControlRequest(std::shared_ptr<Request> req) {

  // Check payload for missing keys
  if (!checkPayloadForKeys(req, {"subject"})) {
    return;
  }

  DynamicJsonDocument json_body = req->getPayload();

  auto subject = json_body["subject"].as<std::string>();
  if (subject == "reboot") {
    req->respond(true);
    auto payload = req->getPayload();
    if (payload.containsKey("message")) {
      rebootChip(payload["message"]);
    }
    rebootChip("Network Request");
  }
  req->respond(false);
}

/**
 * Handles a request that contains config reset information
 * @param req Request that contains config reset command information
 */
void handleConfigResetRequest(std::shared_ptr<Request> req) {
  logger_i("System", "Resetting config");
  bool success = system_storage_->eraseConfig();
  req->respond(success);
}

/**
 * Handles a request that contains config write information
 * @param req Request that contains config write information
 */
void handleConfigWriteRequest(std::shared_ptr<Request> req) {
  logger_i("System", "Writing config...");
  DynamicJsonDocument json_body = req->getPayload();

  // Check payload for missing keys
  if (!checkPayloadForKeys(req, {"config"})) {
    logger_e("System", "Keys missing in config write request");
    req->respond(false);
    return;
  }

  auto config = createConfigFromJson(json_body["config"].as<JsonObject>());

  if (config == nullptr) {
    logger_e("System", "Failed to create config object from json");
    req->respond(false);
    return;
  }

  auto status = system_storage_->saveConfig(*config.get());

  if (status) {
    logger_i("System", "Writing config was successful");
  } else {
    logger_e("System", "Failed to save config");
  }

  req->respond(status);
}

/**
 * Handles a request that contains config read information
 * @param req Request that contains config read information
 */
void handleConfigReadRequest(std::shared_ptr<Request> req) {
  logger_i("System", "Reading from config...");
  // Check payload for missing keys
  if (!checkPayloadForKeys(req, {"param"})) {
    return;
  }

  DynamicJsonDocument json_body = req->getPayload();

  auto param_name = json_body["param"].as<std::string>();
  bool read_successful = false;
  std::string read_val_str;
  uint8_t read_val_uint;

  logger_i("System", "Read param '%s'", param_name.c_str());

  // read wifi ssid
  if (param_name == "wifi_ssid") {
    auto read_val = system_config_->getWifiSSID();
    if (read_val != nullptr) {
      read_successful = true;
      read_val_str = *read_val;
    }
  }

  // read mqtt ip
  if (param_name == "mqtt_ip") {
    auto read_val = system_config_->getMqttIP();
    if (read_val != nullptr) {
      read_successful = true;
      read_val_str = *read_val;
    }
  }

  // read mqtt port
  if (param_name == "mqtt_port") {
    auto read_val = system_config_->getMqttPort();
    if (read_val != nullptr) {
      read_successful = true;
      std::stringstream s_str;
      s_str << *read_val;
      read_val_str = s_str.str();
    }
  }

  // read mqtt username
  if (param_name == "mqtt_user") {
    auto read_val = system_config_->getMqttUsername();
    if (read_val != nullptr) {
      read_successful = true;
      read_val_str = *read_val;
    }
  }

  // send response if read was successful
  if (read_successful) {
    DynamicJsonDocument doc(100);
    doc["value"] = read_val_str;
    req->respond(doc);
    return;
  }

  // read irrecv pin
  if (param_name == "irrecv_pin") {
    read_successful = true;
    read_val_uint = system_config_->getIRRecvPin();
  }

    // read irsend pin
  else if (param_name == "irsend_pin") {
    read_successful = true;
    read_val_uint = system_config_->getIRSendPin();
  }

    // read radio recv pin
  else if (param_name == "radio_recv_pin") {
    read_successful = true;
    read_val_uint = system_config_->getRadioRecvPin();
  }

    // read radio send pin
  else if (param_name == "radio_send_pin") {
    read_successful = true;
    read_val_uint = system_config_->getRadioSendPin();
  }

    // read network mode
  else if (param_name == "network_mode") {
    auto read_val = system_config_->getNetworkMode();
    read_successful = true;
    read_val_uint = (uint8_t) read_val;
  }

  // send response if read was successful
  if (read_successful) {
    DynamicJsonDocument doc(100);
    doc["value"] = read_val_uint;
    req->respond(doc);
    return;
  }
}

/**
 * Handles a request to sync settings between client and bridge
 * @param req Request that contains the sync information
 */
void handleSyncRequest(std::shared_ptr<Request> req) {

  auto req_payload = req->getPayload();

  // Get time sync data from request
  if (req_payload.containsKey("server_time")) {
    auto buf_time = req_payload["server_time"].as<unsigned long long int>();
    system_timer.setTime(buf_time, 0);
  } else {
    logger_e("System", "Have not received server sync time");
  }

  DynamicJsonDocument data_json(4500);

  // Add runtime id
  data_json["runtime_id"] = runtime_id_;

  // Add boot mode
  data_json["boot_mode"] = int(system_mode_);

  // Add software flash date
  auto sw_flashed = getSoftwareFlashDate();
  if (sw_flashed != SW_DATA_DEFAULT) {
    data_json["sw_uploaded"] = sw_flashed;
  } else {
    data_json["sw_uploaded"] = (char *) nullptr;
  }

  // Add software branch name
  auto git_branch = getSoftwareGitBranch();
  if (git_branch != SW_DATA_DEFAULT) {
    data_json["sw_branch"] = git_branch;
  } else {
    data_json["sw_branch"] = (char *) nullptr;
  }

  // Add software commit hash
  auto git_commit = getSoftwareGitCommit();
  if (git_commit != SW_DATA_DEFAULT) {
    data_json["sw_commit"] = git_commit;
  } else {
    data_json["sw_commit"] = (char *) nullptr;
  }

  // Add port mapping
  data_json["port_mapping"] = getPortMapping();

  // Add gadgets
  JsonArray json_gadgets = data_json.createNestedArray("gadgets");
  for (int i = 0; i < gadgets.getGadgetCount(); i++) {
    auto characteristic_data = gadgets[i]->serialized();
    json_gadgets[i] = characteristic_data;
  }

  req->respond(data_json);
}

//endregion

//region SORTING OF REQUESTS

void handleSystemRequest(std::shared_ptr<Request> req) {

  DynamicJsonDocument json_body = req->getPayload();

  logger_i("System", "System command received");

  // React to broadcast
  if (req->getPath() == PATH_BROADCAST) {
    handleBroadcastRequest(req);
    return;
  }

  // All directed Requests
  logger_i("System", "Directed Request");

  // system commands
  if (req->getPath() == PATH_SYSTEM_CONTROL) {
    handleSystemControlRequest(req);
    return;
  }

  // Reset config
  if (req->getPath() == PATH_CONFIG_RESET) {
    handleConfigResetRequest(req);
    return;
  }

  // Write parameters
  if (req->getPath() == PATH_CONFIG_WRITE) {
    handleConfigWriteRequest(req);
    return;
  }

  // Read parameters
  if (req->getPath() == PATH_CONFIG_READ) {
    handleConfigReadRequest(req);
    return;
  }

  req->respond(false);
}

/**
 * Handles a request gotten from the network gadget
 * @param req Request to handle
 */
void handleRequest(std::shared_ptr<Request> req) {
  std::string req_path = req->getPath();
  if (!req->hasReceiver()) {
    req->updateReceiver(client_id_);
    for (const auto &list_path: broadcast_request_paths) {
      if (req_path == list_path) {
        // Handle broadcasts which do not have an receiver
        handleSystemRequest(req);
        return;
      }
    }
  } else if (client_id_ != req->getReceiver()) {
    // Return if the client is not the receiver of the message
    return;
  }

  // Check if the received request is a system request
  for (const auto &list_path: system_request_paths) {
    // TODO: check if last character is # and then only check 0 - (n-2)
    if (req_path.compare(0, list_path.length(), list_path) == 0) {
      handleSystemRequest(req);
      return;
    }
  }

  // For testing purposes
  if (req->getPath() == PATH_ECHO_TEST) {
    DynamicJsonDocument request = req->getPayload();
    req->respond(req->getPath(), request);
    return;
  }

  // Checks for characteristic update request
  if (req->getPath() == PATH_CHARACTERISTIC_UPDATE_FROM_BRIDGE) {
    handleGadgetCharacteristicUpdateRequest(req);
    return;
  }

  // Check for request containing new event
  if (req->getPath() == PATH_EVENT_UPDATE_FROM_BRIDGE) {
    handleEventUpdateRequest(req);
    return;
  }

  if (req->getPath() == PATH_CODE_UPDATE_FROM_BRIDGE) {
    handleCodeUpdateRequest(req);
    return;
  }

  if (req->getPath() == PATH_SYNC) {
    handleSyncRequest(req);
    return;
  }

  logger_e("System", "Received request to unhandled path");
}

//endregion

//region INITIALIZATION METHODS

/**
 * Initialized all of the gadgets stored in the EEPROM
 * @return Whether initializing all gadgets was successful or not
 */
bool initGadgets() {
  auto eeprom_gadgets = system_config_->getGadgets();

  logger_i("System", "Initializing Gadgets: %d", eeprom_gadgets.size());

  for (auto gadget: eeprom_gadgets) {
    auto gadget_ident = (GadgetIdentifier) std::get<0>(gadget);
    auto remote_bf = std::get<1>(gadget);
    auto ports = std::get<2>(gadget);
    auto name = std::get<3>(gadget);
    auto gadget_config_str = std::get<4>(gadget);
    auto code_config_str = std::get<5>(gadget);

    logger_i("System", "Initializing %s", name.c_str());

    // Translate stored ports to actual pins
    pin_set pins;
    for (int i = 0; i < pins.size(); i++) {
      uint8_t buf_port = ports[i];
      uint8_t pin = 0;
      if (buf_port != 0) {
        pin = getPinForPort(buf_port);
      }
      pins[i] = pin;
    }

    DynamicJsonDocument gadget_config(2500);
    DynamicJsonDocument code_config(2500);

    bool deserialization_ok = true;

    DeserializationError err;

    if (!gadget_config_str.empty()) {
      err = deserializeJson(gadget_config, gadget_config_str);
      if (err != DeserializationError::Ok) {
        deserialization_ok = false;
      }
    }

    if (!gadget_config_str.empty()) {
      err = deserializeJson(code_config, code_config_str);
      if (err != DeserializationError::Ok) {
        deserialization_ok = false;
      }
    }

    if (deserialization_ok) {
      logger_i("System", "Creating Gadget");
      auto buf_gadget = createGadget(gadget_ident, pins, name, gadget_config.as<JsonObject>());

      if (buf_gadget != nullptr) {
        // Gadget Remote
        if (remote_bf[0]) {
          logger_i("System", "Linking Gadget Remote");

          using std::placeholders::_1;
          using std::placeholders::_2;
          using std::placeholders::_3;

          buf_gadget->setGadgetRemoteCallback(std::bind(&updateCharacteristicOnBridge, _1, _2, _3));
          buf_gadget->setEventRemoteCallback(std::bind(&updateEventOnBridge, _1, _2));
          buf_gadget->setMainController(main_controller);
        }

        // Code Remote
        if (remote_bf[1]) {
          logger_i("System", "Linking Code Remote");

          for (int method_index = 0; method_index < GadgetMethodCount; method_index++) {
            std::stringstream ss;
            ss << method_index;
            std::string method_str = ss.str();
            if (code_config.containsKey(method_str)) {
              JsonArray code_arr = code_config[method_str].as<JsonArray>();
              for (int i = 0; i < code_arr.size(); i++) {
                unsigned long code = code_arr[i].as<unsigned long>();
                buf_gadget->setMethodForCode((GadgetMethod) method_index, code);
              }
            }
          }

          buf_gadget->printMapping();
        }

        // Event Remote
        if (remote_bf[2]) {
          logger_i("System", "Linking Gadget Remote");
          // TODO: init event remote on gadgets
          logger_e("System", "Not Implemented");
        }

        // IR Gadget
        bool ir_ok = true;
        if (gadgetRequiresIR(gadget_ident)) {
          if (ir_gadget != nullptr) {
            logger_i("System", "Linking IR gadget");
            buf_gadget->setIR(ir_gadget);
          } else {
            logger_e("System", "No IR gadget available");
            ir_ok = false;
          }
        } else {
          logger_i("System", "No IR required");
        }

        // Radio
        // TODO: check when radio is implemented
        bool radio_ok = true;
        if (gadgetRequiresRadio(gadget_ident)) {
          if (radio_gadget != nullptr) {
            logger_i("System", "Linking radio gadget");
            buf_gadget->setRadio(radio_gadget);
          } else {
            logger_i("System", "No radio gadget available");
            radio_ok = false;
          }
        } else {
          logger_i("System", "No radio required");
        }

        // Add created gadget to the list
        if (ir_ok && radio_ok) {
          gadgets.addGadget(buf_gadget);
        } else {
          logger_i("System", "Gadget initialization failed due to ir/radio problems");
        }
      } else {
        logger_e("System", "Gadget could not be created");
      }
    } else {
      logger_e("System", "Error in config deserialization process");
    }
  }
  return true;
}

/**
 * Initializes all Connectors (IR/Radio)
 * @return Whether initializing connectors was successful or not
 */
bool initConnectors() {
  logger_i("System", "Initializing Connectors:");

  uint8_t ir_recv = system_config_->getIRRecvPin();
  uint8_t ir_send = system_config_->getIRSendPin();
  uint8_t radio_recv = system_config_->getRadioRecvPin();
  uint8_t radio_send = system_config_->getRadioSendPin();

  logger_i("System", "Creating IR-Gadget:");

  if (ir_recv || ir_send) {
    ir_gadget = std::make_shared<IR_Gadget>(ir_recv, ir_send);
  } else {
    logger_i("System", "No IR Configured");
    ir_gadget = nullptr;
  }

  logger_i("System", "Creating Radio-Gadget:");

  if (radio_recv || radio_send) {
    logger_i("System", "Radio Configured bot not implemented");
  } else {
    logger_i("System", "No Radio Configured");
    radio_gadget = nullptr;
  }

  return true;
}

/**
 * Initializes the network gadget
 * @param mode The mode the chip should start at
 * @return Whether initializing network was successful or not
 */
bool initNetwork(NetworkMode mode) {
  if (mode == NetworkMode::None) {
    logger_e("System", "No network configured.");
    return false;
  }

  // initialize Network
  if (mode == NetworkMode::MQTT) {

    if (system_config_->getWifiSSID() == nullptr) {
      logger_e("System", "Config has no valid wifi ssid");
      return false;
    }

    if (system_config_->getWifiPW() == nullptr) {
      logger_e("System", "Config has no valid wifi password");
      return false;
    }

    if (system_config_->getMqttIP() == nullptr) {
      logger_e("System", "Config has no valid mqtt ip");
      return false;
    }

    if (system_config_->getMqttPort() == nullptr) {
      logger_e("System", "Config has no valid mqtt port");
      return false;
    }

    std::string ssid = *system_config_->getWifiSSID();
    std::string wifi_pw = *system_config_->getWifiPW();
    uint16_t port = *system_config_->getMqttPort();
    IPAddress ip = *system_config_->getMqttIP();

    if (!system_config_->getMqttUsername() || system_config_->getMqttPassword()) {
      logger_i("System", "Establishing MQTT connection without credentials");
      network_gadget = std::make_shared<MQTTGadget>(client_id_,
                                                    ssid,
                                                    wifi_pw,
                                                    ip,
                                                    port);
    } else {
      logger_i("System", "Establishing MQTT connection using credentials");
      std::string user = *system_config_->getMqttUsername();
      std::string mqtt_pw = *system_config_->getMqttPassword();

      network_gadget = std::make_shared<MQTTGadget>(client_id_,
                                                    ssid,
                                                    wifi_pw,
                                                    ip,
                                                    port,
                                                    user,
                                                    mqtt_pw);
    }

  } else if (mode == NetworkMode::Serial) {
    network_gadget = std::make_shared<SerialGadget>();
  } else {
    logger_e("System", "Unknown Network Settings");
    return false;
  }
  return true;
}

/**
 * Initializes the chip in network only mode using the serial network
 */
void initModeSerial() {
  bool status = initNetwork(NetworkMode::Serial);
  if (status) {
    logger_i("System", "Serial network initialized");
  } else {
    logger_e("System", "serial network was not initialized");
  }
}

/**
 * Initializes the chip in network only mode using the network mode loaded from eeprom
 */
void initModeNetwork() {
  auto mode = system_config_->getNetworkMode();
  auto status = initNetwork(mode);

  if (!status) {
    rebootChip("Network initialization failed.", 15);
  }
}

/**
 * Initializes the chip with all gadgets and connectors loaded
 */
void initModeComplete() {
  auto mode = system_config_->getNetworkMode();
  auto status = initNetwork(mode);

  if (!status) {
    rebootChip("Network initialization failed.", 15);
  }

  initConnectors();

  initGadgets();
}

//endregion

//region REFRESHERS

void handleCodeConnector(const std::shared_ptr<Code_Gadget> &gadget) {
  if (gadget == nullptr) {
    return;
  }
  if (gadget->hasNewCommand()) {
    auto com = gadget->getCommand();
    logger_i("System", "Command: %d", com->getCode());

    handleNewCodeFromConnector(com);
  }
}

/**
 * Gets new requests from the network gadget.
 */
void handleNetwork() {
  if (network_gadget == nullptr) {
    return;
  }
  if (network_gadget->hasRequest()) {
    std::string type;
    std::shared_ptr<Request> req = network_gadget->getRequest();
    RequestGadgetType g_type = network_gadget->getGadgetType();
    if (g_type == RequestGadgetType::MQTT_G)
      type = "MQTT";
    else if (g_type == RequestGadgetType::SERIAL_G)
      type = "Serial";
    else
      type = "<o.O>";

    std::string r_body = req->getBody();
    if (r_body.length() > 400) {
      r_body = "[Body too long]";
    }
    logger_i("System", "[%s] '%s': %s", type.c_str(), req->getPath().c_str(), r_body.c_str());
    handleRequest(req);
  }
}

/**
 * The refresh method that is supposed to be performed in serial only mode
 */
void refreshModeSerial() {
  handleNetwork();
}

/**
 * The refresh method that is supposed to be performed in network only mode
 */
void refreshModeNetwork() {
  handleNetwork();
}

/**
 * The refresh method that is supposed to be performed in full operation mode
 */
void refreshModeComplete() {

  handleNetwork();

  ir_gadget->refresh();
  handleCodeConnector(ir_gadget);

  for (byte c = 0; c < gadgets.getGadgetCount(); c++) {
    // TODO
//    gadgets.getGadget(c)->refresh();
  }
}

/**
 * Refresh method for the main task
 */
void refresh() {
  switch (system_mode_) {
    case BootMode::Serial_Ony:
      refreshModeSerial();
      break;
    case BootMode::Network_Only_EEPROM:
      refreshModeNetwork();
      break;
    case BootMode::Full_Operation:
      refreshModeComplete();
      break;
    default:
      delay(1000);
      break;
  }
}

/**
 * Refresh method for network operation
 */
void refreshNetwork() {
  if (network_gadget == nullptr) {
    return;
  }
  network_gadget->refresh();
}

void sendHeartbeat() {
  if (network_gadget != nullptr) {
    DynamicJsonDocument req_doc(100);

    // Just call the method periodically to detect time rollovers
    system_timer.getTime();

    req_doc["runtime_id"] = runtime_id_;

    // NOT PART OF THE PROTOCOL, debugging purposes only
    req_doc["system_time"] = system_timer.getTime();

    auto heartbeat_request = std::make_shared<Request>(PATH_HEARTBEAT,
                                                       gen_req_id(),
                                                       client_id_,
                                                       PROTOCOL_BRIDGE_NAME,
                                                       req_doc);
    network_gadget->sendRequest(heartbeat_request);
  }
}

//endregion

//region TASKS

/**
 * Function for the main task refresing the main content
 * @param args Unused
 */
[[noreturn]] static void mainTask(void *args) {
  while (true) {
    refresh();
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

/**
 * Function for the network tasks receiving and sending requests
 * @param args Unused
 */
[[noreturn]] static void networkTask(void *args) {
  while (true) {
    refreshNetwork();
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

/**
 * Function for the network tasks receiving and sending requests
 * @param args Unused
 */
[[noreturn]] static void heartbeatTask(void *args) {
  while (true) {
    sendHeartbeat();
    vTaskDelay(5000 / portTICK_PERIOD_MS);
  }
}

/**
 * Creates and starts the tasks used by the system
 */
static void createTasks() {
//  xTaskCreatePinnedToCore(
//      mainTask,                          Task function.
//      "Smarthome_Main",          String with name of task.
//      10000,                 Stack size in words.
//      NULL,                  Parameter passed as input of the task
//      1,                        Priority of the task.
//      &main_task,                        Task handle.
//      0);                        Core to run on

  xTaskCreatePinnedToCore(
      networkTask,                      /* Task function. */
      "Smarthome_MQTT",         /* String with name of task. */
      10000,                /* Stack size in words. */
      NULL,                 /* Parameter passed as input of the task */
      1,                       /* Priority of the task. */
      &network_task,                    /* Task handle. */
      1);                       /* Core to run on */

  xTaskCreatePinnedToCore(
      heartbeatTask,                    /* Task function. */
      "Smarthome_Heartbeat",    /* String with name of task. */
      10000,                /* Stack size in words. */
      NULL,                 /* Parameter passed as input of the task */
      1,                       /* Priority of the task. */
      &heartbeat_task,                  /* Task handle. */
      1);                       /* Core to run on */
}

//endregion

//region MAIN FUNCTIONS

/**
 * Test-Fuction for debugging
 */
void testStuff() {
  logger_i("System", "Testing Stuff");
}

/**
 * Setup-method that is automatically called once on launch
 */
void setup() {
  Serial.begin(SERIAL_SPEED);
  logger_i("System", "Launching...");

  runtime_id_ = int(random(10000));
  logger_i("System", "Runtime ID: %d", runtime_id_);

  logger_i("System", "Software Info:");
  logger_i("System", "Flash Date: %s", getSoftwareFlashDate().c_str());
  logger_i("System", "Git Branch: %s", getSoftwareGitBranch().c_str());
  logger_i("System", "Git Commit: %s", getSoftwareGitCommit().c_str());

  logger_i("System", "Initializing Storage:");

  auto yolo = std::make_shared<StaticStorage>();

  if (StaticStorage::staticConfigStringAvailable()) {
    logger_i("System", "Using static, pre-compiled config");
    system_storage_ = std::make_shared<StaticStorage>();
  } else {
    logger_i("System", "Using dynamic, EEPROM config");
    system_storage_ = std::make_shared<EepromStorage>();
  }

  if (!system_storage_->isInitialized()) {
    rebootChip("System storage initialization error", 15);
  }

  system_config_ = system_storage_->loadConfig();
  if (system_config_ == nullptr) {
    logger_e("System", "Failed to load system configuration data");
    rebootChip("Config loading error", 15);
  }
  logger_i("System", "Config loaded successfully");

  main_controller = std::make_shared<MainSystemController>(network_task, heartbeat_task);

  client_id_ = system_config_->getID();
  logger_i("System", "Client ID: '%s'", client_id_.c_str());

  testStuff();

  logger_i("System", "Boot Mode:");
  system_mode_ = getBootMode();

  switch (system_mode_) {
    case BootMode::Serial_Ony:
      logger_i("System", "Boot Mode: Serial Only");
      initModeSerial();
      break;
    case BootMode::Network_Only_EEPROM:
      logger_i("System", " Boot Mode: Network Only/EEPROM");
      initModeNetwork();
      break;
    case BootMode::Full_Operation:
      logger_i("System", "Boot Mode: Full Operation");
      initModeComplete();
      break;
    default:
      logger_i("System", "Unknown Boot Mode");
      break;
  }

  logger_i("System", "Free Heap: %d", ESP.getFreeHeap());

  createTasks();
}

/**
 * Loop-Method that is called forever while chip is running.
 * Used for the heartbeat sending.
 */
void loop() {
  refresh();
}

//endregion
