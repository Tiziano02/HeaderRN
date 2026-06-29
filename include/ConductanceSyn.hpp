#ifndef CONDUCTANCESYN_HPP
#define CONDUCTANCESYN_HPP

#include <cmath>
#include <cstddef>
#include <vector>

/*
 * configConductanceSyn — parametri di configurazione per la sinapsi conductance-based.
 *
 * Passata a Rete::connettiNeuroni() e a Rete::modificaSinapsi().
 * Tutti i valori sono in unità SI (vedi UnitaSI.hpp).
 *
 * Attributi:
 *   peso   — scala il kick di conduttanza ad ogni spike        [-]  default:  1.0
 *   gpeak  — conduttanza di picco per peso unitario            [S]  default:  1 nS
 *   gsyn   — conduttanza iniziale (solitamente lasciare a 0)   [S]  default:  0
 *   tau    — costante di tempo del decadimento esponenziale    [s]  default:  5 ms
 *   delay  — ritardo sinaptico                                 [s]  default:  1 ms
 *   E_rev  — potenziale di inversione                          [V]  default:  0 V
 *
 * Il tipo di sinapsi si controlla tramite E_rev (la convenzione dei segni emerge
 * dalla fisica, vedi classe ConductanceSyn):
 *   E_rev ~  0 mV   ->  eccitatoria  (AMPA/NMDA)
 *   E_rev ~ -70 mV  ->  inibitoria   (GABA-A)
 */
struct configConductanceSyn {
    double peso = 1.0;
    double gpeak = 1e-9; // [S]
    double gsyn = 0.0;   // [S]
    double tau = 5e-3;   // [s]
    double delay = 1e-3; // [s]
    double E_rev = 0.0;  // [V]
};

/*
 * ConductanceSyn — sinapsi conductance-based con decadimento esponenziale e delay.
 *
 * Modello matematico:
 *   dg_syn/dt = -g_syn / tau
 *
 * Ad ogni spike pre-sinaptico (con ritardo delay):
 *   g_syn -> g_syn + peso * gpeak
 *
 * La corrente sinaptica è:
 *   I_syn = g_syn * (V_post - E_rev)
 *
 * In Rete::step() la corrente viene sottratta all'input totale (inputTotale -= Isyn).
 * Il segno dell'effetto emerge dalla combinazione di I_syn e questa sottrazione:
 *   E_rev ~  0 mV,  V_post ~ -65 mV  ->  I_syn < 0  ->  inputTotale aumenta  ->  eccitatoria
 *   E_rev ~ -70 mV, V_post ~ -65 mV  ->  I_syn > 0  ->  inputTotale diminuisce -> inibitoria
 *
 * A differenza di CurrentSyn, la corrente dipende dal potenziale post-sinaptico
 * corrente: update() richiede V_post come argomento aggiuntivo. Questo rende
 * il modello più realistico biologicamente (la forza trainante si riduce
 * man mano che V_post si avvicina a E_rev).
 *
 * Nota: ConductanceSyn è friend di Rete. Tutti i metodi sono privati e vengono
 * chiamati esclusivamente da Rete::step(). Non istanziare direttamente:
 * usare Rete::connettiNeuroni().
 */
class ConductanceSyn {

    // ── ATTRIBUTI ────────────────────────────────────────────────────────────

  private:
    int idPre_, idPost_;            // ID dei neuroni pre e post-sinaptico
    size_t indexPre_, indexPost_;   // indici interni in Rete::neuroni_
    double peso_;                   // scala il kick di conduttanza ad ogni spike
    double gpeak_;                  // conduttanza di picco per peso unitario [S]
    double gsyn_;                   // conduttanza sinaptica corrente [S]
    double Isyn_;                   // corrente sinaptica corrente [A]  (= gsyn_ * (V_post - E_rev))
    double tau_;                    // costante di tempo del decadimento [s]
    double delay_;                  // ritardo sinaptico [s]
    double E_rev_;                  // potenziale di inversione [V]
    size_t presentStep_;            // posizione corrente nel ring buffer
    size_t delayStep_;              // numero di step corrispondenti al delay
    std::vector<double> delayRing_; // ring buffer per la gestione del delay

    // ── METODI PRIVATI ───────────────────────────────────────────────────────

    void update(double dt, bool preFired,
                double V_post); // avanza lo stato della sinapsi di un passo dt (chiamato da Rete)

    void setIndexPre(size_t idx) { indexPre_ = idx; }   // aggiorna l'indice interno del neurone pre
    void setIndexPost(size_t idx) { indexPost_ = idx; } // aggiorna l'indice interno del neurone post
    void setDelayRing(double dt) {                      // alloca il ring buffer in base al dt della simulazione
        delayStep_ = static_cast<size_t>(std::round(delay_ / dt));
        delayRing_.assign(delayStep_ + 1, 0.0);
    }

    double getCurrent() const { return Isyn_; }        // restituisce la corrente sinaptica corrente [A]
    int getIdPre() const { return idPre_; }            // restituisce l'ID del neurone pre-sinaptico
    int getIdPost() const { return idPost_; }          // restituisce l'ID del neurone post-sinaptico
    size_t getIndexPre() const { return indexPre_; }   // restituisce l'indice interno del neurone pre
    size_t getIndexPost() const { return indexPost_; } // restituisce l'indice interno del neurone post

    // ── COSTRUTTORE / DISTRUTTORE ─────────────────────────────────────────────

  public:
    /*
     * Costruisce una sinapsi conductance-based tra due neuroni.
     * In genere non si chiama direttamente: usare Rete::connettiNeuroni().
     * Il ring buffer del delay viene allocato da Rete::prepare() al momento
     * dell'avvio della simulazione.
     */
    ConductanceSyn(size_t indexPre, size_t indexPost, int idPre, int idPost, configConductanceSyn config)
        : idPre_(idPre), idPost_(idPost), indexPre_(indexPre), indexPost_(indexPost), peso_(config.peso),
          gpeak_(config.gpeak), gsyn_(0.0), Isyn_(0.0), tau_(config.tau), delay_(config.delay), E_rev_(config.E_rev),
          presentStep_(0), delayStep_(0) {}

    ~ConductanceSyn() = default;

    friend class Rete;
};

#endif // CONDUCTANCESYN_HPP