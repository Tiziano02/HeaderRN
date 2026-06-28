#include "Rete.hpp"
#include <cstddef>
#include <iostream>
#include <type_traits>

// ─────────────────────────────────────────────────────────────────────────────
// Costruttore
// ─────────────────────────────────────────────────────────────────────────────

Rete::Rete(int N, Label_Type_Neuron typeNeurone, char typeintegratore) {

    neuroni_.reserve(N);

    switch (typeNeurone) {

    case Label_Type_Neuron::LIF: {
        configLIF configBase;
        for (int i = 0; i < N; ++i)
            aggiungiNeurone(i, typeintegratore, configBase);
        break;
    }

    case Label_Type_Neuron::Exp: {
        configExp configBase;
        for (int i = 0; i < N; ++i)
            aggiungiNeurone(i, typeintegratore, configBase);
        break;
    }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Neuroni
// ─────────────────────────────────────────────────────────────────────────────

void Rete::aggiungiNeurone(int id, char typeIntegratore, const TypeConfig& configurazione) {

    std::visit(
        [&](const auto& config) {
            if (hasNeurone(id)) {
                std::cerr << "[Rete] errore: neurone con ID " << id << " già presente.\n";
                return;
            }

            using T = std::decay_t<decltype(config)>;

            if constexpr (std::is_same_v<T, configLIF>) {
                neuroni_.emplace_back(std::in_place_type<LIF>, id, typeIntegratore, config);
            } else if constexpr (std::is_same_v<T, configExp>) {
                neuroni_.emplace_back(std::in_place_type<Exp>, id, typeIntegratore, config);
            }

            statoNeuroni_.push_back(config.V_);
            idToIndex_[id] = neuroni_.size() - 1;
            inputTotale_.push_back(0.0);
            stimoli_.push_back(0.0);
            statoFiring_.push_back(0.0);
        },
        configurazione);
}

void Rete::modificaIntegratoreNeurone(int id, char typeIntegratore) {
    if (!hasNeurone(id)) {
        std::cerr << "[Rete] errore: neurone con ID " << id << " non esiste.\n";
        return;
    }
    size_t index = getIndex(id);
    std::visit([&](auto& n) { n.tipoIntegratore_ = typeIntegratore; }, neuroni_[index]);
}

void Rete::modificaParametriNeurone(int id, const TypeConfig& configurazione) {
    if (!hasNeurone(id)) {
        std::cerr << "[Rete] errore: neurone con ID " << id << " non esiste.\n";
        return;
    }

    size_t index = getIndex(id);

    std::visit(
        [&](auto& n) {
            using T = std::decay_t<decltype(n)>;
            if constexpr (std::is_same_v<T, LIF>) {
                if (auto cfg = std::get_if<configLIF>(&configurazione)) {
                    n.V_ = cfg->V_;
                    n.Vth_ = cfg->V_th;
                    n.Vth0_ = cfg->V_th;
                    n.VthSpikeMax_ = cfg->V_ThresholdSpikeMax;
                    n.Vrest_ = cfg->V_rest;
                    n.Vreset_ = cfg->V_reset;
                    n.R_ = cfg->R;
                    n.C_ = cfg->C;
                    n.tau_ = cfg->R * cfg->C;
                    n.timeAbsolute_ = cfg->timeAbsolute;
                    n.timeRelative_ = cfg->timeRelative;
                    n.tauRelative_ = cfg->timeRelative / 3;
                } else {
                    std::cerr << "[Rete] errore: configurazione incompatibile per neurone LIF.\n";
                }
            } else if constexpr (std::is_same_v<T, Exp>) {
                if (auto cfg = std::get_if<configExp>(&configurazione)) {
                    n.V_ = cfg->V_;
                    n.Vth_ = cfg->V_th;
                    n.Vth0_ = cfg->V_th;
                    n.VthSpikeMax_ = cfg->V_ThresholdSpikeMax;
                    n.Vrest_ = cfg->V_rest;
                    n.Vreset_ = cfg->V_reset;
                    n.DeltaT_ = cfg->deltaT;
                    n.R_ = cfg->R;
                    n.C_ = cfg->C;
                    n.tau_ = cfg->R * cfg->C;
                    n.timeAbsolute_ = cfg->timeAbsolute;
                    n.timeRelative_ = cfg->timeRelative;
                    n.tauRelative_ = cfg->timeRelative / 3;
                } else {
                    std::cerr << "[Rete] errore: configurazione incompatibile per neurone Exp.\n";
                }
            }
        },
        neuroni_[index]);

    statoNeuroni_[index] = std::visit([](const auto& n) { return n.getPotential(); }, neuroni_[index]);
    bool fire = std::visit([](const auto& n) { return n.hasFired(); }, neuroni_[index]);
    statoFiring_[index] = fire ? 1.0 : 0.0;
}

// ─────────────────────────────────────────────────────────────────────────────
// Sinapsi
// ─────────────────────────────────────────────────────────────────────────────

/*
 * connettiNeuroni — aggiunge una sinapsi e restituisce il suo ID univoco.
 *
 * L'ID è generato internamente (prossimoIdSyn_) e registrato in idToIndexSyn_.
 * Questo risolve il problema di identificazione quando esistono più sinapsi
 * tra la stessa coppia (IDpre, IDpost): ogni sinapsi ha un ID indipendente.
 */
int Rete::connettiNeuroni(int IDpre, int IDpost, const TypeConfigSyn& configurazioneSinapsi) {

    if (!hasNeurone(IDpre) || !hasNeurone(IDpost)) {
        std::cerr << "[Rete] errore: uno o entrambi i neuroni (pre=" << IDpre << ", post=" << IDpost
                  << ") non esistono nella rete.\n";
        return -1; // ID invalido come segnale di errore
    }

    size_t indexPre = idToIndex_[IDpre];
    size_t indexPost = idToIndex_[IDpost];

    std::visit(
        [&](const auto& config) {
            using T = std::decay_t<decltype(config)>;

            if constexpr (std::is_same_v<T, configCurrentSyn>) {
                sinapsi_.emplace_back(std::in_place_type<CurrentSyn>, indexPre, indexPost, IDpre, IDpost, config);
            } else if constexpr (std::is_same_v<T, configConductanceSyn>) {
                sinapsi_.emplace_back(std::in_place_type<ConductanceSyn>, indexPre, indexPost, IDpre, IDpost, config);
            }
        },
        configurazioneSinapsi);

    // Corrente iniziale = 0 per entrambi i tipi
    statoSinapsi_.push_back(0.0);

    // Assegna e registra l'ID
    int idAssegnato = prossimoIdSyn_++;
    idToIndexSyn_[idAssegnato] = sinapsi_.size() - 1;

    return idAssegnato;
}

/*
 * modificaSinapsi — aggiorna i parametri di una sinapsi identificata dal suo ID.
 *
 * Precondizione: va chiamata PRIMA di avviaSimulazione() (e quindi prima di prepare()).
 * Questa è una regola fondamentale del framework: la rete non può essere modificata
 * a simulazione avviata. Se si modifica il delay_, il ring viene ricostruito
 * correttamente solo se prepare() non è ancora stato chiamato (o non lo sarà mai
 * con un dt diverso da quello della simulazione finale).
 *
 * Se il tipo di configurazione non corrisponde al tipo della sinapsi esistente
 * viene stampato un errore e non viene modificato nulla.
 */
void Rete::modificaSinapsi(int IDsin, const TypeConfigSyn& configurazioneSinapsi) {

    if (!hasSinapsi(IDsin)) {
        std::cerr << "[Rete] errore: sinapsi con ID " << IDsin << " non esiste.\n";
        return;
    }

    size_t index = idToIndexSyn_[IDsin];

    std::visit(
        [&](auto& syn) {
            using TSyn = std::decay_t<decltype(syn)>;

            if constexpr (std::is_same_v<TSyn, CurrentSyn>) {
                if (auto cfg = std::get_if<configCurrentSyn>(&configurazioneSinapsi)) {
                    syn.peso_ = cfg->peso;
                    syn.Ipeak_ = cfg->Ipeak;
                    syn.tau_ = cfg->tau;
                    syn.delay_ = cfg->delay;
                    // prepare() non è ancora stato chiamato, quindi il ring non esiste ancora:
                    // basta aggiornare delay_ e sarà prepare() a costruire il ring con il dt corretto.
                } else {
                    std::cerr << "[Rete] errore: configurazione incompatibile per CurrentSyn "
                              << "(hai passato configConductanceSyn?).\n";
                }
            } else if constexpr (std::is_same_v<TSyn, ConductanceSyn>) {
                if (auto cfg = std::get_if<configConductanceSyn>(&configurazioneSinapsi)) {
                    syn.peso_ = cfg->peso;
                    syn.gpeak_ = cfg->gpeak;
                    syn.tau_ = cfg->tau;
                    syn.E_rev_ = cfg->E_rev;
                    syn.delay_ = cfg->delay;
                } else {
                    std::cerr << "[Rete] errore: configurazione incompatibile per ConductanceSyn "
                              << "(hai passato configCurrentSyn?).\n";
                }
            }
        },
        sinapsi_[index]);
}

// ─────────────────────────────────────────────────────────────────────────────
// Metodi operativi interni
// ─────────────────────────────────────────────────────────────────────────────

void Rete::prepare(double dt) {
    for (auto& s : sinapsi_) {
        std::visit([&](auto& syn) { syn.setDelayRing(dt); }, s);
    }
}

/*
 * step — avanza lo stato dell'intera rete di un passo dt.
 *
 * Le due tipologie di sinapsi hanno firme di update() diverse:
 *   CurrentSyn::update(dt, preFired)
 *   ConductanceSyn::update(dt, preFired, V_post)
 *
 * Rete usa std::visit con if constexpr per chiamare la firma corretta,
 * recuperando V_post dal neurone post-sinaptico solo quando serve.
 */
void Rete::step(double dt) {

    std::fill(inputTotale_.begin(), inputTotale_.end(), 0.0);

    for (auto& synVar : sinapsi_) {

        std::visit(
            [&](auto& syn) {
                using TSyn = std::decay_t<decltype(syn)>;

                bool preFired = std::visit([](const auto& n) { return n.hasFired(); }, neuroni_[syn.getIndexPre()]);

                if constexpr (std::is_same_v<TSyn, CurrentSyn>) {
                    // Current-based: update autonomo, non serve V_post
                    syn.update(dt, preFired);

                } else if constexpr (std::is_same_v<TSyn, ConductanceSyn>) {
                    // Conductance-based: serve il potenziale del neurone post-sinaptico
                    double V_post =
                        std::visit([](const auto& n) { return n.getPotential(); }, neuroni_[syn.getIndexPost()]);
                    syn.update(dt, preFired, V_post);
                }

                inputTotale_[syn.getIndexPost()] += syn.getCurrent();
            },
            synVar);
    }

    for (size_t i = 0; i < neuroni_.size(); ++i) {
        inputTotale_[i] += stimoli_[i];
        std::visit([&](auto& n) { n.update(inputTotale_[i], dt); }, neuroni_[i]);
    }
}

void Rete::aggiornaStatoRete() {

    for (size_t i = 0; i < neuroni_.size(); i++) {
        statoNeuroni_[i] = std::visit([](const auto& n) { return n.getPotential(); }, neuroni_[i]);
        bool fire = std::visit([](const auto& n) { return n.hasFired(); }, neuroni_[i]);
        statoFiring_[i] = fire ? 1.0 : 0.0;
    }

    for (size_t i = 0; i < sinapsi_.size(); i++) {
        statoSinapsi_[i] = std::visit([](const auto& s) { return s.getCurrent(); }, sinapsi_[i]);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Getter
// ─────────────────────────────────────────────────────────────────────────────

const std::vector<double>& Rete::getPointerStatoNeuroni() const { return statoNeuroni_; }
const std::vector<double>& Rete::getPointerStatoFiring() const { return statoFiring_; }
const std::vector<double>& Rete::getPointerStatoSinapsi() const { return statoSinapsi_; }
