#pragma once

#include "../gadgets/sh_gadget.h"

class DummyGadget : public SH_Gadget {
protected:

  void executeCharacteristicUpdate(CharacteristicIdentifier characteristic, int value) override;

public:

  DummyGadget(std::string name, GadgetType type);

  std::vector<GadgetCharacteristicSettings> getCharacteristics() override;

  void handleCharacteristicUpdate(CharacteristicIdentifier characteristic, int value) override;

  void handleEvent(std::string sender, EventType event_type) override;

  void refresh() override;

  void handleMethodUpdate(GadgetMethod method) override;

};

DummyGadget::DummyGadget(std::string name, GadgetType type) : SH_Gadget(name, type) {

}

void DummyGadget::executeCharacteristicUpdate(CharacteristicIdentifier characteristic, int value) {}

std::vector<GadgetCharacteristicSettings> DummyGadget::getCharacteristics() {
  return std::vector<GadgetCharacteristicSettings>();
}

void DummyGadget::handleCharacteristicUpdate(CharacteristicIdentifier characteristic, int value) {
  SH_Gadget::handleCharacteristicUpdate(characteristic, value);
}

void DummyGadget::handleEvent(std::string sender, EventType event_type) {
  SH_Gadget::handleEvent(sender, event_type);
}

void DummyGadget::refresh() {}

void DummyGadget::handleMethodUpdate(GadgetMethod method) {}


