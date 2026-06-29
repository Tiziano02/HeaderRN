#ifndef LIF_HPP
#define LIF_HPP

#include "UnitaSI.hpp"

/*
 * configLIF — parametri di configurazione per il neurone LIF.
 *
 * Passata al costruttore di LIF e a Rete::modificaParametriNeurone().
 * Tutti i valori sono in unità SI (vedi UnitaSI.hpp).
 *
 * Attributi:
 *   V_                  — potenziale iniziale di membrana  [V]   default: -65 mV
 *   V_th                — soglia di firing a riposo        [V]   default: -50 mV
 *   V_ThresholdSpikeMax — soglia massima dopo uno spike    [V]   default: -35 mV
 *   V_rest              — potenziale di riposo             [V]   default: -65 mV
 *   V_reset             — potenziale di reset post-spike   [V]   default: -70 mV
 *   R                   — resistenza di ingresso           [Ohm] default:  1 MOhm
 *   C                   — capacità di membrana             [F]   default: 100 pF  -> tau = 100 ms
 *   timeAbsolute        — durata refrattarietà assoluta    [s]   default:   5 ms
 *   timeRelative        — durata refrattarietà relativa    [s]   default:  15 ms
 */
struct configLIF {
    double V_ = -65.0 * mV;
    double V_th = -50.0 * mV;
    double V_ThresholdSpikeMax = -35.0 * mV;
    double V_rest = -65.0 * mV;
    double V_reset = -70.0 * mV;
    double R = 1.0 * Mohm;
    double C = 100.0 * p * F;
    double timeAbsolute = 5.0 * ms;
    double timeRelative = 15.0 * ms;
};

/*
 * LIF — neurone Leaky Integrate-and-Fire con refrattarietà assoluta e relativa.
 *
 * Modello matematico:
 *   tau * dV/dt = -(V - Vrest) + R * I_tot(t)     tau = R * C
 *
 * Quando V >= Vth il neurone emette uno spike: V viene resettato a Vreset,
 * la soglia Vth viene alzata istantaneamente a VthSpikeMax e inizia il
 * periodo refrattario assoluto. Terminato quello, Vth decade esponenzialmente
 * verso Vth0 con costante tauRelative/3 (refrattarietà relativa).
 *
 * Nota: LIF è friend di Rete. Tutti i metodi sono privati e vengono
 * chiamati esclusivamente da Rete::step(). Non istanziare direttamente:
 * usare Rete::aggiungiNeurone() o il costruttore di Rete.
 *
 * Integratori disponibili (passati come char al costruttore):
 *   'E' — Eulero in avanti  (stabile se dt << tau, tipicamente dt < tau/10)
 *   'R' — Runge-Kutta 4     (più accurato, utile per dt grandi)
 */
class LIF {

    // ── ATTRIBUTI ────────────────────────────────────────────────────────────

  private:
    int id_;               // identificatore univoco del neurone
    double V_;             // potenziale di membrana [V]
    double Vth_;           // soglia di firing dinamica [V]
    double Vth0_;          // soglia di firing a riposo (fuori dalla refrattarietà relativa) [V]
    double VthSpikeMax_;   // soglia massima raggiunta subito dopo uno spike [V]
    double Vrest_;         // potenziale di riposo [V]
    double Vreset_;        // potenziale di reset post-spike [V]
    double R_;             // resistenza di ingresso [Ohm]
    double C_;             // capacità di membrana [F]
    double tau_;           // costante di tempo tau = R*C [s]
    double timeAbsolute_;  // durata periodo refrattario assoluto [s]
    double timeRelative_;  // durata periodo refrattario relativo [s]
    double tempoRR_;       // tempo refrattario assoluto rimanente nello step corrente [s]
    double tauRelative_;   // costante di decadimento di Vth_ verso Vth0_ [s]
    bool fired_;           // true se il neurone ha sparato nell'ultimo step
    char tipoIntegratore_; // 'E' = Eulero in avanti, 'R' = Runge-Kutta 4

    // ── METODI PRIVATI ───────────────────────────────────────────────────────

    void euleroInAvanti(double correnteTotale, double dt); // integra dV/dt con Eulero in avanti
    void rungeKutta(double correnteTotale, double dt);     // integra dV/dt con Runge-Kutta 4

    void update(double correnteTotale, double dt); // avanza lo stato del neurone di un passo dt (chiamato da Rete)

    bool hasFired() const { return fired_; }   // true se il neurone ha sparato nell'ultimo step
    double getPotential() const { return V_; } // restituisce il potenziale di membrana corrente [V]
    int getId() const { return id_; }          // restituisce l'ID del neurone

    // ── COSTRUTTORE / DISTRUTTORE ─────────────────────────────────────────────

  public:
    /*
     * Costruisce un neurone LIF con ID, integratore e configurazione specificati.
     * Usare 'E' per Eulero in avanti o 'R' per Runge-Kutta 4.
     * In genere non si chiama direttamente: usare Rete::aggiungiNeurone().
     */
    LIF(int id, char typeIntegratore, configLIF config)
        : id_(id), V_(config.V_), Vth_(config.V_th), Vth0_(config.V_th), VthSpikeMax_(config.V_ThresholdSpikeMax),
          Vrest_(config.V_rest), Vreset_(config.V_reset), R_(config.R), C_(config.C), tau_(config.R * config.C),
          timeAbsolute_(config.timeAbsolute), timeRelative_(config.timeRelative), tempoRR_(0.0),
          tauRelative_(config.timeRelative / 3), fired_(false), tipoIntegratore_(typeIntegratore) {}

    ~LIF() = default;

    friend class Rete;
};

#endif // LIF_HPP