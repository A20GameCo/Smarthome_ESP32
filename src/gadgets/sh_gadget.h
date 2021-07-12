#pragma once

#include <cstring>
#include <functional>
#include <vector>
#include <memory>
#include "../mapping_reference.h"
#include "../system_settings.h"
#include "../datatypes.h"
#include "../console_logger.h"

#include "../connectors/ir_gadget.h"
#include "../connectors/radio_gadget.h"
#include "gadget_characteristic_settings.h"
#include "../connectors/event.h"
//#include "../main_system_controller.h"

// Pair for mapping
using mapping_pair = std::tuple<GadgetMethod, std::vector<unsigned long>>;

// List of all Gadget Types
enum class GadgetType {
  None, Lightbulb, Fan, Doorbell, Wallswitch, Motion_Sensor, Temp_Humm_Sensor
};

enum SH_RGB_Color {
  SH_CLR_red, SH_CLR_green, SH_CLR_blue
};

enum SH_HSL_Color {
  SH_CLR_hue, SH_CLR_saturation, SH_CLR_lightness
};

/**
 * Base class for all gadgets in the system
 */
class SH_Gadget {
private:

  // stores whether initializing was successful
  bool init_error;

  // Callback to update a characteristic on the gadget remote
  std::function<void(std::string, CharacteristicIdentifier, int)> gadget_remote_callback;
  std::function<void(std::string, EventType)> event_remote_callback;
  // Flag to determine if the gadget remote is correctly initialized
  bool gadget_remote_ready;
  bool event_remote_ready;

  // Name of the gadget
  const std::string name;

  // Vector to store the code mapping
  std::vector<mapping_pair> code_mapping;

  // Whether the status of the gadget has changed since the last refresh
  bool has_changed;

  // The IR gadget to be used
  std::shared_ptr<IR_Gadget> ir_gadget_;

  // The radio gadget to be used
  std::shared_ptr<Radio_Gadget> radio_gadget_;

  // Controller for the main system. Behaves like a bundle of callbacks of a "delegate lite"
//  std::shared_ptr<MainSystemController> main_controller_;

protected:

  /**
   * Pauses all the tasks in the main system.
   * Use with Care.
   * Always resume the tasks after perfoming whatever action you want to perform.
   */
  void pauseAllTasks();

  /**
   * Resumes all tasks
   */
  void resumeTasks();

  // Type of the gadget
  const GadgetType type;

  /**
   * Checks if the gadget has changed since the last refresh
   * @return Whether the gadget has changed since the last refresh
   */
  bool gadgetHasChanged();

  /**
   * Protocols a change to the gadget for the next refresh
   */
  void setGadgetHasChanged();

  /**
   * Updates the initialization error. If status_update is true, the error will not change. If its false, the error will now be false.
   * @param status_update The update for the init_error
   */
  void updateInitStatus(bool status_update);

  /**
   * Updates a characteristic on the remote
   * @param characteristic Characteristic to update
   * @param value Value for the characteristic
   */
  void updateCharacteristic(CharacteristicIdentifier characteristic, int value);

  /**
   * Returns the method for the passed code
   * @param code The code the message should be returned for
   * @return The Method mapped to the code
   */
  GadgetMethod getMethodForCode(unsigned long code);

  /**
   * Sends an array of raw IR data
   * @param raw_data Raw IR data
   * @param com_len Length of the ir data array
   * @return Whether sending was successful
   */
  bool sendRawIR(const uint16_t raw_data[], uint8_t com_len);

  /**
   * Sends a IR command
   * @param command Command to send
   * @param com_type Coding type of the ri command
   * @return Whether sending was successful
   */
  bool sendIR(unsigned long command, uint8_t com_type);

  /**
   * Updates the gadget with a characteristic
   * @param characteristic Characteristic the gadget should be updated with
   * @param value The value the characteristic should be updated with
   */
  virtual void executeCharacteristicUpdate(CharacteristicIdentifier characteristic, int value) = 0;

public:

  /**
   * Constructor for the base gadget
   * @param name Name of the gadget. Should be unique in the system.
   * @param type Type of the gadget
   */
  explicit SH_Gadget(std::string name, GadgetType type);

  /**
   * @return A serialized JSON representation of this gadget
   */
  DynamicJsonDocument serialized();

  /**
   * Sets the callback for the gadget remote
   * @param update_method Method used to update the remote
   */
  void setGadgetRemoteCallback(std::function<void(std::string, CharacteristicIdentifier, int)> update_method);

  /**
   * Sets the callback for the event remote
   * @param update_method Method used to update the remote
   */
  void setEventRemoteCallback(std::function<void(std::string, EventType)> send_event);

  /**
   * Returns the type of the gadget
   * @return the type of the gadget
   */
  GadgetType getType();

  /**
   * Returns the unique name of the gadget
   * @return the name of the gadget
   */
  std::string getName() const;

  /**
   * Prints the mapping to the console
   */
  void printMapping();

  /**
   * Returns all of the characteristics the gadget makes use of
   * @param characteristic_str [out] a string containing all characteristics
   * @return whether writing the characteristics to the string was successful
   */
  virtual std::vector<GadgetCharacteristicSettings> getCharacteristics() = 0;

  /**
   * Checks if the gadget was successfully initialized
   * @return whether the gadget was successfully initialized
   */
  bool hasInitError() const;

  /**
   * Checks the code for a mapped method and updates the gadget using the method if one is found
   * @param code The code to update the gadget with
   */
  void handleCodeUpdate(unsigned long code);

  /**
   * Updates the gadget with a characteristic
   * @param characteristic Characteristic the gadget should be updated with
   * @param value The value the characteristic should be updated with
   */
  virtual void handleCharacteristicUpdate(CharacteristicIdentifier characteristic, int value);

  virtual void handleEvent(std::string sender, EventType event_type);


  /**
   * Refresh the gadget and its hardware. Used as loop method.
   */
  virtual void refresh() = 0;

  /**
   * Adds a code to a message. This message will be used to update the gadget when the code is received
   * @param method The method to call
   * @param code The code that should trigger the method
   * @return Whether mapping the code to the method was successful
   */
  bool setMethodForCode(GadgetMethod method, unsigned long code);

  /**
   * Method to handle the update of the gadget via GadgetMethod
   * @param method Method to update the gadget with
   */
  virtual void handleMethodUpdate(GadgetMethod method) = 0;

  /**
   * Initialized the IR capabilities of the gadget
   * @param new_ir_gadget The IR_Gadget to be used
   */
  void setIR(const std::shared_ptr<IR_Gadget> &new_ir_gadget);

  /**
   * Returns whether the gadget has IR access or not
   * @return Whether the gadget has IR access or not
   */
  bool hasIR() const;

  /**
   * Initialized the radio capabilities of the gadget
   * @param new_radio_gadget The Radio_Gadget to be used
   */
  void setRadio(const std::shared_ptr<Radio_Gadget> &new_radio_gadget);

  /**
   * Returns whether the gadget has radio access or not
   * @return Whether the gadget has radio access or not
   */
  bool hasRadio() const;

  /**
   * Sets the main controller for the created gadget.
   * @param controller The controller for the main system
   */
//  void setMainController(std::shared_ptr<MainSystemController> controller);
};
