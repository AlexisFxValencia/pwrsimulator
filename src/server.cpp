#include "server.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <thread>

HttpServer::HttpServer(SimulationEngine& engine, int port)
    : engine_(engine), port_(port), running_(false), server_fd_(-1) {}

HttpServer::~HttpServer() {
    stop();
}

bool HttpServer::start() {
    server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd_ == -1) {
        std::cerr << "Failed to create socket" << std::endl;
        return false;
    }

    int opt = 1;
    setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port_);

    if (bind(server_fd_, (struct sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "Failed to bind port " << port_ << std::endl;
        close(server_fd_);
        return false;
    }

    if (listen(server_fd_, 10) < 0) {
        std::cerr << "Failed to listen on socket" << std::endl;
        close(server_fd_);
        return false;
    }

    running_ = true;
    std::cout << "PWR Simulator Backend started on http://localhost:" << port_ << std::endl;

    // Start accept loop in background thread
    std::thread(&HttpServer::acceptLoop, this, server_fd_).detach();
    return true;
}

void HttpServer::stop() {
    if (!running_) return;
    running_ = false;
    if (server_fd_ != -1) {
        close(server_fd_);
        server_fd_ = -1;
    }
}

void HttpServer::acceptLoop(int server_fd) {
    while (running_) {
        sockaddr_in client_addr{};
        socklen_t client_len = sizeof(client_addr);
        int client_socket = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        if (client_socket < 0) {
            if (!running_) break;
            continue;
        }

        std::thread(&HttpServer::handleClient, this, client_socket).detach();
    }
}

static std::string load_file(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        file.open("../" + path);
    }
    if (!file.is_open()) return "";
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

static double extract_json_value(const std::string& body, const std::string& key) {
    size_t pos = body.find("\"" + key + "\"");
    if (pos == std::string::npos) return 0.0;
    pos = body.find(':', pos);
    if (pos == std::string::npos) return 0.0;
    size_t end = body.find_first_of(",}", pos);
    std::string val_str = body.substr(pos + 1, end - pos - 1);
    val_str.erase(0, val_str.find_first_not_of(" \t\n\r"));
    val_str.erase(val_str.find_last_not_of(" \t\n\r") + 1);
    try {
        return std::stod(val_str);
    } catch (...) {
        return 0.0;
    }
}

static bool extract_json_bool(const std::string& body, const std::string& key) {
    size_t pos = body.find("\"" + key + "\"");
    if (pos == std::string::npos) return false;
    pos = body.find(':', pos);
    if (pos == std::string::npos) return false;
    size_t end = body.find_first_of(",}", pos);
    std::string val_str = body.substr(pos + 1, end - pos - 1);
    return val_str.find("true") != std::string::npos;
}

void HttpServer::handleClient(int client_socket) {
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
        std::ostringstream json;
        json << "{"
             << "\"power\":" << engine_.getPower() << ","
             << "\"fuel_temp\":" << engine_.getFuelTemp() << ","
             << "\"coolant_temp\":" << engine_.getCoolantTemp() << ","
             << "\"reactivity\":" << engine_.getTotalReactivity() << ","
             << "\"rod_pos\":" << engine_.getControlRodPos() << ","
             << "\"sim_time\":" << engine_.getSimulationTime() << ","
             << "\"scrammed\":" << (engine_.isScrammed() ? "true" : "false") << ","
             << "\"paused\":" << (engine_.isPaused() ? "true" : "false") << ","
             << "\"time_scale\":" << engine_.getTimeScale()
             << "}";
        response_content = json.str();
        content_type = "application/json";
    } 
    else if (method == "POST" && path == "/api/control") {
        if (body.find("rod_pos") != std::string::npos) {
            engine_.setControlRodPos(extract_json_value(body, "rod_pos"));
        } 
        if (body.find("delta_pcm") != std::string::npos) {
            engine_.adjustControlRods(extract_json_value(body, "delta_pcm"));
        }
        if (body.find("paused") != std::string::npos) {
            engine_.setPaused(extract_json_bool(body, "paused"));
        }
        if (body.find("time_scale") != std::string::npos) {
            int ts = static_cast<int>(extract_json_value(body, "time_scale"));
            engine_.setTimeScale(ts);
        }

        std::ostringstream json;
        json << "{"
             << "\"status\":\"success\","
             << "\"rod_pos\":" << engine_.getControlRodPos() << ","
             << "\"reactivity\":" << engine_.getTotalReactivity() << ","
             << "\"paused\":" << (engine_.isPaused() ? "true" : "false") << ","
             << "\"time_scale\":" << engine_.getTimeScale()
             << "}";
        response_content = json.str();
        content_type = "application/json";
    } 
    else if (method == "POST" && path == "/api/scram") {
        engine_.scram();
        response_content = "{\"status\":\"scrammed\"}";
        content_type = "application/json";
    } 
    else if (method == "POST" && path == "/api/reset") {
        engine_.reset();
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
