#ifndef SIMULAZIONE_HPP
#define SIMULAZIONE_HPP

#include "Rete.hpp"
#include "Input.hpp"

#include <fstream>

/**
 * Simulazione — Coordina la rete, gli stimoli esterni e l'output su file.
 *
 * La simulazione gestisce la griglia temporale e il loop principale.
 *
 * ============================================================================
 * STIMOLI ESTERNI
 * ============================================================================
 * La simulazione utilizza un unico vettore di StimoloCollegato per gestire
 * tutti gli stimoli esterni. Ogni stimolo è un std::variant che può contenere
 * diversi tipi (costante, sinusoidale, ecc.).
 *
 * Per aggiungere uno stimolo, usare iniettaStimoli().
 *
 * ============================================================================
 * OUTPUT
 * ============================================================================
 * I risultati vengono salvati in file binari con un header iniziale di 4 byte
 * che specifica il numero di colonne (tempo + dati). Usare gli script Python
 * forniti per leggere i file.
 */
class Simulazione {

  private:
    Rete rete_; // Rete da simulare

    // ---------- Stimoli Esterni ----------
    std::vector<stimolo> stimoli_; // Un unico vettore per tutti gli stimoli

    // ---------- Parametri Temporali ----------
    double dt_;        // Passo temporale [s]
    int stepCorrente_; // Step attuale
    int stepTotali_;   // Numero totale di step

    // ---------- I/O ----------
    std::ofstream filePotenziali_;
    std::ofstream fileFiring_;
    std::ofstream fileSinapsi_;
    std::string fileNameV_;
    std::string fileNameF_;
    std::string fileNameS_;

    std::vector<char> bufferV_;
    std::vector<char> bufferF_;
    std::vector<char> bufferS_;
    size_t bytesPerStepV_ = 0;
    size_t bytesPerStepF_ = 0;
    size_t bytesPerStepS_ = 0;
    size_t posizioneBuffer_ = 0;
    size_t stepsPerFlush_ = 0;

    // ---------- Metodi Interni ----------
    void inizializzaOutput();
    void loadStatoRete(double time);
    void writeFile();

    /**
     * Valuta tutti gli stimoli al tempo t e li applica alla rete.
     *
     * Itera su stimoli_ e usa std::visit per calcolare il valore corrente
     * di ogni stimolo in base al tipo (costante, sinusoidale, ecc.).
     */
    void valutaStimoli(double t);

  public:
    /**
     * Costruisce una simulazione.
     *
     * @param rete Rete da simulare (viene copiata internamente).
     * @param dt Passo temporale [s].
     * @param T Durata totale della simulazione [s].
     *
     * @warning Verifica che dt non sia troppo grande rispetto a tau_min.
     */
    Simulazione(const Rete& rete, double dt, double T);

    /**
     * Inietta una lista di stimoli nella simulazione.
     *
     * @param stimoli Vettore di coppie (ID neurone, parametri stimolo).
     *
     * @note I parametri possono essere StimoloCostante o StimoloSinusoidale.
     * @warning I neuroni devono esistere; altrimenti viene stampato un errore.
     *
     * Esempio:
     *   sim.iniettaStimoli({
     *       {0, StimoloCostante{0.0, 100.0*ms, 20e-9}},
     *       {2, StimoloSinusoidale{0.0, 100.0*ms, 10e-9, 40*Hz, 0.0}}
     *   });
     */
    void iniettaStimoli(const std::vector<stimolo>& stimoli);

    /**
     * Avvia la simulazione e salva i risultati su file binari.
     *
     * @param filenameV Nome file per i potenziali.
     * @param filenameF Nome file per i firing.
     * @param filenameS Nome file per le correnti sinaptiche.
     */
    void avviaSimulazione(const std::string& filenameV, const std::string& filenameF, const std::string& filenameS);

    /**
     * @return Durata totale della simulazione [s].
     */
    double getTempoTotale() const { return stepTotali_ * dt_; }

    ~Simulazione() = default;
};

#endif // SIMULAZIONE_HPP