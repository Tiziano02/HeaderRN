#ifndef CURRENTSYN_HPP
#define CURRENTSYN_HPP

#include <cmath>
#include <cstddef>
#include <vector>

/*
 * configCurrentSyn — parametri di configurazione per la sinapsi current-based.
 *
 * Passata a Rete::connettiNeuroni() e a Rete::modificaSinapsi().
 * Tutti i valori sono in unità SI (vedi UnitaSI.hpp).
 *
 * Attributi:
 *   peso   — scala il kick di corrente ad ogni spike        [-]  default:  1.0
 *   Ipeak  — corrente di picco per peso unitario            [A]  default:  100 pA
 *   Isyn   — corrente iniziale (solitamente lasciare a 0)   [A]  default:  0
 *   tau    — costante di tempo del decadimento esponenziale [s]  default:  5 ms
 *   delay  — ritardo sinaptico                              [s]  default:  1 ms
 *
 * Attenzione alla convenzione dei segni: in Rete::step() la corrente sinaptica
 * viene sottratta all'input totale del neurone post (inputTotale -= Isyn).
 * Quindi:
 *   peso > 0  ->  Isyn > 0  ->  inputTotale diminuisce  ->  inibitoria
 *   peso < 0  ->  Isyn < 0  ->  inputTotale aumenta     ->  eccitatoria
 */
struct configCurrentSyn {
    double peso = 1.0;
    double Ipeak = 1e-10; // [A]
    double Isyn = 0.0;    // [A]
    double tau = 5e-3;    // [s]
    double delay = 1e-3;  // [s]
};

/*
 * CurrentSyn — sinapsi current-based con decadimento esponenziale e delay.
 *
 * Modello matematico:
 *   dI_syn/dt = -I_syn / tau
 *
 * Ad ogni spike pre-sinaptico (con ritardo delay):
 *   I_syn -> I_syn + peso * Ipeak
 *
 * Il delay è implementato con un ring buffer: lo spike viene inserito
 * nella posizione futura corretta e recuperato dopo esattamente delayStep_ passi.
 *
 * A differenza di ConductanceSyn, la corrente non dipende dal potenziale
 * post-sinaptico: il segno e l'intensità dell'effetto sono determinati
 * interamente da peso e Ipeak. Vedi configCurrentSyn per la convenzione dei segni.
 *
 * Nota: CurrentSyn è friend di Rete. Tutti i metodi sono privati e vengono
 * chiamati esclusivamente da Rete::step(). Non istanziare direttamente:
 * usare Rete::connettiNeuroni().
 */
class CurrentSyn {

    // ── ATTRIBUTI ────────────────────────────────────────────────────────────

  private:
    int idPre_, idPost_;            // ID dei neuroni pre e post-sinaptico
    size_t indexPre_, indexPost_;   // indici interni in Rete::neuroni_
    double peso_;                   // scala il kick di corrente ad ogni spike
    double Ipeak_;                  // corrente di picco per peso unitario [A]
    double Isyn_;                   // corrente sinaptica corrente [A]
    double tau_;                    // costante di tempo del decadimento [s]
    double delay_;                  // ritardo sinaptico [s]
    size_t presentStep_;            // posizione corrente nel ring buffer
    size_t delayStep_;              // numero di step corrispondenti al delay
    std::vector<double> delayRing_; // ring buffer per la gestione del delay

    // ── METODI PRIVATI ───────────────────────────────────────────────────────

    void update(double dt, bool preFired); // avanza lo stato della sinapsi di un passo dt (chiamato da Rete)

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
     * Costruisce una sinapsi current-based tra due neuroni.
     * In genere non si chiama direttamente: usare Rete::connettiNeuroni().
     * Il ring buffer del delay viene allocato da Rete::prepare() al momento
     * dell'avvio della simulazione.
     */
    CurrentSyn(size_t indexPre, size_t indexPost, int idPre, int idPost, configCurrentSyn config)
        : idPre_(idPre), idPost_(idPost), indexPre_(indexPre), indexPost_(indexPost), peso_(config.peso),
          Ipeak_(config.Ipeak), Isyn_(0.0), tau_(config.tau), delay_(config.delay), presentStep_(0), delayStep_(0) {}

    ~CurrentSyn() = default;

    friend class Rete;
};

#endif // CURRENTSYN_HPP