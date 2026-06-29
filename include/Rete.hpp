#ifndef RETE_HPP
#define RETE_HPP

#include "Neurone.hpp"
#include "Sinapsi.hpp"

#include <algorithm>
#include <cstddef>
#include <unordered_map>
#include <vector>

/*
 * Rete — Contenitore della topologia e motore di evoluzione della rete neurale.
 *
 * La rete è rappresentata come:
 *   - un vettore di neuroni  (TypeNeuron  = variant<LIF, Exp>)
 *   - un vettore di sinapsi  (TypeSyn     = variant<CurrentSyn,
 * ConductanceSyn>)
 *   - mappe per accesso rapido ID -> indice
 *   - buffer pre-allocati per correnti e stato
 *
 * La scelta di una lista di sinapsi invece di una matrice di adiacenza
 * semplifica l'aggiunta futura di delay e plasticità sinaptica.
 *
 * ============================================================================
 * COSTRUTTORE
 * ============================================================================
 * Rete(N, neuronModel, integratore)
 *   - Crea N neuroni del tipo specificato (LIF o Exp) con parametri di default.
 *   - L'integratore può essere 'E' (Eulero in avanti) o 'R' (Runge-Kutta 4).
 *
 * ============================================================================
 * METODI PUBBLICI — NEURONI
 * ============================================================================
 * aggiungiNeurone(id, integratore, config)
 *   - Aggiunge un neurone alla rete con ID e configurazione specificati.
 *   - L'ID deve essere unico; se già esiste, stampa un errore.
 *
 * modificaIntegratoreNeurone(id, integratore)
 *   - Cambia il metodo di integrazione di un neurone esistente.
 *
 * modificaParametriNeurone(id, config)
 *   - Aggiorna i parametri di un neurone esistente.
 *   - La config deve essere dello stesso tipo del neurone (LIF o Exp).
 *
 * ============================================================================
 * METODI PUBBLICI — SINAPSI
 * ============================================================================
 * connettiNeuroni(IDpre, IDpost, configSyn) -> int
 *   - Crea una sinapsi tra due neuroni esistenti.
 *   - Restituisce un ID univoco che identifica la sinapsi.
 *   - Può essere chiamato più volte sulla stessa coppia (sinapsi multiple).
 *
 * modificaSinapsi(IDsin, configSyn)
 *   - Modifica i parametri di una sinapsi esistente (identificata dal suo ID).
 *   - La config deve essere dello stesso tipo della sinapsi (Current o
 * Conductance).
 *   - Deve essere chiamata PRIMA di prepare() / avviaSimulazione().
 *
 * getSinapsiIds(pre, post) -> vector<int>
 *   - Restituisce tutti gli ID delle sinapsi tra due neuroni.
 *   - Utile per recuperare ID persi o per iterare su sinapsi multiple.
 *
 * ============================================================================
 * METODI INTERNI (accessibili solo a Simulazione)
 * ============================================================================
 * step(dt)
 *   - Avanza la rete di un passo temporale dt.
 *   - Aggiorna neuroni e sinapsi in sequenza.
 *
 * prepare(dt)
 *   - Prepara la rete per la simulazione (es. costruisce i ring buffer per i
 * delay).
 *   - Chiamato automaticamente da Simulazione.
 *
 * aggiornaStatoRete()
 *   - Sincronizza i vettori di stato (potenziali, firing, correnti) dopo uno
 * step.
 *
 * ============================================================================
 * METODI GETTER (privati, esposti a Simulazione)
 * ============================================================================
 * getPointerStatoNeuroni() / getPointerStatoFiring() / getPointerStatoSinapsi()
 *   - Restituiscono puntatori ai vettori di stato per l'output su file.
 *
 * hasNeurone(id) / hasSinapsi(id)
 *   - Verificano l'esistenza di un neurone/sinapsi nella rete.
 */
class Rete {
  private:
    // attributi per la rappresentazione della rete
    std::vector<TypeNeuron> neuroni_;
    std::vector<TypeSyn> sinapsi_;
    std::unordered_map<int, size_t> idToIndex_;    // mappa ID neurone -> indice
    std::unordered_map<int, size_t> idToIndexSyn_; // mappa ID sinapsi -> indice
    int prossimoIdSyn_ = 0;                        // contatore ID sinapsi, analogo all'ID neurone

    // attributi per l'evoluzione della rete
    std::vector<double> stimoli_;
    std::vector<double> inputTotale_;

    // vettori di stato della rete
    std::vector<double> statoNeuroni_;
    std::vector<double> statoFiring_;
    std::vector<double> statoSinapsi_;

