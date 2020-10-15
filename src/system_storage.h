#pragma once

#include <ArduinoJson.h>
#include <EEPROM.h>
#include <sstream>
#include <utility>
#include <cmath>
#include <string>
#include "system_settings.h"
#include "console_logger.h"

#include "network_library.h"
#include "remote_library.h"

// valid config bitfield
#define VALID_CONFIG_BITFIELD_BYTE 0

#define CONFIG_CHECK_INDEX_ID 0
#define CONFIG_CHECK_INDEX_WIFI_SSID 1
#define CONFIG_CHECK_INDEX_WIFI_PW 2
#define CONFIG_CHECK_INDEX_MQTT_IP 3
#define CONFIG_CHECK_INDEX_MQTT_PORT 4
#define CONFIG_CHECK_INDEX_MQTT_USER 5
#define CONFIG_CHECK_INDEX_MQTT_PW 6

// valid gadget storage bitfield
#define GADGET_COUNT_POS 1

// valid remote storage bitfield
#define SYSTEM_SETTINGS_BITFIELD_BYTE 2

// ir
#define IR_RECV_PIN_POS 3
#define IR_SEND_PIN_POS 4

// radio
#define RADIO_RECV_POS 5
#define RADIO_SEND_POS 6

// network mode
#define NETWORK_MODE_POS 7

// remotes
#define GADGET_REMOTE_POS 8
#define CODE_REMOTE_POS GADGET_REMOTE_POS + 1
#define EVENT_REMOTE_POS CODE_REMOTE_POS + 1

// id
#define ID_POS 15
#define ID_MAX_LEN 20

//wifi_ssid
#define WIFI_SSID_POS (ID_POS + ID_MAX_LEN + 1)
#define WIFI_SSID_MAX_LEN 25

//wifi_pw
#define WIFI_PW_POS (WIFI_SSID_POS + WIFI_SSID_MAX_LEN + 1)
#define WIFI_PW_MAX_LEN 50

//wifi_pw
#define MQTT_IP_POS (WIFI_PW_POS + WIFI_PW_MAX_LEN + 1)
#define MQTT_IP_MAX_LEN 4

//mqtt port
#define MQTT_PORT_POS (MQTT_IP_POS + MQTT_IP_MAX_LEN + 1)
#define MQTT_PORT_MAX_LEN 2

//mqtt username
#define MQTT_USER_POS (MQTT_PORT_POS + MQTT_PORT_MAX_LEN + 1)
#define MQTT_USER_MAX_LEN 50

//mqtt password
#define MQTT_PW_POS (MQTT_USER_POS + MQTT_USER_MAX_LEN + 1)
#define MQTT_PW_MAX_LEN 50

//gadgets
#define GADGET_POS_START (MQTT_PW_POS + MQTT_PW_MAX_LEN + 1)
#define GADGET_MAX_COUNT 8
#define GADGET_BLOCK_START (GADGET_POS_START + ((GADGET_MAX_COUNT + 1) * 2))

using gadget_tuple = std::tuple<uint8_t, uint8_t, std::string, std::string>;

static bool validateJson(const char *new_json_str) {
  DynamicJsonDocument json_file(2048);
  DeserializationError err = deserializeJson(json_file, new_json_str);
  return err == DeserializationError::Ok;
}

class System_Storage {
private:

  /**
   * Method takes a bitfield, changes the selected bit to the passed value and returns the new bitfield
   * @param index the bit to be written (0-7)
   * @param new_value the value to be written
   * @param old_flag the bitfield to be changed
   * @return
   */
  static uint8_t calculateNewContentFlag(uint8_t index, bool new_value, uint8_t old_flag) {
    auto content_flag = old_flag;
    auto mask = (unsigned int) pow(2, index);
    if (new_value) {
      content_flag = content_flag | mask;
    } else {
      mask = 0xFF - mask;
      content_flag = content_flag & mask;
    }
    return content_flag;
  }

