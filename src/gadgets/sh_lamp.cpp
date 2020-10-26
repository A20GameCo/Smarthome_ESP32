#include "sh_lamp.h"

#include <utility>

SH_Lamp::SH_Lamp(std::string name, SHLampType lamp_type) :
  SH_Gadget(std::move(name), GadgetType::Lightbulb),
  lamp_color_(0, 0, 0),
  default_brightness_(75),
  min_brightness_(10),
  last_brightness_(75),
  lamp_type_(lamp_type) {}

void SH_Lamp::setBrightness(byte new_brightness) {
  lamp_color_.setBrightness(new_brightness);
  setGadgetHasChanged();
  updateCharacteristic("brightness", (int) new_brightness);
}

float SH_Lamp::getBrightness() {
  return lamp_color_.getBrightness();
}

void SH_Lamp::setColor(uint8_t r, uint8_t g, uint8_t b) {
  lamp_color_.setRGB(r, g, b);
  setGadgetHasChanged();
}

uint8_t SH_Lamp::getColor(uint8_t color_index) {
  if (color_index == 0)
    return lamp_color_.getRGB()->getRed();
  if (color_index == 1)
    return lamp_color_.getRGB()->getGreen();
  if (color_index == 2)
    return lamp_color_.getRGB()->getBlue();
  return 0;
}

void SH_Lamp::getColor(uint8_t color_buffer[]) {
  color_buffer[0] = lamp_color_.getRGB()->getRed();
  color_buffer[1] = lamp_color_.getRGB()->getGreen();
  color_buffer[2] = lamp_color_.getRGB()->getBlue();
}

void SH_Lamp::setHue(unsigned int new_hue) {
  lamp_color_.setHue(new_hue);
  setGadgetHasChanged();
  updateCharacteristic("hue", (int) new_hue);
}

float SH_Lamp::getHue() {
  return lamp_color_.getHue();
}

void SH_Lamp::toggleStatus() {
  setStatus(!getStatus());
}

bool SH_Lamp::getStatus() {
  return getBrightness() != 0;
}

void SH_Lamp::setStatus(bool new_status) {
  if (new_status == 0) {
    lamp_color_.setBrightness(0);
  } else {
    if (last_brightness_ > min_brightness_) {
      lamp_color_.setBrightness(last_brightness_);
    } else {
      lamp_color_.setBrightness(default_brightness_);
    }
  }
  setGadgetHasChanged();
  updateCharacteristic("status", new_status);
}

void SH_Lamp::handleCharacteristicUpdate(const char *characteristic, int value) {
  logger.print(getName(), "Updating Characteristic: '");
  logger.print(characteristic);
  logger.println("'");
  if (strcmp(characteristic, "status") == 0) {
    setStatus((bool) value);
  } else if (strcmp(characteristic, "brightness") == 0) {
    setBrightness((byte) value);
  } else if (strcmp(characteristic, "hue") == 0) {
    setHue((unsigned int) value);
  }
}

bool SH_Lamp::getCharacteristics(char *characteristic_str) {
  switch (lamp_type_) {
    case SHLampType::ON_OFF :
      return false;
    case SHLampType::BRI_ONLY :
      strcpy(characteristic_str, R"("brightness": "default")");
      break;
    case SHLampType::CLR_ONLY :
      strcpy(characteristic_str, R"("hue": "default", "saturation": "default")");
      break;
    case SHLampType::CLR_BRI :
      strcpy(characteristic_str, R"("brightness": "default", "hue": "default", "saturation": "default")");
      break;
    default :
      return false;
  }
  return true;
}

void SH_Lamp::handleMethodUpdate(GadgetMethod method) {
  switch (method) {
    case GadgetMethod::toggleStatus:
      toggleStatus();
      break;
    case GadgetMethod::turnOn:
      setStatus(true);
      break;
    case GadgetMethod::turnOff:
      setStatus(false);
      break;
    default:
      break;
  }
}
