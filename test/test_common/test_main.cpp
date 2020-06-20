#include <unity.h>

void test_dummy_true(void) {
  TEST_ASSERT_EQUAL(1, 1);
}

void test_dummy_false(void) {
  TEST_ASSERT_EQUAL(1, 3);
}

void process() {
  UNITY_BEGIN();
  RUN_TEST(test_dummy_true);
  RUN_TEST(test_dummy_false);
  UNITY_END();
}

#ifdef ARDUINO

#include <Arduino.h>
void setup() {
    // NOTE!!! Wait for >2 secs
    // if board doesn't support software reset via Serial.DTR/RTS
    delay(2000);

    process();
}

void loop() {
    digitalWrite(13, HIGH);
    delay(100);
    digitalWrite(13, LOW);
    delay(500);
}

#else

int main(int argc, char **argv) {
  std::cout << "Running Desktop Tests..." << std::endl;
  process();
  std::cout << "Tests Done." << std::endl;
  return 0;
}

#endif