  /**
   * Sets the bit of a bitfield to the selected value
   * @param bitfield_address the eeprom-address the bitfield is stored in
   * @param index the bit to be written (0-7)
   * @param value the value to be written
   */
  static void setFlag(int bitfield_address, uint8_t index, bool value) {
    uint8_t content_flag = EEPROM.readByte(bitfield_address);
    content_flag = calculateNewContentFlag(index, value, content_flag);
    EEPROM.writeByte(bitfield_address, content_flag);
    EEPROM.commit();
  }

  /**
   * Reads the value stored at the selected index of a bitfield
   * @param bitfield_address the eeprom-address the bitfield is stored in
   * @param index the bit to be read (0-7)
   * @return the value of the read bit
   */
  static bool getFlag(int bitfield_address, uint8_t index) {
    uint8_t content_flag = EEPROM.readByte(bitfield_address);
    auto mask = (unsigned int) pow(2, index);
    uint8_t content_info = content_flag & mask;
    return content_info != 0;
  }

  /**
   * Sets the bit at the content bitfield to the selected value
   * @param index the index the byte is located at
   * @param value the value to set it to
   */
  static void setContentFlag(uint8_t index, bool value) {
    setFlag(VALID_CONFIG_BITFIELD_BYTE, index, value);
  }

  /**
   * Returns the bit in the content bitfield at the selected index
   * @param index the index the bit is located at
   * @return the value of the bit at the selected index
   */
  static bool getContentFlag(uint8_t index) {
    return getFlag(VALID_CONFIG_BITFIELD_BYTE, index);
  }

  /**
   * Writes a uint8_t (one byte) to the eeprom
   * @param pos position to write to
   * @param value value to be written
   * @return whether writing was successful
   */
  static bool writeByte(int pos, uint8_t value) {
    EEPROM.writeByte(pos, value);
    EEPROM.commit();
    return true;
  }

  /**
   * Reads a uint8_t (one byte) from the selected part of the memory
   * @param pos position to read from
   * @return the uint16_t value
   */
  static uint8_t readByte(int pos) {
    return EEPROM.readByte(pos);
  }

  /**
   * Writes a uint16_t (two bytes) to the selected part of the memory
   * @param pos position to write to
   * @param value the value to write
   * @return whether writing was successful
   */
  static bool writeUInt16(int pos, uint16_t value) {

    uint8_t first = value >> (uint8_t) 8;
    uint8_t second = value & (uint16_t) 0x00ff;

    bool success = writeByte(pos, first);
    success = success & writeByte(pos + 1, second);

    return success;
  }

  /**
   * Reads a uint16_t (two bytes) from the selected part of the memory
   * @param pos position to read from
   * @return the uint16_t value
   */
  static uint16_t readUInt16(int pos) {
    auto first = readByte(pos);
    auto second = readByte(pos + 1);
    return (uint16_t) ((first * (0xFF + 1)) + second);
  }

  /**
   * Writes the content of a string to the eeprom
   * @param start start-index for the string
   * @param max_len length of the string
   * @param content the string to write
   * @return whether writing was successful
   */
  static bool writeContent(int start, int max_len, std::string content) {
    int id_length = content.size();
    if (id_length > max_len) {
      logger.println(LOG_TYPE::ERR, "written content is too long");
      return false;
    }
    for (int pos = 0; pos < id_length; pos++) {
      EEPROM.writeChar(pos + start, content[pos]);
    }
    EEPROM.writeChar(id_length + start, '\0');
    EEPROM.commit();
    return true;
  }

  /**
   * Reads the sontent of the eeprom as a string
   * @param start start-index of the string
   * @param max_len maximum length of the string to read
   * @return the read content
   */
  static std::string readContent(int start, int max_len) {
    std::stringstream ss;
    for (int pos = 0; pos < max_len; pos++) {
      char c = EEPROM.readChar(pos + start);
      if (c == '\n' || c == 0) {
        return ss.str();
      }
      ss << c;
    }
    return ss.str();
  }

