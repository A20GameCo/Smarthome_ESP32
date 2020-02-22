#ifndef __Request_Connector__
#define __Request_Connector__

// External Imports
#include <ArduinoJson.h>
#include <cstring>
#include <IPAddress.h>

// Other Imports
#include "../system_settings.h"
#include "../console_logger.h"

enum REQUEST_TYPE {
  REQ_UNKNOWN, REQ_HTTP_GET, REQ_HTTP_POST, REQ_HTTP_PUT, REQ_HTTP_DELETE, REQ_MQTT, REQ_SERIAL
};

class Request_Gadget {
protected:
  bool request_gadget_is_ready;
  bool has_request;
  REQUEST_TYPE type;
  char body[REQUEST_BODY_LEN_MAX]{};
  char path[REQUEST_PATH_LEN_MAX]{};

  void setBody(const char *new_body) {
    strncpy(body, new_body, REQUEST_BODY_LEN_MAX);
  };

  void setPath(const char *new_path) {
    strncpy(path, new_path, REQUEST_PATH_LEN_MAX);
  };

  void setRequest(const char *new_path, const char *new_body, REQUEST_TYPE new_type, bool changeStatus = true) {
    setBody(new_body);
    setPath(new_path);
    type = new_type;
    if (changeStatus)
      has_request = true;
  };

public:
  Request_Gadget() :
      request_gadget_is_ready(false) {
  }

  Request_Gadget(JsonObject data) :
      request_gadget_is_ready(false),
      has_request(false),
      type(REQ_UNKNOWN) {
  }

  bool requestGadgetIsReady() {
    return request_gadget_is_ready;
  }

  bool hasRequest() {
    bool buffer = has_request;
    has_request = false;
    return buffer;
  }

  REQUEST_TYPE getRequestType() {
    return type;
  }

  const char *getRequestBody() {
    return &body[0];
  }

  const char *getRequestPath() {
    return &path[0];
  }

  virtual void sendRequest(REQUEST_TYPE req_type, IPAddress ip, const char *req_path, const char *req_body) {
  }

  virtual void sendAnswer(const char *req_body, int status_code) {
  }

  virtual void sendAnswer(JsonObject req_body, int status_code) {
  }

  virtual void refresh() {
  }
};

#endif //__Request_Connector__