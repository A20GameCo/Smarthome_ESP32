// Settings
#include "user_settings.h"
#include "system_settings.h"

// Connectors
#include "connectors/ir_gadget.h"
#include "connectors/mqtt_connector.h"
#include "connectors/rest_connector.h"
#include "connectors/serial_gadget.h"
#include "connectors/radio_gadget.h"

// Gadget-Lib
#include "gadgets/gadget_library.h"

// Tools
#include "console_logger.h"

// External Dependencies
#include "Client.h"
#include <WiFi.h>
#include "ArduinoJson.h"
#include <EEPROM.h>

#include "wifi_credentials.h"


static void rebootChip(const char *reason) {
  if (reason != nullptr) {
    logger.print("Rebooting Chip because: '");
    logger.add(reason);
    logger.add("' in ");
  } else {
    logger.println("Rebooting Chip in ");
  }
  for (byte k = 0; k < 5; k++) {
    logger.add(5 - k);
    logger.add(" ");
    delay(1000);
  }
  ESP.restart();
}

static void rebootChip() {
  rebootChip(nullptr);
}

class SH_Main {
private:
  IR_Gadget *ir_gadget;
  MQTT_Gadget *mqtt_gadget;
  REST_Gadget *rest_gadget;
  Serial_Gadget *serial_gadget;
  Radio_Gadget *radio_gadget;

  WiFiClient network_client;

  SH_Gadget *gadgets[MAIN_MAX_GADGETS];

  byte anz_gadgets = 0;

  bool init_gadgets(JsonArray gadget_json) {
    anz_gadgets = gadget_json.size() < MAIN_MAX_GADGETS ? gadget_json.size() : MAIN_MAX_GADGETS;
    logger.print(LOG_INFO, "Creating Gadgets: ");
    logger.addln(anz_gadgets);
    logger.incIntent();
    bool everything_ok = true;
    for (unsigned int pointer = 0; pointer < anz_gadgets; pointer++) {
      JsonObject gadget = gadget_json[pointer].as<JsonObject>();
      SH_Gadget *buffergadget = create_gadget(gadget);
      gadgets[pointer] = buffergadget;
      initGadgetConnectors(buffergadget);
      delay(500);
      everything_ok = everything_ok && gadgets[pointer]->init();
      delay(500);
    }
    logger.decIntent();
    return everything_ok;
  }

  void initGadgetConnectors(SH_Gadget *gadget) {
    logger.incIntent();
    logger.println("Initializing Connectors:");
    logger.incIntent();
    gadget->initConnectors(mqtt_gadget);
    logger.decIntent();
    logger.decIntent();
  }

  bool init_connectors(JsonObject connectors_json) {
    logger.print("Initializing Connectors: ");
    logger.addln(connectors_json.size());
    logger.incIntent();
    if (connectors_json["ir"] != nullptr) {
      ir_gadget = new IR_Gadget(connectors_json["ir"].as<JsonObject>());
    } else {
      ir_gadget = new IR_Gadget();
    }

    if (connectors_json["mqtt"] != nullptr) {
      mqtt_gadget = new MQTT_Gadget(connectors_json["mqtt"].as<JsonObject>(), &network_client);
    } else {
      mqtt_gadget = new MQTT_Gadget();
    }

    if (connectors_json["rest"] != nullptr) {
      rest_gadget = new REST_Gadget(connectors_json["rest"].as<JsonObject>(), &network_client);
    } else {
      rest_gadget = new REST_Gadget();
    }

    if (connectors_json["serial"] != nullptr) {
      serial_gadget = new Serial_Gadget(connectors_json["serial"].as<JsonObject>());
    } else {
      serial_gadget = new Serial_Gadget();
    }
    logger.decIntent();
    return true;
  }

  bool init_network(JsonObject json) {
    if (strcmp(json["type"].as<char *>(), "wifi") == 0) {
      logger.println("Creating Network: WiFi");
      logger.incIntent();
      network_client = WiFiClient();

      const char *ssid = json["config"]["ssid"].as<char *>();
      const char *passwd = json["config"]["password"].as<char *>();
      ssid = WIFI_SSID;
      passwd = WIFI_PW;

      logger.print(LOG_DATA, "");
      logger.add("Connecting to ");
      logger.add(ssid);

      byte connection_tries = 0;

      while (WiFiClass::status() != WL_CONNECTED && connection_tries < 6) {
        WiFi.begin(ssid, passwd);
        delay(1000);
        logger.add(".");
        connection_tries++;
      }
      logger.addln();
      if (WiFiClass::status() != WL_CONNECTED) {
        logger.println(LOG_DATA, "could not establish WiFi Connection...");
      } else {
        randomSeed(micros());
        logger.println(LOG_DATA, "WiFi connected");
        logger.print(LOG_DATA, "IP address: ");
        logger.addln(WiFi.localIP());
      }
    } else {
      logger.println(LOG_ERR, "Unknown Network Settings");
    }
    logger.decIntent();
    return false;
  }