  /**
   * Reads how many gadgets are currently saved in the eeprom
   * @return the gadget count
   */
  static uint8_t getGadgetCount() {
    return readByte(GADGET_COUNT_POS);
  }

  /**
   * Writes the new gadget count to the eerpom
   * @param count how many gadgets are stored in eeprom
   * @return whether saving was successful or not
   */
  static bool writeGadgetCount(uint8_t count) {
    return writeByte(GADGET_COUNT_POS, count);
  }

  /**
   * Gets the memory position the selected gadget starts at
   * @param gadget_nr the nr of the wanted gadget
   * @return the memory position
   */
  static uint16_t getGadgetMemoryStart(uint8_t gadget_nr) {
    if (gadget_nr >= GADGET_MAX_COUNT) {
      return 0;
    }
    if (gadget_nr == 0) {
      return GADGET_BLOCK_START;
    }
    return readUInt16(GADGET_POS_START + (gadget_nr * 2));
  }

  /**
   * Gets the end of the memory segment for the selected gadget
   * @param gadget_nr the nr of the wanted gadget
   * @return the end of the memory
   */
  static uint16_t getGadgetMemoryEnd(uint8_t gadget_nr) {
    if (gadget_nr >= GADGET_MAX_COUNT) {
      return 0;
    }
    return readUInt16(GADGET_POS_START + ((gadget_nr + 1) * 2));
  }

  /**
   * Sets the end of the memory segment for the selected gadget
   * @param gadget_nr the nr of the wanted gadget
   * @param mem_end the end of the memory
   * @return whether writing was successful
   */
  static bool setGadgetMemoryEnd(uint8_t gadget_nr, uint16_t mem_end) {
    if (gadget_nr >= GADGET_MAX_COUNT) {
      return false;
    }
    if (mem_end < GADGET_BLOCK_START) {
      return false;
    }
    if (mem_end >= EEPROM_CONFIG_LEN_MAX) {
      return false;
    }
    return writeUInt16(GADGET_POS_START + ((gadget_nr + 1) * 2), mem_end + 1);
  }

public:

  /**
   * Prints the whole eeprom layout to the console
   */
  static void printEEPROMLayout() {
    std::stringstream ss;
    ss << "EEPROM config:";
    ss << "\nvalid config bitfield: " << VALID_CONFIG_BITFIELD_BYTE;
    ss << "\ngadget count: " << GADGET_COUNT_POS;
    ss << "\nsystem settings bitfield: " << SYSTEM_SETTINGS_BITFIELD_BYTE;
    ss << "\nir_recv pin: " << IR_RECV_PIN_POS;
    ss << "\nir_send pin: " << IR_SEND_PIN_POS;
    ss << "\nradio_recv pin: " << RADIO_RECV_POS;
    ss << "\nradio_send pin: " << RADIO_SEND_POS;
    ss << "\nnetwork mode: " << NETWORK_MODE_POS;
    ss << "\ngadget remote: " << GADGET_REMOTE_POS;
    ss << "\ncode remote: " << CODE_REMOTE_POS;
    ss << "\nevent remote: " << EVENT_REMOTE_POS;
    ss << "\nid: " << ID_POS << " - " << ID_POS + ID_MAX_LEN;
    ss << "\nwifi_ssid: " << WIFI_SSID_POS << " - " << WIFI_SSID_POS + WIFI_SSID_MAX_LEN;
    ss << "\nwifi_pw: " << WIFI_PW_POS << " - " << WIFI_PW_POS + WIFI_PW_MAX_LEN;
    ss << "\nmqtt_ip: " << MQTT_IP_POS << " - " << MQTT_IP_POS + MQTT_IP_MAX_LEN;
    ss << "\nmqtt_port: " << MQTT_PORT_POS << " - " << MQTT_PORT_POS + MQTT_PORT_MAX_LEN;
    ss << "\nmqtt_user: " << MQTT_USER_POS << " - " << MQTT_USER_POS + MQTT_USER_MAX_LEN;
    ss << "\nmqtt_pw: " << MQTT_PW_POS << " - " << MQTT_PW_POS + MQTT_PW_MAX_LEN;
    Serial.println(ss.str().c_str());
  }

