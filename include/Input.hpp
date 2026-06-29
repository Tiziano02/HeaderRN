#ifndef INPUT_HPP
#define INPUT_HPP

#include <variant>

// ============================================================
// TIPI DI STIMOLO
// ============================================================

/**
 * Stimolo costante: fornisce una corrente costante per un intervallo di tempo.
 */
struct configConstantStimulus {
    double timeStart; // Istante di inizio [s]
    double timeEnd;   // Istante di fine [s]
    double ampiezza;  // Corrente [A]
};

/**
 * Stimolo sinusoidale: fornisce una corrente sinusoidale.
 */
struct configSinStimulus {
    double timeStart; // Istante di inizio [s]
    double timeEnd;   // Istante di fine [s]
    double ampiezza;  // Ampiezza della sinusoide [A]
    double frequenza; // Frequenza [Hz]
    double fase;      // Fase iniziale [rad]
};

// ============================================================
// TYPE-ALIAS PER USARE VARIANT
// ============================================================

/**
 * ParametriStimolo è un variant che può contenere uno dei tipi di stimolo.
 * Uniforma la gestione degli stimoli come per neuroni (LIF/Exp) e sinapsi
 * (CurrentSyn/ConductanceSyn).
 */
using typeParameters = std::variant<configConstantStimulus, configSinStimulus>;

/**
 * Struttura che associa uno stimolo al neurone target.
 * Sostituisce il vecchio registro + database multipli.
 */
struct stimolo {
    size_t indiceNeurone;     // Indice nel vettore neuroni_ (per accesso rapido)
    typeParameters parametri; // Parametri dello stimolo
};

#endif // INPUT_HPP