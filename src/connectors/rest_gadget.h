#ifndef __REST_Connector__
#define __REST_Connector__

#include <WebServer.h>
#include <ESPmDNS.h>
#include "request_gadget.h"
#include "../system_storage.h"

class RestRequest : public Request {
private:
  WiFiClient *client;
  unsigned int port{};
  IPAddress ip;
  char content_type[REQUEST_CONTENT_TYPE_LEN_MAX]{};
  bool can_respond;

public:

  RestRequest(REQUEST_TYPE req_type, const char *req_path, const char *req_body, unsigned int req_port,
              const IPAddress &req_ip, const char *req_content_type) :
    Request(req_type, req_path, req_body),
    client(nullptr),
    port(req_port),
    ip(req_ip),
    can_respond(false) {
    strncpy(content_type, req_content_type, REQUEST_CONTENT_TYPE_LEN_MAX);
  }

  RestRequest(REQUEST_TYPE req_type, const char *req_path, const char *req_body, const char *req_content_type,
              std::function<void(Request *request)> answer_method, WiFiClient *network_client) :
    Request(req_type, req_path, req_body, answer_method),
    client(network_client),
    can_respond(true) {
    strncpy(content_type, req_content_type, REQUEST_CONTENT_TYPE_LEN_MAX);
  }

  RestRequest(REQUEST_TYPE req_type, const char *req_path, const char *req_body, const char *req_content_type,
              WiFiClient *network_client) :
    Request(req_type, req_path, req_body),
    client(network_client),
    can_respond(true) {
    strncpy(content_type, req_content_type, REQUEST_CONTENT_TYPE_LEN_MAX);
  }

  ~RestRequest() override = default;;

  RestRequest *createResponse(const char *res_path, const char *res_body) override {
    if (!can_respond) {
      return nullptr;
    }
    RestRequest *res;
    if (validateJson(res_body)) {
      res = new RestRequest(REQ_HTTP_RESPONSE, res_path, res_body, "application/json", client);
    } else {
      res = new RestRequest(REQ_HTTP_RESPONSE, res_path, res_body, "plain/text", client);
    }
    return res;
  }

  unsigned int getPort() {
    return port;
  }

  IPAddress *getIP() {
    return &ip;
  }

  const char *getContentType() {
    return &content_type[0];
  }

  WiFiClient *getClient() {
    return client;
  }
};

// Gadget to communicate with REST Clients/Servers
class REST_Gadget : public Request_Gadget {
protected:
  WiFiClient *client{};
  WebServer *server{};

  void executeRequestSending(Request *req) override {
    logger.print("System / REST", "sending Request '");
    char type[REQUEST_TYPE_LEN_MAX];

    switch (req->getType()) {
      case REQ_HTTP_GET:
        strncpy(type, "GET", REQUEST_TYPE_LEN_MAX);
        break;
      case REQ_HTTP_POST:
        strncpy(type, "POST", REQUEST_TYPE_LEN_MAX);
        break;
      case REQ_HTTP_DELETE:
        strncpy(type, "DELETE", REQUEST_TYPE_LEN_MAX);
        break;
      case REQ_HTTP_PUT:
        strncpy(type, "PUT", REQUEST_TYPE_LEN_MAX);
        break;
      case REQ_HTTP_RESPONSE:
        strncpy(type, "RESPONSE", REQUEST_TYPE_LEN_MAX);
        break;
      default:
        return;
    }
    logger.add(type);
    logger.addln("'");

    auto *rest_req = (RestRequest *) req;
    if (req->getType() == REQ_HTTP_RESPONSE) {
      WiFiClient *res_client = rest_req->getClient();
      if (res_client == nullptr) {
        logger.println(LOG_ERR, "No Client to send on");
        return;
      }
      if (!res_client->connected()) {
        logger.println(LOG_ERR, "Client not connected (anymore)");
        return;
      }
      res_client->print("HTTP/1.1 ");
      res_client->println(rest_req->getPath());
      res_client->print("Content-Type: ");
      res_client->println(rest_req->getContentType());
      res_client->print("Content-Length: ");
      res_client->println(strlen(rest_req->getBody()));
      res_client->println();
      res_client->println(rest_req->getBody());
      res_client->println();
      res_client->stop();
    } else {
      if (client->connect(*(rest_req->getIP()), rest_req->getPort())) {
        logger.print("connected to ");
        logger.addln(client->remoteIP());
        client->print(type);
        client->print(" ");
        client->print(rest_req->getPath());
        client->println(" HTTP/1.1");
        client->println("User-Agent: ESP32_Smarthome");
        client->println("Connection: close");
        client->print("Content-Type: ");
        client->println(rest_req->getContentType());
        client->print("Content-Length: ");
        client->println(strlen(rest_req->getBody()));
        client->println();
        client->println(rest_req->getBody());
        client->println();
        client->stop();
      }
    }
  }

  void initServer() {
    if (MDNS.begin("esp32")) {
      logger.println("MDNS responder started");
    }
    server = new WebServer(80);

    server->on("*", std::bind(&REST_Gadget::handleNotFound, this));
    server->onNotFound(std::bind(&REST_Gadget::handleNotFound, this));
    server->begin();
    logger.println("HTTP server started");
  }

  void handleNotFound() {
    REQUEST_TYPE req_type;
    if (server->method() == HTTP_GET) {
      req_type = REQ_HTTP_GET;
    } else if (server->method() == HTTP_POST) {
      req_type = REQ_HTTP_POST;
    } else if (server->method() == HTTP_DELETE) {
      req_type = REQ_HTTP_DELETE;
    } else if (server->method() == HTTP_PUT) {
      req_type = REQ_HTTP_PUT;
    } else {
      req_type = REQ_UNKNOWN;
    }
    WiFiClient newClient = server->client();
    auto *inc_client = new WiFiClient(newClient);
    Serial.println(inc_client->remotePort());
    Serial.println(inc_client->remoteIP());
    using std::placeholders::_1;
    auto *req = new RestRequest(req_type, server->uri().c_str(), server->arg(0).c_str(), "text/plain",
                                std::bind(&Request_Gadget::sendRequest, this, _1), inc_client);
    addIncomingRequest(req);
  }

public:
  REST_Gadget() :
    Request_Gadget() {};

  explicit REST_Gadget(JsonObject data) :
    Request_Gadget(data) {
    client = new WiFiClient;
    logger.println("Initializing REST_Gadget");
    logger.incIndent();
    initServer();
    logger.decIndent();
    request_gadget_is_ready = true;
  };

  void refresh() override {
    if (!request_gadget_is_ready) {
      return;
    }
    server->handleClient();
    sendQueuedItems();
  }

};

#endif