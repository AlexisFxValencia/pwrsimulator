#include "reactor.hpp"

#include <iostream>
#include <thread>
#include <chrono>
#include <mutex>
#include <fstream>
#include <sstream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <algorithm>

// Global reactor instance and synchronization mutex
Reactor reactor;
std::mutex reactor_mutex;
bool running = true;

// Background simulation thread (20 Hz update rate)
void simulation_loop() {
    auto last_time = std::chrono::steady_clock::now();
    while (running) {
        auto current_time = std::chrono::steady_clock::now();
        std::chrono::duration<double> elapsed = current_time - last_time;
        last_time = current_time;

        {
            std::lock_guard<std::mutex> lock(reactor_mutex);
            reactor.update(elapsed.count());
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

// Helper to load static file
std::string load_file(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        file.open("../" + path);
    }
    if (!file.is_open()) return "";
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// Simple JSON body value extractor
double extract_json_value(const std::string& body, const std::string& key) {
    size_t pos = body.find("\"" + key + "\"");
    if (pos == std::string::npos) return 0.0;
    pos = body.find(':', pos);
    if (pos == std::string::npos) return 0.0;
    size_t end = body.find_first_of(",}", pos);
    std::string val_str = body.substr(pos + 1, end - pos - 1);
    // Trim whitespace
    val_str.erase(0, val_str.find_first_not_of(" \t\n\r"));
    val_str.erase(val_str.find_last_not_of(" \t\n\r") + 1);
    try {
        return std::stod(val_str);
    } catch (...) {
        return 0.0;
    }
}

void handle_client(int client_socket) {
    char buffer[4096];
    memset(buffer, 0, sizeof(buffer));
    ssize_t bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received <= 0) {
        close(client_socket);
        return;
    }

    std::string request(buffer);
    std::string method, path;
    std::stringstream req_stream(request);
    req_stream >> method >> path;

    // Find body if POST
    std::string body = "";
    size_t header_end = request.find("\r\n\r\n");
    if (header_end != std::string::npos) {
        body = request.substr(header_end + 4);
    }

    std::string response_content;
    std::string content_type = "text/plain";
    int status_code = 200;

    if (method == "GET" && (path == "/" || path == "/index.html")) {
        response_content = load_file("public/index.html");
        content_type = "text/html; charset=utf-8";
        if (response_content.empty()) {
            status_code = 404;
            response_content = "404 Not Found";
        }
    } 
    else if (method == "GET" && path == "/api/status") {
        std::lock_guard<std::mutex> lock(reactor_mutex);
        std::ostringstream json;
        json << "{"
             << "\"power\":" << reactor.getPower() << ","
             << "\"fuel_temp\":" << reactor.getFuelTemp() << ","
             << "\"coolant_temp\":" << reactor.getCoolantTemp() << ","
             << "\"reactivity\":" << reactor.getTotalReactivity() << ","
             << "\"rod_pos\":" << reactor.getControlRodPos() << ","
             << "\"scrammed\":" << (reactor.isScrammed() ? "true" : "false")
             << "}";
        response_content = json.str();
        content_type = "application/json";
    } 
    else if (method == "POST" && path == "/api/control") {
        std::lock_guard<std::mutex> lock(reactor_mutex);
        if (body.find("rod_pos") != std::string::npos) {
            double pos = extract_json_value(body, "rod_pos");
            reactor.setControlRodPos(pos);
        } else if (body.find("delta_pcm") != std::string::npos) {
            double delta = extract_json_value(body, "delta_pcm");
            reactor.adjustControlRods(delta);
        }

        std::ostringstream json;
        json << "{"
             << "\"status\":\"success\","
             << "\"rod_pos\":" << reactor.getControlRodPos() << ","
             << "\"reactivity\":" << reactor.getTotalReactivity()
             << "}";
        response_content = json.str();
        content_type = "application/json";
    } 
    else if (method == "POST" && path == "/api/scram") {
        std::lock_guard<std::mutex> lock(reactor_mutex);
        reactor.scram();
        response_content = "{\"status\":\"scrammed\"}";
        content_type = "application/json";
    } 
    else if (method == "POST" && path == "/api/reset") {
        std::lock_guard<std::mutex> lock(reactor_mutex);
        reactor.reset();
        response_content = "{\"status\":\"reset\"}";
        content_type = "application/json";
    } 
    else {
        status_code = 404;
        response_content = "Not Found";
    }

    std::ostringstream http_resp;
    http_resp << "HTTP/1.1 " << status_code << (status_code == 200 ? " OK" : " Not Found") << "\r\n"
              << "Content-Type: " << content_type << "\r\n"
              << "Content-Length: " << response_content.size() << "\r\n"
              << "Connection: close\r\n\r\n"
              << response_content;

    std::string resp_str = http_resp.str();
    send(client_socket, resp_str.c_str(), resp_str.size(), 0);
    close(client_socket);
}

int main() {
    // Start background simulation thread
    std::thread sim_thread(simulation_loop);

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        std::cerr << "Failed to create socket" << std::endl;
        return 1;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "Failed to bind port 8080" << std::endl;
        return 1;
    }

    if (listen(server_fd, 10) < 0) {
        std::cerr << "Failed to listen on socket" << std::endl;
        return 1;
    }

    std::cout << "PWR Simulator Backend started on http://localhost:8080" << std::endl;

    while (running) {
        sockaddr_in client_addr{};
        socklen_t client_len = sizeof(client_addr);
        int client_socket = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        if (client_socket < 0) {
            if (!running) break;
            continue;
        }

        // Handle each client connection in a detached thread or synchronously
        std::thread(handle_client, client_socket).detach();
    }

    close(server_fd);
    running = false;
    sim_thread.join();
    return 0;
}
