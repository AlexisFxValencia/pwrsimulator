#ifndef SIMULATION_HPP
#define SIMULATION_HPP

#include "reactor.hpp"
#include <mutex>
#include <thread>
#include <atomic>

class SimulationEngine {
public:
    SimulationEngine();
    ~SimulationEngine();

    void start();
    void stop();

    // Reactor control proxies (thread-safe)
    void setControlRodPos(double pos);
    void adjustControlRods(double delta_pcm);
    void scram();
    void reset();

    // Simulation state controls
    void setPaused(bool paused);
    void setTimeScale(int scale);

    bool isPaused() const { return is_paused_; }
    int getTimeScale() const { return time_scale_; }

    // State getters (thread-safe)
    double getPower() const;
    double getFuelTemp() const;
    double getCoolantTemp() const;
    double getTotalReactivity() const;
    double getControlRodPos() const;
    double getSimulationTime() const;
    bool isScrammed() const;

private:
    void runLoop();

    Reactor reactor_;
    mutable std::mutex mutex_;
    std::thread sim_thread_;
    std::atomic<bool> running_;
    std::atomic<bool> is_paused_;
    std::atomic<int> time_scale_;
};

#endif // SIMULATION_HPP
