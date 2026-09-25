#include "reactor.hpp"
#include <algorithm>
#include <cmath>

Reactor::Reactor() {
    reset();
}

void Reactor::reset() {
    power_ = 1.0;          // 100% initial power
    precursor_ = 1.0;      // Equilibrium precursor concentration
    fuel_temp_ = 600.0;    // °C
    coolant_temp_ = 315.0; // °C
    rod_pos_ = 0.5;        // 50% withdrawn (critical at steady state)
    is_scrammed_ = false;

    rod_reactivity_pcm_ = 0.0;
    doppler_reactivity_pcm_ = alpha_doppler_ * (fuel_temp_ - tf_nominal_);
    moderator_reactivity_pcm_ = alpha_mod_ * (coolant_temp_ - tc_nominal_);
    total_reactivity_pcm_ = rod_reactivity_pcm_ + doppler_reactivity_pcm_ + moderator_reactivity_pcm_;
}

void Reactor::setControlRodPos(double pos) {
    if (is_scrammed_) return;
    rod_pos_ = std::clamp(pos, 0.0, 1.0);
    // Map rod position [0, 1] to reactivity [-3500 pcm, +3500 pcm]
    rod_reactivity_pcm_ = (rod_pos_ - 0.5) * 7000.0;
}

void Reactor::adjustControlRods(double delta_pcm) {
    if (is_scrammed_) return;
    rod_reactivity_pcm_ += delta_pcm;
    rod_pos_ = std::clamp((rod_reactivity_pcm_ / 7000.0) + 0.5, 0.0, 1.0);
}

void Reactor::scram() {
    is_scrammed_ = true;
    rod_pos_ = 0.0;
    rod_reactivity_pcm_ = -5000.0; // Large negative reactivity drop
}

void Reactor::update(double total_dt) {
    // Sub-stepping for numerical stability of stiff point kinetics equations
    int sub_steps = 50;
    double dt = total_dt / sub_steps;

    for (int i = 0; i < sub_steps; ++i) {
        // 1. Update feedback reactivities
        doppler_reactivity_pcm_ = alpha_doppler_ * (fuel_temp_ - tf_nominal_);
        moderator_reactivity_pcm_ = alpha_mod_ * (coolant_temp_ - tc_nominal_);
        
        if (is_scrammed_) {
            rod_reactivity_pcm_ = -5000.0;
        }

        total_reactivity_pcm_ = rod_reactivity_pcm_ + doppler_reactivity_pcm_ + moderator_reactivity_pcm_;
        
        // Convert pcm to dimensionless Δk/k
        double rho = total_reactivity_pcm_ * 1e-5;

        // 2. Point Kinetics Equations (1 group delayed neutrons)
        // dn/dt = [(rho - beta) / Lambda] * n + lambda * C
        // dC/dt = (beta / Lambda) * n - lambda * C
        double dn_dt = ((rho - beta_) / generation_time_) * power_ + lambda_ * precursor_;
        double dC_dt = (beta_ / generation_time_) * power_ - lambda_ * precursor_;

        power_ += dn_dt * dt;
        precursor_ += dC_dt * dt;

        // Clamp power to prevent negative blowups during fast transients
        power_ = std::max(0.00001, power_);

        // 3. Thermal-Hydraulics (Fuel and Coolant temperatures)
        // Fuel heat balance: M_f C_pf dT_f/dt = Power_gen - Heat_transfer_to_coolant
        // dT_f/dt = ( (power_ * nominal_power) - H_tc * (T_f - T_c) ) / Thermal_capacity_f
        // Simplified temperature derivative models
        double dtf_dt = ( (power_ * tf_nominal_) - fuel_temp_ - heat_transfer_coeff_ * (fuel_temp_ - coolant_temp_) ) / tau_f_;
        
        // Coolant heat balance
        double dtc_dt = ( heat_transfer_coeff_ * (fuel_temp_ - coolant_temp_) - coolant_cooling_coeff_ * (coolant_temp_ - t_inlet_) ) / tau_c_;

        fuel_temp_ += dtf_dt * dt;
        coolant_temp_ += dtc_dt * dt;
    }
}
