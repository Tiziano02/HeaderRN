#ifndef CONDUCTANCESYN_HPP
#define CONDUCTANCESYN_HPP

#include <cmath>
#include <cstddef>
#include <vector>

struct configConductanceSyn {
    double peso = 1.0;
    double gpeak = 1e-9; // conduttanza di picco per peso unitario [S]
    double gsyn = 0.0;   // conduttanza iniziale [S]
    double tau = 5e-3;   // costante di tempo del decadimento [s]
    double delay = 1e-3; // ritardo sinaptico [s]
    double E_rev = 0.0;  // potenziale di inversione [V]
                         //   ~  0 mV  -> eccitatoria (AMPA/NMDA)
                         //   ~ -70 mV -> inibitoria  (GABA-A)
};

/*
 * ConductanceSyn — sinapsi conductance-based con decadimento esponenziale.
 *
 * Modello matematico:
 *   dg_syn/dt = -g_syn / tau
 *
 * Ad ogni spike pre-sinaptico:
 *   g_syn -> g_syn + peso * gpeak
 *
 * La corrente iniettata nel neurone post-sinaptico è:
 *   I_syn = g_syn * (V_post - E_rev)
 *
 * Il segno emerge naturalmente dalla differenza (V_post - E_rev):
 *   E_rev >  V_post  ->  I_syn < 0  ->  inibitoria
 *   E_rev <  V_post  ->  I_syn > 0  ->  eccitatoria
 *
 * Nota: a differenza di CurrentSyn, update() richiede V_post perché
 * la corrente dipende dal potenziale del neurone post-sinaptico.
 *
 * Firma di update:
 *   update(dt, preFired, V_post)
 */
class ConductanceSyn {

  private:
    int idPre_, idPost_;
    size_t indexPre_, indexPost_;
    double peso_;
    double gpeak_;
    double gsyn_; // conduttanza sinaptica corrente [S]
    double Isyn_; // corrente sinaptica corrente [A]  (= gsyn_ * (V_post - E_rev))
    double tau_;
    double delay_;
    double E_rev_;

    size_t presentStep_;
    size_t delayStep_;
    std::vector<double> delayRing_;

    // metodi operativi
    void update(double dt, bool preFired, double V_post);

    // metodi setter
    void setIndexPre(size_t idx) { indexPre_ = idx; }
    void setIndexPost(size_t idx) { indexPost_ = idx; }
    void setDelayRing(double dt) {
        delayStep_ = static_cast<size_t>(std::round(delay_ / dt));
        delayRing_.assign(delayStep_ + 1, 0.0);
    }

    // metodi getter
    double getCurrent() const { return Isyn_; }
    int getIdPre() const { return idPre_; }
    int getIdPost() const { return idPost_; }
    size_t getIndexPre() const { return indexPre_; }
    size_t getIndexPost() const { return indexPost_; }

  public:
    ConductanceSyn(size_t indexPre, size_t indexPost, int idPre, int idPost, configConductanceSyn config) : idPre_(idPre), idPost_(idPost), indexPre_(indexPre), indexPost_(indexPost), peso_(config.peso), gpeak_(config.gpeak), gsyn_(0.0), Isyn_(0.0), tau_(config.tau), delay_(config.delay), E_rev_(config.E_rev), presentStep_(0), delayStep_(0) {}

    ~ConductanceSyn() = default;

    friend class Rete;
};

#endif // CONDUCTANCESYN_HPP
