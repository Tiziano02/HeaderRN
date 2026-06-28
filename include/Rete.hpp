#ifndef RETE_HPP
#define RETE_HPP

#include "Neurone.hpp"
#include "Sinapsi.hpp"

#include <algorithm>
#include <cstddef>
#include <unordered_map>
#include <vector>

/*
 * Rete — contenitore della topologia e motore di evoluzione della rete neurale.
 *
 * La rete è rappresentata come:
 *   - un vettore di neuroni  (TypeNeuron  = variant<LIF, Exp>)
 *   - un vettore di sinapsi  (TypeSyn     = variant<CurrentSyn, ConductanceSyn>)
 *   - una mappa ID -> indice per accesso rapido ai neuroni
 *   - un buffer di correnti afferenti pre-allocato, riusato ad ogni step
 *   - vettori di stato della rete: statoNeuroni_, statoFiring_, statoSinapsi_
 *
 * La scelta di una lista di sinapsi invece di una matrice di adiacenza
 * semplifica l'aggiunta futura di delay e plasticità sinaptica.
 *
 * Costruttori:
 *   Rete(N, typeNeurone, typeIntegratore)
 *         — N neuroni con parametri biologici di default, ID da 0 a N-1
 *
 * Metodi operativi:
 *   aggiungiNeurone(id, typeIntegratore, config)
 *   modificaIntegratoreNeurone(id, typeIntegratore)
 *   modificaParametriNeurone(id, config)
 *   connettiNeuroni(IDpre, IDpost, configSyn)   — aggiunge una sinapsi
 *   modificaSinapsi(IDsin, configSyn)           — modifica una sinapsi per ID
 *   step(dt)
 *   aggiornaStatoRete()
 *
 * Metodi getter (privati, esposti a Simulazione via friend):
 *   getPointerStatoNeuroni()
 *   getPointerStatoFiring()
 *   getPointerStatoSinapsi()
 *
 * Metodi di controllo:
 *   hasNeurone(id)  — true se un neurone con quell'ID esiste nella rete
 */
class Rete {

  private:
    std::vector<TypeNeuron> neuroni_;
    std::vector<TypeSyn> sinapsi_;
    std::vector<double> stimoli_;
    std::vector<double> inputTotale_;
    std::unordered_map<int, size_t> idToIndex_;    // mappa ID neurone -> indice
    std::unordered_map<int, size_t> idToIndexSyn_; // mappa ID sinapsi -> indice

    int prossimoIdSyn_ = 0; // contatore ID sinapsi, analogo all'ID neurone

    std::vector<double> statoNeuroni_;
    std::vector<double> statoFiring_;
    std::vector<double> statoSinapsi_;

    // metodi setter stimoli
    void resetStimoli() { std::fill(stimoli_.begin(), stimoli_.end(), 0.0); }
    void addStimolo(size_t i, double value) { stimoli_[i] += value; }

    // metodi getter
    const std::vector<double>& getPointerStatoNeuroni() const;
    const std::vector<double>& getPointerStatoFiring() const;
    const std::vector<double>& getPointerStatoSinapsi() const;

    size_t getNumNeuroni() const { return neuroni_.size(); }
    size_t getNumSinapsi() const { return sinapsi_.size(); }
    size_t getIndex(int id) const { return idToIndex_.at(id); }

    // metodi di controllo
    bool hasNeurone(int id) const { return idToIndex_.count(id) > 0; }
    bool hasSinapsi(int id) const { return idToIndexSyn_.count(id) > 0; }

    // metodi operativi privati
    void step(double dt);
    void aggiornaStatoRete();
    void prepare(double dt);

  public:
    Rete(int N, Label_Type_Neuron typeNeurone, char typeintegratore = 'E');

    // --- Neuroni ---

    void aggiungiNeurone(int ID, char typeIntegratore, const TypeConfig& configurazione);
    void modificaIntegratoreNeurone(int ID, char typeIntegratore);
    void modificaParametriNeurone(int ID, const TypeConfig& configurazione);

    // --- Sinapsi ---

    /*
     * connettiNeuroni — aggiunge una sinapsi tra IDpre e IDpost.
     *
     * Restituisce l'ID univoco assegnato alla sinapsi (analogo all'ID neurone).
     * Usare quell'ID per modificarla in seguito con modificaSinapsi().
     *
     * Esempio:
     *   int idSyn = rete.connettiNeuroni(0, 1, configCurrentSyn{...});
     *   rete.modificaSinapsi(idSyn, configCurrentSyn{nuovo_peso, ...});
     *
     * TypeConfigSyn = std::variant<configCurrentSyn, configConductanceSyn>
     */
    int connettiNeuroni(int IDpre, int IDpost, const TypeConfigSyn& configurazioneSinapsi);

    /*
     * modificaSinapsi — aggiorna i parametri di una sinapsi esistente.
     *
     * IDsin: ID restituito da connettiNeuroni().
     * La configurazione deve essere dello stesso tipo della sinapsi originale;
     * se non lo è viene stampato un errore e non viene modificato nulla.
     */
    void modificaSinapsi(int IDsin, const TypeConfigSyn& configurazioneSinapsi);

    ~Rete() = default;

    friend class Simulazione;
};

#endif // RETE_HPP
