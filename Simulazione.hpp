#ifndef SINAPSI_HPP
#define SINAPSI_HPP

#include <vector>
#include <fstream>
#include <iostream>
#include "UnitaSI.hpp"


struct Input {
    int id;
    std::vector<double> valori;
};

class Simualazione
{
private:
    // parametri simulazione 
    double dt_;
    double T_;
    int stepTotali_ = static_cast<int>(T_ / dt_);
    int stepCorrente_ = 0;

    // input esterno 
    std::vector<std::vector<double>> inputEsternoTotale_;   // input associati a ciascun neurone
    void setInput(Input& input);

public:
    Simualazione(double dt, double T) : dt_(dt), T_(T) {}; 

    void inizializzaSimulazione(double dt, double T, std::vector<Input> inputEsterno){
        stepTotali_ = static_cast<int>(T / dt);
        
        //controllo che l'input ha la stessa dimensione della simulazione
        for(size_t i=0; i<inputEsterno.size();i++)
            if( stepTotali_ != inputEsterno[i].valori.size())
                return;
        
        // imposto l'input esterno 
        for(size_t i=0; i<inputEsterno.size();i++)
            setInput(inputEsterno[i]);
        
    }
    
        void simulazione(double dt,const std::string &filenameV, const std::string &filenameF, const std::string &filenameS) {

        stepCorrente_ = 0;
        double time = 0.0 * s;

        std::ofstream filePotenziali;
        std::ofstream fileFiring;
        std::ofstream fileSinapsi;
        filePotenziali.open(filenameV);
        fileFiring.open(filenameF);
        fileSinapsi.open(filenameS);

        if (!filePotenziali.is_open() || !fileFiring.is_open() || !fileSinapsi.is_open()) {
            std::cerr << "Errore nell'apertura dei file di output!" << std::endl;
            return;
        }

        for (int n = 0; n < stepTotali_; ++n) {
            Rete::step(dt);    // esegue un passo di simulazione
            time += dt; // aggiorna il tempo

            salvaStatoRete(filePotenziali, fileFiring, fileSinapsi, time); // salva lo stato della rete nei file
        }
    }

};

#endif // SINAPSI_HPP
