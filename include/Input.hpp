#ifndef INPUT_HPP
#define INPUT_HPP

#include <iostream>

// struct per chi utilizza il framework
struct parametriStimoloCostante {
    double timeStart;
    double timeEnd;
    double ampiezza;
};

struct parametriStimoloSeno {
    double timeStart;
    double timeEnd;
    double ampiezza;
    double frequenza;
    double fase;
};

// struct logica interna

enum class Tipo_stimolo { Costante, Sinusoidale /* aggiungere qui */ };

struct rigaRegistroStimolo {
    int id;              // debug/query
    size_t indexNeurone; // usato nel loop caldo
    Tipo_stimolo tipo;   // dice QUALE database guardare
    size_t rigaDB;       // dice QUALE riga di quel database guardare
    int stepStart;
    int stepEnd;
};

#endif