  bool test_initialization() {
    logger.println("Testing initialization:");
    logger.incIntent();

    bool everything_ok = true;

    logger.print(LOG_DATA, "IR: ");
    if (ir_gadget->codeGadgetIsReady()) {
      logger.addln("OK");
    } else {
      logger.addln("ERR");
      everything_ok = false;
    }

    logger.print(LOG_DATA, "MQTT: ");
    if (mqtt_gadget->requestGadgetIsReady()) {
      logger.addln("OK");
    } else {
      logger.addln("ERR");
      everything_ok = false;
    }

    logger.print(LOG_DATA, "Serial: ");
    if (serial_gadget->requestGadgetIsReady()) {
      logger.addln("OK");
    } else {
      logger.addln("ERR");
      everything_ok = false;
    }
    logger.decIntent();
    return everything_ok;
  }

  void test_stuff() {
    logger.println("Testing Stuff");
    logger.incIntent();
    //    rest_gadget->sendRequest(REQ_HTTP_POST, "text/plain", IPAddress(192, 168, 178, 111), 3005, "/irgendein/scheiss",
//                             "pennerus maximus schmongus");
    logger.decIntent();
  }

  void decodeStringCommand(const char *message, unsigned int length) {
    std::string com = message;

    if (com.rfind("_sys:", 0) == 0) {
      logger.print("System Command Detected: ");
      if (com.rfind("_sys:flash", 0) == 0) {
        logger.addln("flash");
//        char input_json[900]{};
      } else if (com.rfind("_sys:reboot", 0) == 0) {
        logger.addln("reboot");
        rebootChip("Input Command");
      } else {
        logger.addln("<unknown>");
      }
    } else if (com.rfind("_dev:", 0) == 0) {
      logger.print("Development Command Detected: ");
      if (com.rfind("_dev:log_on", 4) == 0) {
        logger.addln("log_on");
        logger.activateLogging();
      } else if (com.rfind("_dev:log_off", 4) == 0) {
        logger.addln("log_off");
        logger.deactivateLogging();
      } else {
        logger.addln("<unknown>");
      }
    }
  }

  void refreshCodeConnector(Code_Gadget *gadget) {
    // Refresh Command
    gadget->refresh();

    // Check if Gadgets have new Commands
    if (gadget->hasNewCommand()) {
      unsigned long com = gadget->getCommand();
      logger.print("[HEX] Hex-Com: 0x");
      logger.addln(com, HEX);
      forwardCommand(com);
    }
  }

  void refreshRequestConnector(Request_Gadget *gadget) {
    // Refresh Command
    gadget->refresh();

    // Check if Gadgets have new Commands
    if (gadget->hasRequest()) {
      char type[REQUEST_TYPE_LEN_MAX]{};
      REQUEST_TYPE req_type = gadget->getRequestType();
      const char *req_body = gadget->getRequestBody();
      const char *req_path = gadget->getRequestPath();
      if (req_type == REQ_UNKNOWN)
        strncpy(type, "UWH0T??", REQUEST_TYPE_LEN_MAX);
      else if (req_type == REQ_HTTP_GET)
        strncpy(type, "GET", REQUEST_TYPE_LEN_MAX);
      else if (req_type == REQ_HTTP_POST)
        strncpy(type, "POST", REQUEST_TYPE_LEN_MAX);
      else if (req_type == REQ_HTTP_DELETE)
        strncpy(type, "DELETE", REQUEST_TYPE_LEN_MAX);
      else if (req_type == REQ_HTTP_PUT)
        strncpy(type, "PUT", REQUEST_TYPE_LEN_MAX);
      else if (req_type == REQ_MQTT)
        strncpy(type, "MQTT", REQUEST_TYPE_LEN_MAX);
      else if (req_type == REQ_SERIAL)
        strncpy(type, "Serial", REQUEST_TYPE_LEN_MAX);
      else
        strncpy(type, "{|_(-):(-)_|}", REQUEST_TYPE_LEN_MAX);

      gadget->sendAnswer("bullshit", 200);
      logger.print("[");
      logger.add(type);
      logger.add("] '");
      logger.add(req_path);
      logger.add("' :");
      logger.addln(req_body);
      forwardRequest(req_type, req_path, req_body);
    }
//    if (gadget->hasResponse()) {
//
//      logger.print("[");
//      logger.add(gadget->getResponseStatusCode());
//      logger.add("] '");
//      logger.add(gadget->getResponsePath());
//      logger.add("' :");
//      logger.addln(gadget->getResponseBody());
//    }
  }

