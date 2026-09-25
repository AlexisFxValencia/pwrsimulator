#ifndef REACTOR_HPP
#define REACTOR_HPP

#include <string>

class Reactor {
public:
    Reactor();

    // Advance simulation by dt seconds
    void update(double dt);

    // Control actions
    void adjustControlRods(double delta_pcm); // Adjust rod reactivity in pcm
    void setControlRodPos(double pos);          // Set absolute rod position (0.0 = fully inserted, 1.0 = fully withdrawn)
    void scram();                               // Emergency shutdown

    // Getters for state representation
    double getPower() const { return power_; }                   // Normalized power (1.0 = 100%)
    double getFuelTemp() const { return fuel_temp_; }            // Fuel temperature (°C)
    double getCoolantTemp() const { return coolant_temp_; }      // Coolant temperature (°C)
    double getTotalReactivity() const { return total_reactivity_pcm_; } // Total reactivity (pcm)
    double getControlRodPos() const { return rod_pos_; }         // Rod position (0.0 to 1.0)
    double getDelayedNeutronPrecursor() const { return precursor_; }
    bool isScrammed() const { return is_scrammed_; }

    // Reset simulation
    void reset();

private:
    // Core state variables
    double power_;          // Relative power (n)
    double precursor_;      // Delayed neutron precursor concentration (C)
    double fuel_temp_;      // Fuel temperature (T_f) in °C
    double coolant_temp_;   // Coolant temperature (T_c) in °C
    double rod_pos_;        // Control rod position (0.0 inserted, 1.0 withdrawn)
    bool is_scrammed_;

    // Reactivity components (in pcm, 1 pcm = 10^-5 Δk/k)
    double rod_reactivity_pcm_;
    double doppler_reactivity_pcm_;
    double moderator_reactivity_pcm_;
    double total_reactivity_pcm_;

    // Reactor physics constants (PWR parameters)
    const double beta_ = 0.0065;          // Total delayed neutron fraction
    const double lambda_ = 0.08;          // Delayed neutron decay constant (s^-1)
    const double generation_time_ = 2e-5; // Neutron generation time Λ (s)

    // Temperature coefficients
    const double alpha_doppler_ = -2.5;   // pcm / °C (Doppler feedback)
    const double alpha_mod_ = -5.0;       // pcm / °C (Moderator temperature coefficient)

    // Thermal-hydraulics constants
    const double nominal_power_mw_ = 3000.0; // Thermal MW (typical 3-loop PWR)
    const double tf_nominal_ = 600.0;     // Nominal fuel temp (°C)
    const double tc_nominal_ = 315.0;     // Nominal coolant temp (°C)
    
    // Time constants and heat transfer coefficients
    const double tau_f_ = 5.0;            // Fuel thermal time constant (s)
    const double tau_c_ = 3.0;            // Coolant thermal time constant (s)
    const double heat_transfer_coeff_ = 0.15; // Coupling between fuel and coolant
    const double coolant_cooling_coeff_ = 0.20; // Heat removal by steam generator / secondary loop
    const double t_inlet_ = 290.0;        // Core inlet temperature (°C)
};

#endif // REACTOR_HPP
