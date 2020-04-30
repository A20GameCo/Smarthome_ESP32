#ifndef __SYSTEM_SETTINGS__
#define __SYSTEM_SETTINGS__

//general
#define EEPROM_SIZE 64
#define SERIAL_SPEED 115200
#define EEPROM_CONFIG_LEN_MAX 2000
#define BASE_TIME 15862

// Client
#define CLIENT_NAME_LEN_MAX 30

// gadget-Command-Mapping
#define MAPPING_MAX_COMMANDS 15
#define MAPPING_MAX_CODES 10
#define MAPPING_MAX_COMMAND_NAME_LEN 25
#define MAIN_MAX_GADGETS 20
#define GADGET_NAME_LEN_MAX 25

// MQTT
#define MQTT_USERNAME_MAX_LEN 40
#define MQTT_PW_MAX_LEN 40

// Reguests
#define REQUEST_PATH_LEN_MAX 40
#define REQUEST_BODY_LEN_MAX 2000
#define REQUEST_TYPE_LEN_MAX 20
#define REQUEST_QUEUE_LEN 5
#define REQUEST_CONTENT_TYPE_LEN_MAX 40

// CodeCommands
#define CODE_BUFFER_SIZE 15
#define CODE_TIME_GAP 150
#define CODE_TYPE_NAME_LEN 30
#define CODE_STR_LEN_MAX 300

// Rest
#define RESPONSE_MAX_WAIT_TIME 1000

// Homebridge
#define HOMEBRIDGE_REGISTER_STR_MAX_LEN 240
#define HOMEBRIDGE_UNREGISTER_STR_MAX_LEN 50
#define HOMEBRIDGE_UPDATE_STR_LEN_MAX 240
#define HOMEBRIDGE_SERVICE_TYPE_LEN_MAX 25

// Remotes
#define REMOTE_MANAGER_MAX_REMOTES 3

// Logger
#define LOGGER_MAX_BUFFER_LEN 4000

const char json_str[] = R"(
{
  "id" : "TestClientESP32",
  "gadgets": [
    {
      "type": "sh_lamp_neopixel_basic",
      "name": "Testlampe NP",
      "lamp_type": 0,
      "pin": 21,
      "length": "1",
      "mapping": {
        "toggleStatus": [
          1,
          16
        ],
        "turnOn": [
          2,
          17
        ],
        "turnOff": [
          3,
          18
        ]
      }
    },
    {
      "type": "sh_lamp_basic",
      "name": "Testlampe 2",
      "lamp_type": 0,
      "pin": 2,
      "default_state": 0,
      "mapping": {
        "toggleStatus": [
          1,
          33464415
        ],
        "turnOn": [
          2,
          33448095
        ],
        "turnOff": [
          3,
          33480735
        ]
      }
    },
    {
      "type": "sh_fan_westinghouse_ir",
      "name": "TestFan",
      "mapping": {
        "toggleStatus": [
          5,
          16
        ],
        "turnOn": [
          6,
          17
        ],
        "turnOff": [
          7,
          18
        ]
      }
    },
    {
      "type": "sh_lamp_westinghouse_ir",
      "name": "Deckenlampe",
      "mapping": {}
    }
  ],
  "network": {
    "type": "wifi",
    "config": {
      "ssid": "testwlan",
      "password": "testpw"
    }
  },
  "connectors": {
    "ir": {
      "recv_pin": 22,
      "send_pin": 23
    },
    "mqtt": {
      "ip": "192.168.178.111",
      "port": "1883"
    },
    "rest": {},
    "serial": {}
  },
  "connector-mapping": {
    "ir": [
      "TestFan",
      "Deckenlampe"
    ],
    "radio": []
  },
  "remotes": {
    "smarthome": [
      "TestFan",
      "Testlampe 2"
    ]
  },
  "code-remote": {
    "ip": "192.168.178.108",
    "port": "3006"
  }
}
)";

const char default_config[] = R"(
{"gadgets": [],"network": {},"connectors": {},"connector-mapping": {},"remote-mapping": {}}
)";

#endif //__SYSTEM_SETTINGS__
