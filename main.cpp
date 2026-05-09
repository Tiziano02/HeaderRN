#include "Input.hpp"
#include "Rete.hpp"
#include "Simulazione.hpp"
#include "Sinapsi.hpp"
#include "UnitaSI.hpp"

#include <iostream>
#include <vector>

int main() {

    // =========================================================
    // PARAMETRI RETE
    // =========================================================

    const int N = 20;

    // =========================================================
    // CREAZIONE RETE
    // =========================================================

    Rete rete;

    // creo i neuroni
    for (int i = 0; i < N; ++i) {

        Neurone neurone(
            i,                // id
            -65.0 * mV,       // V iniziale
            -50.0 * mV,       // soglia
            -65.0 * mV,       // riposo
            -70.0 * mV,       // reset
            1.0 * M * Ohm,    // resistenza
            100.0 * p * F,    // capacità
            5.0 * mS          // refrattario
        );

        rete.aggiungiNeurone(neurone);
    }

    // =========================================================
    // CONNESSIONI RING
    // =========================================================

    for (int i = 0; i < N; ++i) {

        int next = (i + 1) % N;

        Sinapsi s(
            1.0,              // peso
            18.0 * n * A,     // Ipeak
            i,                // pre
            next,             // post
            5.0 * mS          // tau sinaptica
        );

        rete.connettiNeuroni(s);
    }

    // =========================================================
    // PARAMETRI SIMULAZIONE
    // =========================================================

    double dt = 0.1 * mS;
    double T  = 500.0 * mS;

    Simulazione sim(rete, dt, T);

    // =========================================================
    // INPUT ESTERNO
    // =========================================================
    // Stimolo SOLO il neurone 0
    // per innescare l'onda nel ring

    int nSteps = static_cast<int>(T / dt);

    std::vector<double> inputN0(nSteps, 0.0);

    // impulso iniziale
    for (int i = 10; i < 80; ++i) {
        inputN0[i] = 25.0 * n * A;
    }

    Input in0;
    in0.id = 0;
    in0.valori = inputN0;

    std::vector<Input> inputs = {in0};

    sim.aggiungiInputEsterni(inputs);

    // =========================================================
    // AVVIO SIMULAZIONE
    // =========================================================

    sim.avviaSimulazione(
        "potenziali.txt",
        "firing.txt",
        "sinapsi.txt"
    );

    std::cout << "Simulazione completata.\n";

    return 0;
}