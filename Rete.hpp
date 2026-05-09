#ifndef RETE_HPP
#define RETE_HPP

#include "Neurone.hpp"
#include "UnitaSI.hpp"
#include "Sinapsi.hpp"
#include <fstream>
#include <iostream> // --> eliminabile se si toglie la gestione degli errori con i file
#include <map>
#include <string>
#include <vector>

class Rete {

private:
    std::vector<Neurone> neuroni_;
    std::vector<Sinapsi> sinapsi_; // vettore di sinapsi --> nell'oggetto Sinapsi ci sono già gli ID dei neuroni pre e post sinaptici, quindi non serve una matrice di adiacenza, basta un vettore di sinapsi
    std::vector<double> inputTotale_;
    std::map<int, size_t> idToIndex_; // mappa dagli ID agli indici
    
public:
    // costruttore di default
    Rete() = default; // --> cosa dovrebbe fare ? 0 neuroni, matrice vuota, input vuoto, mappa vuota

    // costruttore utility --> utilizzabile se si vuole N neuroni standard tutti uguali biologicamente
    Rete(int N) {
        for (int i = 0; i < N; ++i) {
            Neurone n(i);
            aggiungiNeurone(n); // aggiunge il neurone alla rete
        }
    }

    // metodi per gestire la rete neurale
    void aggiungiNeurone(const Neurone &neurone);
    void connettiNeuroni(const Sinapsi &s);
    
    
    void step(double dt); // --> pure questo fa parte della simulazione --> serve oggetto simulazione ? 

    // metodi getter
    std::vector<double> getPotenziali() const;
    std::vector<int> getFiringStates() const;
    std::vector<double> getSinapsi() const; // getter per le sinapsi, utile per debug o analisi della rete

    // metodo log
    void salvaStatoRete(std::ofstream &filePotenziali, std::ofstream &fileFiring, std::ofstream &fileSinapsi, double time);

    ~Rete() = default;
};

#include "reteImp.hpp"

#endif // RETE_HPP
