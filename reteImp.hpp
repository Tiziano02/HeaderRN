#ifndef RETEIMP_HPP
#define RETEIMP_HPP

#include "Rete.hpp"
#include "Sinapsi.hpp"

// Implementazione dei metodi della classe ReteNeurale

// metodi per gestire la rete neurale
void Rete::aggiungiNeurone(const Neurone &neurone) {

    neuroni_.push_back(neurone);                       // aggiunge il neurone alla lista
    idToIndex_[neurone.getId()] = neuroni_.size() - 1; // aggiunge ID alla mappa

    // osservazione: quando aggiungo un neurone non aggiungo una sinapsi perché non so ancora a quali neuroni sarà connesso, le sinapsi vengono aggiunte successivamente con il metodo connettiNeuroni

    // osservazione: quando aggiungo un neurone devo aumentare di una riga la matrice degli input
    inputEsterno_.push_back({});

    inputTotale_.push_back(0.0); // aggiunge un nuovo input totale associato al nuovo neurone
}
void Rete::connettiNeuroni(const Sinapsi &s) {
    if (idToIndex_.count(s.getIdPre()) && idToIndex_.count(s.getIdPost())) // verifica che entrambi i neuroni esistano (perche mappa ha ID unici)
        sinapsi_.push_back(s);                                             // inserisco la connessione da id1 a id2 con il peso specificato
}

void Rete::step(double dt) {

    // reset input totale prima di calcolare il nuovo stato della rete
    std::fill(inputTotale_.begin(), inputTotale_.end(), 0.0);

    // update sinapsi usando la classe
    for (size_t i = 0; i < sinapsi_.size(); ++i)
        sinapsi_[i].update(dt, neuroni_[idToIndex_[sinapsi_[i].getIdPre()]].hasFired()); // aggiorna la sinapsi in base al firing del neurone pre-sinaptico

    // aggiungi il contributo di tutte le sinapsi al potenziale del neurone post-sinaptico
    for (size_t cor = 0; cor < sinapsi_.size(); ++cor) {
        size_t post = idToIndex_[sinapsi_[cor].getIdPost()];
        inputTotale_[post] += sinapsi_[cor].getCurrent();
    }

    // aggiungo l'input esterno e aggiorno lo stato di ogni neurone
    for (size_t i = 0; i < neuroni_.size(); ++i) {
        if (stepCorrente_ < inputEsterno_[i].size())
            inputTotale_[i] += inputEsterno_[i][stepCorrente_];

        neuroni_[i].update(inputTotale_[i], dt);
    }

    stepCorrente_++;
}

// metodi getter
std::vector<double> Rete::getPotenziali() const {

    std::vector<double> potenziali;
    potenziali.reserve(neuroni_.size());

    for (const auto &neurone : neuroni_)
        potenziali.push_back(neurone.getPotential());

    return potenziali;
}
std::vector<double> Rete::getSinapsi() const {

    std::vector<double> sinapsi;
    sinapsi.reserve(sinapsi_.size());

    for (const auto &s : sinapsi_)
        sinapsi.push_back(s.getCurrent());

    return sinapsi;
}
std::vector<int> Rete::getFiringStates() const {

    std::vector<int> firingStates;
    firingStates.reserve(neuroni_.size());

    for (const auto &neurone : neuroni_)
        firingStates.push_back(neurone.hasFired() ? 1 : 0); // 1 se ha sparato, 0 altrimenti

    return firingStates;
}

// metodi log
void Rete::salvaStatoRete(std::ofstream &filePotenziali, std::ofstream &fileFiring, std::ofstream &fileSinapsi, double time) {

    std::vector<double> potenziali = getPotenziali();
    std::vector<int> spikes = getFiringStates();
    std::vector<double> sinapsi = getSinapsi();

    filePotenziali << time << " ";
    for (double v : potenziali)
        filePotenziali << v << " ";
    filePotenziali << "\n";

    fileFiring << time << " ";
    for (int f : spikes)
        fileFiring << f << " ";
    fileFiring << "\n";

    fileSinapsi << time << " ";
    for (double s : sinapsi)
        fileSinapsi << s << " ";
    fileSinapsi << "\n";
}

#endif // RETEIMP_HPP