  /**
   * Resets the valid content flag to 0
   */
  static void resetContentFlag() {
    uint8_t content_flag = 0;
    EEPROM.writeByte(VALID_CONFIG_BITFIELD_BYTE, content_flag);
    EEPROM.commit();
  }

  // init eeprom
  /**
   * Initializes the system EEPROM
   * @return whether the EEPROM was correctly initialized
   */
  static bool initEEPROM() {
    logger.println("Initializing EEPROM...");

    if (!EEPROM.begin(EEPROM_CONFIG_LEN_MAX)) {
      logger.println(LOG_TYPE::ERR, "failed to initialize EEPROM");
      return false;
    }
    return true;
  }

  // read and write gadgets
  /**
   * Writes the data for a gadget to the eeprom
   * @param config_bf the configuration-bitfield
   * @param gadget_type the gadget-type
   * @param gadget_json the general gadget config
   * @param code_json the code-mapping config
   * @return whether writing was successful
   */
  static bool writeGadget(uint8_t config_bf, uint8_t gadget_type, const std::string& gadget_json, const std::string& code_json) {
    auto gadget_index = readByte(GADGET_COUNT_POS);

    if (gadget_index >= GADGET_MAX_COUNT) {
      logger.println(LOG_TYPE::ERR, "Cannot save gadget: maximum count of gadgets reached");
      return false;
    }

    auto addr = getGadgetMemoryStart(gadget_index);
    uint16_t gadget_json_len = gadget_json.length();
    uint16_t code_json_len = code_json.length();
    uint16_t complete_len = 1 + 1 + 2 + gadget_json_len + code_json_len;
    uint16_t end_index = addr + complete_len;

    if (end_index > EEPROM_CONFIG_LEN_MAX) {
      logger.println(LOG_TYPE::ERR, "Cannot save gadget: missing space in eeprom");
      return false;
    }

    auto success = true;
    success = writeByte(addr, config_bf);
    success = success && writeByte(addr + 1, gadget_type);
    success = success && writeUInt16(addr + 2, gadget_json_len);
    success = success && writeContent(addr + 4, gadget_json_len, gadget_json);
    success = success && writeContent(addr + 4 + gadget_json_len + 1, code_json_len, code_json);

    if (!success) {
      logger.println(LOG_TYPE::ERR, "Cannot save gadget: error writing content");
      return false;
    }

    // set the starting point for the next gadget
    if (!setGadgetMemoryEnd(gadget_index, end_index)) {
      logger.println(LOG_TYPE::ERR, "Cannot save gadget: error saving gadget memory end");
      return false;
    }

    // increment gadget count to formally "save" the gadget
    writeGadgetCount(gadget_index + 1);
    return true;
  }

  /**
   * Reads the data for a gadget from the eeprom
   * @param gadget_index the position of the gadget
   * @return the data for the gadget
   */
  static gadget_tuple readGadget(uint8_t gadget_index) {
    auto addr = getGadgetMemoryStart(gadget_index);
    auto addr_end = getGadgetMemoryEnd(gadget_index);
    auto stored_gadgets = readByte(GADGET_COUNT_POS);

    if (!addr || !addr_end || gadget_index >= stored_gadgets) {
      return std::tuple<uint8_t, uint8_t, std::string, std::string>(0, 0, "", "");
    }
    auto config_bf = readByte(addr);
    auto gadget_type = readByte(addr + 1);
    auto gadget_json_len = readUInt16(addr + 2);
    auto gadget_json = readContent(addr + 4, gadget_json_len);
    auto code_json = readContent(addr + 4 + gadget_json_len + 1, addr_end);
    return std::tuple<uint8_t, uint8_t, std::string, std::string>(config_bf, gadget_type, gadget_json, code_json);
  }

