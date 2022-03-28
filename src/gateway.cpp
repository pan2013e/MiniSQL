#include "gateway.h"
#include "API.h"

using namespace std;
using HttpServer = SimpleWeb::Server<SimpleWeb::HTTP>;

void init_web_interface() {
    HttpServer server;
    server.config.port = 8080;
    server.resource["^/([0-9]+)$"]["GET"] = [](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request) {
        response->write(request->path_match[1].str());
    };
    promise<unsigned short> server_port;
    std::thread server_thread([&server, &server_port]() {
        // Start server
        server.start([&server_port](unsigned short port) {
            server_port.set_value(port);
        });
    });
    server_thread.join();
}