  void forwardCommand(unsigned long code) {
    logger.incIntent();
    for (byte c = 0; c < anz_gadgets; c++) {
      gadgets[c]->handleCode(code);
    }
    logger.decIntent();
  }

  void forwardStringRequest(REQUEST_TYPE type, const char *path, const char *body) {
    logger.println("Forwarding as String");
    for (byte c = 0; c < anz_gadgets; c++) {
      gadgets[c]->handleRequest(type, path, body);
    }
  }

  void forwardJsonRequest(REQUEST_TYPE type, const char *path, JsonObject body) {
    logger.println("Forwarding as JSON");
    for (byte c = 0; c < anz_gadgets; c++) {
      gadgets[c]->handleRequest(type, path, body);
    }
  }

  void forwardRequest(REQUEST_TYPE type, const char *path, const char *body) {
    logger.incIntent();
    if (body != nullptr) {
      char first_char = body[0];
      unsigned int last_pos = strlen(body) - 1;
      if (last_pos > 0) {
        char last_char = body[last_pos];
        if (first_char == '{' && last_char == '}') {
          try {
            DynamicJsonDocument json_file(2048);
            deserializeJson(json_file, body);
            JsonObject json_doc = json_file.as<JsonObject>();
            forwardJsonRequest(type, path, json_doc);
          }
          catch (DeserializationError &e) {
            forwardStringRequest(type, path, body);
          }
        } else {
          forwardStringRequest(type, path, body);
        }
      }
    }
    logger.decIntent();
  }

  void refreshConnectors() {
    refreshCodeConnector(serial_gadget);
    refreshCodeConnector(ir_gadget);
//    refreshCodeConnector(radio_gadget);
    refreshRequestConnector(rest_gadget);
    refreshRequestConnector(mqtt_gadget);
    refreshRequestConnector(serial_gadget);
  }

public:
  void init() {

    Serial.begin(115200);

    EEPROM.begin(1023);

    logger.println(LOG_INFO, "Launching...");
    logger.println(LOG_INFO, "Loading Config...");
    logger.incIntent();
    DynamicJsonDocument json_file(2048);
    try {
      deserializeJson(json_file, json_str);
    }
    catch (DeserializationError &e) {
      logger.println(LOG_ERR, "Cannot read JSON, creating blank Config.");
      deserializeJson(json_file, default_config);
    }
    JsonObject json = json_file.as<JsonObject>();

    logger.decIntent();

    init_network(json["network"]);
    init_connectors(json["connectors"]);
    init_gadgets(json["gadgets"]);

    test_initialization();

    test_stuff();
    logger.print("Free Heap:");
    logger.add(ESP.getFreeHeap());
    logger.addln();

    uint16_t rawData[95] = {1264, 418,  1210, 470,  398, 1284,  396, 1266,  420, 1258,  1284, 418,  374, 1308,  372, 1288,  418, 1284,  398, 1282,  1264, 418,  1240, 7154,  1256, 422,  1262, 442,  368, 1292,  392, 1290,  418, 1280,  1238, 444,  372, 1308,  370, 1310,  372, 1306,  422, 1258,  1218, 464,  1210, 7204,  1210, 476,  1232, 442,  424, 1258,  398, 1262,  418, 1280,  1240, 442,  398, 1260,  394, 1288,  416, 1260,  422, 1282,  1238, 442,  1240, 7176,  1208, 470,  1238, 424,  420, 1280,  424, 1236,  392, 1306,  1212, 450,  390, 1312,  370, 1310,  372, 1308,  396, 1264,  1258, 444,  1238};
    ir_gadget->sendRawIR(rawData, 95);
  }

  void refresh() {
    refreshConnectors();

    for (byte c = 0; c < anz_gadgets; c++) {
      gadgets[c]->refresh();
    }
  }
};