  /**
   * Deletes the gadget with the selected index and moves all other gadgets up
   * @param gadget_index the index of the gadgets to be deleted
   * @return whether the process of deleting was successful
   */
  static bool deleteGadget(uint8_t gadget_index) {
    uint8_t gadget_count = getGadgetCount();

    if (gadget_count == 0) {
      logger.println(LOG_TYPE::ERR, "Cannot delete gadget: no gadget saved");
      return false;
    }

    if (gadget_index > gadget_count -1) {
      logger.println(LOG_TYPE::ERR, "Cannot delete gadget: index does not exist");
      return false;
    }

    if (gadget_index == gadget_count -1) {
      return writeGadgetCount(gadget_count - 1);
    }

    std::vector<gadget_tuple> gadgets;

    for (uint8_t i = gadget_index + 1; i < gadget_count; i++) {
      auto buf_gadget = readGadget(i);
      gadgets.push_back(buf_gadget);
    }

    if (!writeGadgetCount(gadget_index)) {
      logger.println(LOG_TYPE::ERR, "Error in deleting process while updating gadget count");
      return false;
    }

    auto copy_gadget_count = (uint8_t) gadgets.size();

    for (uint8_t i = 0; i < copy_gadget_count; i++) {
      auto buf_gadget = gadgets[i];

      auto e1 = std::get<0>(buf_gadget);
      auto e2 = std::get<1>(buf_gadget);
      auto e3 = std::get<2>(buf_gadget);
      auto e4 = std::get<3>(buf_gadget);

      if (!writeGadget(e1, e2, e3, e4)) {
        logger.println(LOG_TYPE::ERR, "Error in in deletion process: moving gadgets failed");
        return false;
      }
    }
    return true;
  }

  /**
   * Invalidates all saved gadgets
   * @return whehter invalidating the gadgets was successful
   */
  static bool resetGadgets() {
    return writeGadgetCount(0);
  }

  // read + write IR pins
  /**
   * Writes the ir receive pin to the eeprom
   * @param pin the pin used for ir receiver
   * @return whether writing was successful
   */
  static bool writeIRrecvPin(uint8_t pin) {
    return writeByte(IR_RECV_PIN_POS, pin);
  }

  /**
   * Reads the ir receive pin from the eeprom
   * @return the ir recv pin
   */
  static uint8_t readIRrecvPin() {
    return readByte(IR_RECV_PIN_POS);
  }

  /**
   * Writes the ir send pin to the eeprom
   * @param pin the pin used for ir sender
   * @return whether writing was successful
   */
  static bool writeIRsendPin(uint8_t pin) {
    return writeByte(IR_SEND_PIN_POS, pin);
  }

  /**
   * Reads the ir send pin from the eeprom
   * @return the ir send pin
   */
  static uint8_t readIRsendPin() {
    return readByte(IR_SEND_PIN_POS);
  }

  // read + write radio pins
  /**
   * Writes the radio receiver pin to the eeprom
   * @param pin the pin used for receiving radio
   * @return whether writing radio was successful
   */
  static bool writeRadioRecvPin(uint8_t pin) {
    return writeByte(RADIO_RECV_POS, pin);
  }

  /**
   * Reads the radio receiver pin from the eeprom
   * @return the radio receiver pin
   */
  static uint8_t readRadioRecvPin() {
    return readByte(RADIO_RECV_POS);
  }

  /**
  * Writes the radio send pin to the eeprom
  * @param pin the pin used for radio sending
  * @return whether writing was successful
  */
  static bool writeRadioSendPin(uint8_t pin) {
    return writeByte(RADIO_SEND_POS, pin);
  }

  /**
   * Reads the radio send pin from the eeprom
   * @return the radio send pin
   */
  static uint8_t readRadioSendPin() {
    return readByte(RADIO_SEND_POS);
  }