    // metodi setter stimoli

    void resetStimoli() { std::fill(stimoli_.begin(), stimoli_.end(), 0.0); }
    void addStimolo(size_t i, double value) { stimoli_[i] += value; }

    // metodi getter

    const std::vector<double>& getPointerStatoNeuroni() const { return statoNeuroni_; }
    const std::vector<double>& getPointerStatoFiring() const { return statoFiring_; }
    const std::vector<double>& getPointerStatoSinapsi() const { return statoSinapsi_; }

    size_t getNumNeuroni() const { return neuroni_.size(); }
    size_t getNumSinapsi() const { return sinapsi_.size(); }
    size_t getIndex(int id) const { return idToIndex_.at(id); }

    // metodi di controllo

    bool hasNeurone(int id) const { return idToIndex_.count(id) > 0; }
    bool hasSinapsi(int id) const { return idToIndexSyn_.count(id) > 0; }

    // metodi operativi privati

    void step(double dt);     // Avanza la rete di un passo temporale.
    void aggiornaStatoRete(); // Aggiorna i vettori di stato dopo lo step.
    void prepare(double dt);  // Prepara la rete (es. ring buffer per i delay).
    double getMinTau() const;

  public:
    // /**
    //  * @brief Costruisce una rete con N neuroni del tipo specificato.
    //  * @param N Numero di neuroni da creare (ID da 0 a N-1).
    //  * @param typeNeurone Modello di neurone (LIF o Exp).
    //  * @param typeintegratore Metodo di integrazione ('E' per Eulero, 'R' per
    //  * Runge-Kutta 4).
    //  */
    Rete(int N, NeuronModel typeNeurone, char typeintegratore = 'E');

    // --- Neuroni ---

    /**
     * @brief Costruisce una rete con N neuroni del tipo specificato.
     * @param N Numero di neuroni da creare (ID da 0 a N-1).
     * @param typeNeurone Modello di neurone (LIF o Exp).
     * @param typeintegratore Metodo di integrazione ('E' per Eulero, 'R' per
     * Runge-Kutta 4).
     */
    void aggiungiNeurone(int ID, char typeIntegratore, const TypeConfig& configurazione);

    /**
     * @brief Cambia il metodo di integrazione di un neurone esistente.
     * @param ID ID del neurone.
     * @param typeIntegratore Nuovo metodo ('E' o 'R').
     * @warning Se l'ID non esiste, stampa un errore.
     */
    void modificaIntegratoreNeurone(int ID, char typeIntegratore);

    /**
     * @brief Modifica i parametri di un neurone esistente.
     * @param ID ID del neurone.
     * @param configurazione Nuova configurazione (deve essere dello stesso tipo
     * del neurone).
     * @warning Se l'ID non esiste o la configurazione è incompatibile, stampa un
     * errore.
     */
    void modificaParametriNeurone(int ID, const TypeConfig& configurazione);

    // --- Sinapsi ---

    /**
     * @brief Crea una sinapsi tra due neuroni e restituisce il suo ID.
     * @param IDpre ID del neurone pre-sinaptico.
     * @param IDpost ID del neurone post-sinaptico.
     * @param configurazioneSinapsi Parametri della sinapsi (configCurrentSyn o
     * configConductanceSyn).
     * @return ID univoco della nuova sinapsi, o -1 se uno dei neuroni non esiste.
     * @note Può essere chiamato più volte sulla stessa coppia per creare sinapsi
     * multiple.
     */
    int connettiNeuroni(int IDpre, int IDpost, const TypeConfigSyn& configurazioneSinapsi);

    /**
     * @brief Modifica i parametri di una sinapsi esistente.
     * @param IDsin ID della sinapsi (restituito da connettiNeuroni).
     * @param configurazioneSinapsi Nuova configurazione (stesso tipo della
     * sinapsi originale).
     * @warning Se l'ID non esiste o la configurazione è incompatibile, stampa un
     * errore.
     * @warning Deve essere chiamata prima di prepare() / avviaSimulazione().
     */
    void modificaSinapsi(int IDsin, const TypeConfigSyn& configurazioneSinapsi);

    /**
     * @brief Restituisce tutti gli ID delle sinapsi tra due neuroni.
     * @param pre ID del neurone pre-sinaptico.
     * @param post ID del neurone post-sinaptico.
     * @return Vettore con gli ID delle sinapsi (può essere vuoto).
     */
    std::vector<int> getSinapsiIds(int pre, int post) const;

    ~Rete() = default;
    friend class Simulazione;
};

#endif // RETE_HPP
