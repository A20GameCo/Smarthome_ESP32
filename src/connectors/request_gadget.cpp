#include "request_gadget.h"

#include <utility>

// Request_Gadget
void Request_Gadget::addIncomingRequest(Request *request) {
  xQueueSend(in_request_queue_, &request, portMAX_DELAY);
};

void Request_Gadget::sendQueuedItems() {
  if (!request_gadget_is_ready_) {
    return;
  }
  if (uxQueueMessagesWaiting(out_request_queue_) > 0) {
    Request *buf_req;
    xQueueReceive(out_request_queue_, &buf_req, portMAX_DELAY);
    executeRequestSending(buf_req);
  }
}

Request_Gadget::Request_Gadget() :
  type_(RequestGadgetType::NONE_G),
  request_gadget_is_ready_(false) {
  in_request_queue_ = xQueueCreate(REQUEST_QUEUE_LEN, sizeof(Request *));
  out_request_queue_ = xQueueCreate(REQUEST_QUEUE_LEN, sizeof(Request *));
}

Request_Gadget::Request_Gadget(RequestGadgetType t) :
  type_(t),
  request_gadget_is_ready_(false) {
  in_request_queue_ = xQueueCreate(REQUEST_QUEUE_LEN, sizeof(Request *));
  out_request_queue_ = xQueueCreate(REQUEST_QUEUE_LEN, sizeof(Request *));
  request_gadget_is_ready_ = true;
}

bool Request_Gadget::requestGadgetIsReady() const {
  return request_gadget_is_ready_;
}

RequestGadgetType Request_Gadget::getGadgetType() {
  return type_;
}

bool Request_Gadget::hasRequest() {
  return uxQueueMessagesWaiting(in_request_queue_) > 0;
}

Request *Request_Gadget::getRequest() {
  Request *buf_req;
  xQueueReceive(in_request_queue_, &buf_req, portMAX_DELAY);
  return buf_req;
}

void Request_Gadget::sendRequest(Request *request) {
  xQueueSend(out_request_queue_, &request, portMAX_DELAY);
};