  // read + write network mode
  /**
   * Writes the network mode to the eeprom
   * @param mode the mode the network should use
   * @return whether writing was successful
   */
  static bool writeNetworkMode(NetworkMode mode) {
    return writeByte(NETWORK_MODE_POS, (uint8_t) mode);
  }

  /**
   * Reads the network mode from the eeprom
   * @return the network mode
   */
  static NetworkMode readNetworkMode() {
    uint8_t mode = readByte(NETWORK_MODE_POS);
    if (mode < NetworkModeCount) {
      return (NetworkMode) mode;
    }
    return NetworkMode::None;
  }

  // read + write gadget remote
  /**
   * Writes the gadget remote mode to the eeprom.
   * Writing a 0 is interpreted as 'no remote used'.
   * @param mode the mode used for the gadget remote
   * @return whether writing was successful
   */
  static bool writeGadgetRemote(GadgetRemoteMode mode) {
    return writeByte(GADGET_REMOTE_POS, (uint8_t) mode);
  }

  /**
   * Reads the gadget remote mode from the eeprom
   * @return the gadget remote mode
   */
  static GadgetRemoteMode readGadgetRemote() {
    uint8_t mode = readByte(GADGET_REMOTE_POS);
    if (mode < GadgetRemoteModeCount) {
      return (GadgetRemoteMode) mode;
    }
    return GadgetRemoteMode::None;
  }

  // read + write code remote
  /**
   * Writes the code remote mode to the eeprom.
   * Writing a 0 is interpreted as 'no remote used'.
   * @param mode the mode used for the code remote
   * @return whether writing was successful
   */
  static bool writeCodeRemote(CodeRemoteMode mode) {
    return writeByte(CODE_REMOTE_POS, (uint8_t) mode);
  }

  /**
   * Reads the code remote mode from the eeprom
   * @return the code remote mode
   */
  static CodeRemoteMode readCodeRemote() {
    uint8_t mode =  readByte(CODE_REMOTE_POS);
    if (mode < CodeRemoteModeCount) {
      return (CodeRemoteMode) mode;
    }
    return CodeRemoteMode::None;
  }

  // read + write event remote
  /**
   * Writes the event remote mode to the eeprom.
   * Writing a 0 is interpreted as 'no remote used'.
   * @param mode the mode used for the event remote
   * @return whether writing was successful
   */
  static bool writeEventRemote(EventRemoteMode mode) {
    return writeByte(EVENT_REMOTE_POS, (uint8_t) mode);
  }

  /**
   * Reads the event remote mode from the eeprom
   * @return the gadget remote mode
   */
  static EventRemoteMode readEventRemote() {
    uint8_t mode = readByte(EVENT_REMOTE_POS);
    if (mode < EventRemoteModeCount) {
      return (EventRemoteMode) mode;
    }
    return EventRemoteMode::None;
  }

  // read + write ID
  /**
   * Writes the chip identifier to the eeprom
   * @param id the chip id to be written
   * @return whether writing was successful
   */
  static bool writeID(const std::string& id) {
    bool success;
    if (id == "null") {
      setContentFlag(CONFIG_CHECK_INDEX_ID, false);
      success = true;
    } else {
      success = writeContent(ID_POS, ID_MAX_LEN, id);
      setContentFlag(CONFIG_CHECK_INDEX_ID, success);
    }
    return success;
  }

  /**
   * Reads the chip identifier from the eeprom
   * @return the chip identifier
   */
  static std::string readID() {
    if (!hasValidID()) {
      std::stringstream sstr;
      sstr << "esp_" << (micros() % 17731776);
      writeID(sstr.str());
    }
    return readContent(ID_POS, ID_MAX_LEN);
  }

  /**
   * Checks if the there is a valid ID stored in the EEPROM
   * @return whether there is a valid id
   */
  static bool hasValidID() {
    return getContentFlag(CONFIG_CHECK_INDEX_ID);
  }

