#include "simulation.hpp"
#include <chrono>

SimulationEngine::SimulationEngine()
    : running_(false), is_paused_(false), time_scale_(1) {}

SimulationEngine::~SimulationEngine() {
    stop();
}

void SimulationEngine::start() {
    if (running_) return;
    running_ = true;
    sim_thread_ = std::thread(&SimulationEngine::runLoop, this);
}

void SimulationEngine::stop() {
    if (!running_) return;
    running_ = false;
    if (sim_thread_.joinable()) {
        sim_thread_.join();
    }
}

void SimulationEngine::runLoop() {
    while (running_) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (!is_paused_) {
                double fixed_dt = 0.05;
                int steps = time_scale_;
                if (steps < 1) steps = 1;
                for (int i = 0; i < steps; ++i) {
                    reactor_.update(fixed_dt);
                }
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

void SimulationEngine::setControlRodPos(double pos) {
    std::lock_guard<std::mutex> lock(mutex_);
    reactor_.setControlRodPos(pos);
}

void SimulationEngine::adjustControlRods(double delta_pcm) {
    std::lock_guard<std::mutex> lock(mutex_);
    reactor_.adjustControlRods(delta_pcm);
}

void SimulationEngine::scram() {
    std::lock_guard<std::mutex> lock(mutex_);
    reactor_.scram();
}

void SimulationEngine::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    reactor_.reset();
    is_paused_ = false;
    time_scale_ = 1;
}

void SimulationEngine::setPaused(bool paused) {
    is_paused_ = paused;
}

void SimulationEngine::setTimeScale(int scale) {
    if (scale >= 1) {
        time_scale_ = scale;
    }
}

double SimulationEngine::getPower() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return reactor_.getPower();
}

double SimulationEngine::getFuelTemp() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return reactor_.getFuelTemp();
}

double SimulationEngine::getCoolantTemp() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return reactor_.getCoolantTemp();
}

double SimulationEngine::getTotalReactivity() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return reactor_.getTotalReactivity();
}

double SimulationEngine::getControlRodPos() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return reactor_.getControlRodPos();
}

double SimulationEngine::getSimulationTime() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return reactor_.getSimulationTime();
}

bool SimulationEngine::isScrammed() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return reactor_.isScrammed();
}
