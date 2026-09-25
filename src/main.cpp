#include "httplib.h"
#include "nlohmann/json.hpp"
#include "reactor.hpp"

#include <iostream>
#include <thread>
#include <chrono>
#include <mutex>
#include <fstream>
#include <sstream>

using json = nlohmann::json;

// Global reactor instance and synchronization mutex
Reactor reactor;
std::mutex reactor_mutex;
bool running = true;

// Background simulation thread (e.g. 20 Hz update rate)
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

        std::this_thread::sleep_for(std::chrono::milliseconds(50)); // 20 Hz
    }
}

// Helper to load static file
std::string load_file(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) return "";
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

int main() {
    // Start background simulation thread
    std::thread sim_thread(simulation_loop);

    httplib::Server svr;

    // Serve index.html at root
    svr.Get("/", [](const httplib::Request&, httplib::Response& res) {
        std::string html = load_file("public/index.html");
        if (html.empty()) {
            res.set_content("<html><body><h1>Error: public/index.html not found</h1></body></html>", "text/html");
        } else {
            res.set_content(html, "text/html");
        }
    });

    // GET /api/status - Retrieve reactor status
    svr.Get("/api/status", [](const httplib::Request&, httplib::Response& res) {
        std::lock_guard<std::mutex> lock(reactor_mutex);
        json j;
        j["power"] = reactor.getPower();
        j["fuel_temp"] = reactor.getFuelTemp();
        j["coolant_temp"] = reactor.getCoolantTemp();
        j["reactivity"] = reactor.getTotalReactivity();
        j["rod_pos"] = reactor.getControlRodPos();
        j["scrammed"] = reactor.isScrammed();
        res.set_content(j.dump(), "application/json");
    });

    // POST /api/control - Adjust control rods or reactivity
    svr.Post("/api/control", [](const httplib::Request& req, httplib::Response& res) {
        try {
            json j = json::parse(req.body);
            std::lock_guard<std::mutex> lock(reactor_mutex);

            if (j.contains("rod_pos")) {
                double pos = j["rod_pos"];
                reactor.setControlRodPos(pos);
            } else if (j.contains("delta_pcm")) {
                double delta = j["delta_pcm"];
                reactor.adjustControlRods(delta);
            }

            json resp;
            resp["status"] = "success";
            resp["rod_pos"] = reactor.getControlRodPos();
            resp["reactivity"] = reactor.getTotalReactivity();
            res.set_content(resp.dump(), "application/json");
        } catch (...) {
            res.status = 400;
            res.set_content("{\"error\": \"Invalid JSON request\"}", "application/json");
        }
    });

    // POST /api/scram - Emergency shutdown
    svr.Post("/api/scram", [](const httplib::Request&, httplib::Response& res) {
        std::lock_guard<std::mutex> lock(reactor_mutex);
        reactor.scram();
        json j;
        j["status"] = "scrammed";
        j["power"] = reactor.getPower();
        res.set_content(j.dump(), "application/json");
    });

    // POST /api/reset - Reset simulation
    svr.Post("/api/reset", [](const httplib::Request&, httplib::Response& res) {
        std::lock_guard<std::mutex> lock(reactor_mutex);
        reactor.reset();
        json j;
        j["status"] = "reset";
        res.set_content(j.dump(), "application/json");
    });

    std::cout << "PWR Simulator Backend started on http://localhost:8080" << std::endl;
    svr.listen("0.0.0.0", 8080);

    running = false;
    sim_thread.join();
    return 0;
}