  /**
   * Writes the WIFI SSID to the eeprom
   * @param ssid the ssid to be written
   * @return whether writing was successful
   */
  static bool writeWifiSSID(const std::string& ssid) {
    bool success;
    if (ssid == "null") {
      setContentFlag(CONFIG_CHECK_INDEX_WIFI_SSID, false);
      success = true;
    } else {
      success = writeContent(WIFI_SSID_POS, WIFI_SSID_MAX_LEN, ssid);
      setContentFlag(CONFIG_CHECK_INDEX_WIFI_SSID, success);
    }
    return success;
  }

  /**
   * Reads the WIFI SSID from the eeprom
   * @return the wifi ssid
   */
  static std::string readWifiSSID() {
    return readContent(WIFI_SSID_POS, WIFI_SSID_MAX_LEN);
  }

  /**
   * Checks if the there is a valid WIFI SSID stored in the EEPROM
   * @return whether there is a valid ssid
   */
  static bool hasValidWifiSSID() {
    return getContentFlag(CONFIG_CHECK_INDEX_WIFI_SSID);
  }

  /**
   * Writes the WIFI password to the eeprom
   * @param pw the password to be written
   * @return whether writing was successful
   */
  static bool writeWifiPW(const std::string& pw) {
    bool success;
    if (pw == "null") {
      setContentFlag(CONFIG_CHECK_INDEX_WIFI_PW, false);
      success = true;
    } else {
      success = writeContent(WIFI_PW_POS, WIFI_PW_MAX_LEN, pw);
      setContentFlag(CONFIG_CHECK_INDEX_WIFI_PW, success);
    }
    return success;
  }

  /**
   * Reads the WIFI password from the eeprom
   * @return the wifi password
   */
  static std::string readWifiPW() {
    return readContent(WIFI_PW_POS, WIFI_PW_MAX_LEN);
  }

  /**
   * Checks if the there is a valid WIFI password stored in the EEPROM
   * @return whether there is a valid password
   */
  static bool hasValidWifiPW() {
    return getContentFlag(CONFIG_CHECK_INDEX_WIFI_PW);
  }

  /**
   * Writes the MQTT IP-Address to the eeprom.
   * Write 0.0.0.0 to set ip to 'no valid ip'
   * @param ip the ip to be written
   * @return whether writing was successful
   */
  static bool writeMQTTIP(const IPAddress& ip) {
    bool success = true;
    for (int i = 0; i < 4; i++) {
      success = success && writeByte(MQTT_IP_POS + i, ip[i]);
    }
    if (ip == IPAddress(0, 0, 0, 0)) {
      setContentFlag(CONFIG_CHECK_INDEX_MQTT_PW, false);
      success = true;
    } else {
      setContentFlag(CONFIG_CHECK_INDEX_MQTT_PW, success);
    }
    return success;
  }

  /**
   * Reads the MQTT IP-Address from the eeprom
   * @return the ip
   */
  static IPAddress readMQTTIP() {
    IPAddress ip;
    for (int i = 0; i < 4; i++) {
      ip[i] = readByte(MQTT_IP_POS + i);
    }
    return ip;
  }

  /**
   * Checks if the there is a valid MQTT IP-Address stored in the EEPROM
   * @return whether there is a valid ip
   */
  static bool hasValidMQTTIP() {
    return getContentFlag(CONFIG_CHECK_INDEX_MQTT_IP);
  }

  /**
   * Writes the MQTT port to the eeprom.
   * Write 0 to set port to 'no valid port'
   * @param port the port to be written
   * @return whether writing was successful
   */
  static bool writeMQTTPort(uint16_t port) {
    bool success = writeUInt16(MQTT_PORT_POS, port);

    if (port == 0) {
      setContentFlag(CONFIG_CHECK_INDEX_MQTT_PORT, false);
      success = true;
    } else {
      setContentFlag(CONFIG_CHECK_INDEX_MQTT_PORT, success);
    }
    return success;
  }

