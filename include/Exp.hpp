#ifndef EXP_HPP
#define EXP_HPP

#include "UnitaSI.hpp"

struct configExp {
    double V_ = -65.0 * mV;
    double V_th = -50.0 * mV;
    double V_ThresholdSpikeMax = -35.0 * mV;
    double V_rest = -65.0 * mV;
    double V_reset = -70.0 * mV;
    double deltaT = 2.5 * mV;
    double R = 1.0 * Mohm;
    double C = 100.0 * p * F;
    double timeAbsolute = 5.0 * ms;
    double timeRelative = 15.0 * ms;
    // altro....
};

/*
 * Neurone — modello Exponetial Leaky Integrate-and-Fire (expLIF)
 *
 * Modello matematico:
 *   tau * dV/dt = -(V - Vrest) + R * I_tot(t)
 *   tau = R * C
 *
 * Quando V raggiunge Vth il neurone emette uno spike:
 * V viene resettato a Vreset e inizia il periodo refrattario assoluto di durata tauR,
 * durante il quale la dinamica è bloccata.
 *
 * Parametri biologici di default (costruttore con solo ID):
 *   Vrest  = -65 mV,  Vth = -50 mV,  Vreset = -70 mV
 *   R = 1 MOhm,  C = 100 pF  ->  tau = 100 ms
 *   tauR = 5 ms
 *
 * Metodi pubblici:
 *   update(I, dt)   — avanza lo stato del neurone di un passo dt
 *
 * Metodi getter:
 *   hasFired()      — true se il neurone ha sparato nell'ultimo step
 *   getPotential()  — restituisce V corrente [V]
 *   getId()         — restituisce l'ID del neurone
 */
class Exp {

  private:
    int id_;
    double V_;             // potenziale di membrana [V]
    double Vth_;           // soglia di firing dinamica [V]
    double Vth0_;          // soglia di firing fuori dal tempo refrattario relativo [V]
    double VthSpikeMax_;   // soglia massima di firing quando avviene uno spike [V]
    double Vrest_;         // potenziale di riposo [V]
    double Vreset_;        // potenziale di reset post-spike [V]
    double R_;             // resistenza di ingresso [Ohm]
    double C_;             // capacità di membrana [F]
    double tau_;           // costante di tempo tau = R*C [s]
    double timeAbsolute_;  // durata periodo refrattario assoluto [s]
    double timeRelative_;  // durata periodo refrattario relativo [s]
    double tempoRR_;       // tempo refrattario assoluto rimanente [s]
    double tauRelative_;   // tau decadimento Vth_;
    double DeltaT_;        // delta esponenziale --> non mi ricordo bene il modello exponential LIF
    bool fired_;           // true se il neurone ha sparato nell'ultimo step
    char tipoIntegratore_; // tipo di integratore da utilizzare: 'E' = Eulero in avanti, 'R' = Runge-Kutta 4

    // metodi privati di integrazione
    void euleroInAvanti(double correnteTotale, double dt);
    void rungeKutta(double correnteTotale, double dt);

    void update(double correnteTotale, double dt);

    bool hasFired() const { return fired_; }
    double getPotential() const { return V_; }
    int getId() const { return id_; }

  public:
    ~Exp() = default;

    Exp(int id, char typeIntegratore, configExp config)
        : id_(id), V_(config.V_), Vth_(config.V_th), Vth0_(config.V_th), VthSpikeMax_(config.V_ThresholdSpikeMax),
          Vrest_(config.V_rest), Vreset_(config.V_reset), R_(config.R), C_(config.C), tau_(config.R * config.C),
          timeAbsolute_(config.timeAbsolute), timeRelative_(config.timeRelative), tempoRR_(0.0),
          tauRelative_(config.timeRelative / 3), DeltaT_(config.deltaT), fired_(false),
          tipoIntegratore_(typeIntegratore) {}

    friend class Rete;
};

#endif // NEURONE_HPP