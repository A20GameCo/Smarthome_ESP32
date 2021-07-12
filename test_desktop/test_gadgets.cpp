#include <stdexcept>
#include <cassert>

#include "../src/console_logger.h"
#include "../src/test_dummys/gadget_dummy.h"

void test_gadgets(){
  DummyGadget gadget("homer", GadgetType::Lightbulb);
  assert(!gadget.hasInitError());
}

int main () {
  logger_i("Gadget", "testing gadgets...");
  test_gadgets();
  logger_i("Gadget", "finished test.");
}