  /**
   * Reads the MQTT port from the eeprom
   * @return the port
   */
  static uint16_t readMQTTPort() {
    return readUInt16(MQTT_PORT_POS);
  }

  /**
   * Checks if the there is a valid MQTT port stored in the EEPROM
   * @return whether there is a valid port
   */
  static bool hasValidMQTTPort() {
    return getContentFlag(CONFIG_CHECK_INDEX_MQTT_PORT);
  }

  /**
   * Writes the MQTT username to the eeprom
   * @param username the username to be written
   * @return whether writing was successful
   */
  static bool writeMQTTUsername(const std::string &username) {
    bool success;
    success = writeContent(MQTT_USER_POS, MQTT_USER_MAX_LEN, username);
    if (username == "null") {
      setContentFlag(CONFIG_CHECK_INDEX_MQTT_USER, false);
      success = true;
    } else {
      setContentFlag(CONFIG_CHECK_INDEX_MQTT_USER, success);
    }
    return success;
  }

  /**
   * Reads the MQTT username from the eeprom
   * @return the username
   */
  static std::string readMQTTUsername() {
    return readContent(MQTT_USER_POS, MQTT_USER_MAX_LEN);
  }

  /**
   * Checks if the there is a valid MQTT username stored in the EEPROM
   * @return whether there is a valid username
   */
  static bool hasValidMQTTUsername() {
    return getContentFlag(CONFIG_CHECK_INDEX_MQTT_USER);
  }

  /**
  * Writes the MQTT password to the eeprom
  * @param pw the password to be written
  * @return whether writing was successful
  */
  static bool writeMQTTPassword(const std::string &pw) {
    bool success;
    success = writeContent(MQTT_PW_POS, MQTT_PW_MAX_LEN, pw);
    if (pw == "null") {
      setContentFlag(CONFIG_CHECK_INDEX_MQTT_PW, false);
      success = true;
    } else {
      setContentFlag(CONFIG_CHECK_INDEX_MQTT_PW, success);
    }
    return success;
  }

  /**
   * Reads the MQTT password from the eeprom
   * @return the password
   */
  static std::string readMQTTPassword() {
    return readContent(MQTT_PW_POS, MQTT_PW_MAX_LEN);
  }

  /**
   * Checks if the there is a valid MQTT password stored in the EEPROM
   * @return whether there is a valid password
   */
  static bool hasValidMQTTPassword() {
    return getContentFlag(CONFIG_CHECK_INDEX_MQTT_PW);
  }

  /**
   * Checks the EEPROM for valid wifi ssid + password and mqtt ip + port
   * @return whether all of these four are valid
   */
  static bool hasValidNetworkConfig() {
    return (hasValidWifiSSID() && hasValidWifiPW() && hasValidMQTTIP() && hasValidMQTTPort());
  }

  /**
   * Reads the content of the EEPROM
   * @return the content of the first 500 cells of the EEPROM
   */
  static std::string readWholeEEPROM() {
    std::stringstream ss;
    for (int i = 0; i < 500; i++) {
      char bufchar = EEPROM.readChar(i);
      if (bufchar == 0) {
        ss << "\\n";
      } else {
        ss << bufchar;
      }
    }
    return ss.str();
  }

  /**
   * Fills EEPROM with repeating '-_-_-_'-sequences
   */
  static void writeTestEEPROM() {
    for (int i = 0; i < 750; i++) {
      if (i % 2) {
        EEPROM.writeChar(i, '-');
      } else {
        EEPROM.writeChar(i, '_');
      }
//      char k = (char) (48 + (i % 10));
//      EEPROM.writeChar(i, k);
    }

    writeByte(GADGET_COUNT_POS, 0);
    writeByte(VALID_CONFIG_BITFIELD_BYTE, 0);
    writeByte(SYSTEM_SETTINGS_BITFIELD_BYTE, 0);

    EEPROM.commit();
  }
};
