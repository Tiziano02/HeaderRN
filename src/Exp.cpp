#include "Exp.hpp"
#include <cmath>

/*
 * update — avanza lo stato del neurone di un passo dt.
 *
 * Sequenza:
 *  1. Reset del flag fired_.
 *  2. Se il neurone è in periodo refrattario: decrementa tempoRR_ ed esce.
 *  3. Integrazione LIF con metodo di Eulero in avanti:
 *       V(t+dt) = V(t) + (dt/tau) * [-(V(t) - Vrest) + R * I_tot]
 *  4. Se V >= Vth: spike -> fired_ = true, V = Vreset, avvia periodo refrattario.
 *
 * Note:
 *  - fired_ è valido solo per lo step corrente; viene letto da Rete::step()
 *    prima del prossimo update().
 *  - Il metodo di Eulero è stabile se dt << tau (tipicamente dt < tau/10).
 */
void Exp::euleroInAvanti(double correnteTotale, double dt) {

    double termExp = DeltaT_ * std::exp((V_ - Vth_) / DeltaT_);
    double dV = (-(V_ - Vrest_) + termExp + R_ * correnteTotale) / tau_;
    V_ += dV * dt;
}

void Exp::rungeKutta(double correnteTotale, double dt) {
    // Funzione lambda locale per calcolare f(V, I) in modo pulito
    auto f = [this, correnteTotale](double v) {
        double termExp = DeltaT_ * std::exp((v - Vth_) / DeltaT_);
        return (-(v - Vrest_) + termExp + R_ * correnteTotale) / tau_;
    };

    // Calcolo dei 4 coefficienti di Runge-Kutta
    double k1 = f(V_);
    double k2 = f(V_ + 0.5 * dt * k1);
    double k3 = f(V_ + 0.5 * dt * k2);
    double k4 = f(V_ + dt * k3);

    // Aggiornamento finale del potenziale di membrana
    V_ += (dt / 6.0) * (k1 + 2.0 * k2 + 2.0 * k3 + k4);
}

void Exp::update(double correnteTotale, double dt) {
    fired_ = false;

    // 1. Il decadimento della soglia
    Vth_ += dt * ((Vth0_ - Vth_) / tauRelative_);

    // 2. Gestione Refrattarietà Assoluta
    if (tempoRR_ > 0.0) {
        tempoRR_ -= dt;
        if (tempoRR_ < 0.0)
            tempoRR_ = 0.0;
        V_ = Vreset_; // mantiene il potenziale inchiodato a reset
        return;       // Salta l'integrazione fisica: il neurone è sordo
    }

    // 3. Integrazione del Potenziale (Fase normale o Refrattarietà Relativa)
    if (tipoIntegratore_ == 'E') {
        euleroInAvanti(correnteTotale, dt);
    } else if (tipoIntegratore_ == 'R') {
        rungeKutta(correnteTotale, dt);
    }

    // 4. Controllo del Firing
    if (V_ >= Vth_) {
        fired_ = true;
        V_ = Vreset_;
        tempoRR_ = timeAbsolute_; // Fissa il tempo assoluto
        Vth_ = VthSpikeMax_;      // Alza la soglia al massimo istantaneamente!
    }
}