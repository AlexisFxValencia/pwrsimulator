#include "simulation.hpp"
#include "server.hpp"
#include <iostream>
#include <chrono>
#include <thread>
#include <csignal>

std::atomic<bool> keep_running(true);

void signal_handler(int) {
    keep_running = false;
}

int main() {
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    SimulationEngine engine;
    engine.start();

    HttpServer server(engine, 8080);
    if (!server.start()) {
        engine.stop();
        return 1;
    }

    std::cout << "App running. Press Ctrl+C to exit." << std::endl;

    while (keep_running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::cout << "Shutting down gracefully..." << std::endl;
    server.stop();
    engine.stop();

    return 0;
}
