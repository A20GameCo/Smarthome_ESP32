#include <stdexcept>
#include <cassert>

#include "../src/console_logger.h"

void test_blub() {
  assert(true);
}

int main () {
  logger_i("UnitBlub", "starting uniting all the blubs" );
  test_blub();
  logger_i("UnitBlub2", "done uniting");
}
