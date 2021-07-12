#include <stdexcept>
#include <cassert>

#include "../src/console_logger.h"
#include "../src/connectors/request_queue.cpp"

void test_request_queue() {
  RequestQueue queue;

  assert(queue.isEmpty());

  auto test_req = std::make_shared<Request>("/test",
                                            12345,
                                            "testsuite",
                                            "nobody",
                                            DynamicJsonDocument(15));

  queue.push(test_req);
  assert(!queue.isEmpty());

  auto out_req = queue.pop();

  assert(out_req != nullptr);

  assert(*test_req == *out_req);
}

int main () {
  logger_i("RequestQueue", "starting test of request queue...");
  test_request_queue();
  logger_i("RequestQueue", "test ended.");
}