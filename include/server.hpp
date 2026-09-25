#ifndef SERVER_HPP
#define SERVER_HPP

#include "simulation.hpp"
#include <string>
#include <atomic>

class HttpServer {
public:
    HttpServer(SimulationEngine& engine, int port = 8080);
    ~HttpServer();

    bool start();
    void stop();

private:
    void acceptLoop(int server_fd);
    void handleClient(int client_socket);

    SimulationEngine& engine_;
    int port_;
    std::atomic<bool> running_;
    int server_fd_;
};

#endif // SERVER_HPP
