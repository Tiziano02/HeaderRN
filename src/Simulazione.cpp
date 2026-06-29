#include "Simulazione.hpp"
#include "Input.hpp"
#include "Utility.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>

// ============================================================
// COSTRUTTORE
// ============================================================

Simulazione::Simulazione(const Rete& rete, double dt, double T)
    : rete_(rete), dt_(dt), stepCorrente_(0), stepTotali_(static_cast<int>(std::round(T / dt))) {

    // Warning per dt instabile
    double tauMin = rete_.getMinTau();
    if (dt > tauMin / 10.0) {
        std::cerr << "⚠️  [Simulazione] ATTENZIONE: dt = " << dt << "s è maggiore di tau_min/10 (" << tauMin / 10.0
                  << "s).\n"
                  << "   La simulazione potrebbe essere numericamente instabile.\n"
                  << "   Considera di ridurre dt o aumentare la risoluzione temporale.\n";
    }
}

// ============================================================
// STIMOLI ESTERNI
// ============================================================

void Simulazione::iniettaStimoli(const std::vector<stimolo>& stimoli) {
    // Pre-riserva spazio per evitare riallocazioni multiple
    stimoli_.reserve(stimoli_.size() + stimoli.size());

    for (const auto& [id, params] : stimoli) {
        if (!rete_.hasNeurone(id)) {
            std::cerr << "[Simulazione] errore: neurone ID " << id << " non trovato.\n";
            continue; // Salta questo stimolo ma continua con gli altri
        }

        // Converte ID in indice per accesso rapido nel loop caldo
        size_t indice = rete_.getIndex(id);
        stimoli_.push_back({indice, params});
    }
}

void Simulazione::valutaStimoli(double t) {
    rete_.resetStimoli();

    for (const auto& stimolo : stimoli_) {
        double valore = std::visit(
            [&](const auto& params) -> double {
                using T = std::decay_t<decltype(params)>;

                // Controllo se siamo nell'intervallo di tempo attivo
                if (t < params.timeStart || t > params.timeEnd) {
                    return 0.0;
                }

                if constexpr (std::is_same_v<T, configConstantStimulus>) {
                    return params.ampiezza;
                } else if constexpr (std::is_same_v<T, configSinStimulus>) {
                    return params.ampiezza * std::sin(params.frequenza * t + params.fase);
                }
                // Se si aggiungono nuovi tipi, aggiungere qui un nuovo constexpr branch
            },
            stimolo.parametri);

        rete_.addStimolo(stimolo.indiceNeurone, valore);
    }
}

// ============================================================
// OUTPUT
// ============================================================

void Simulazione::inizializzaOutput() {

    // 1. Apertura file e creazione dell'header

    // - apertura

    filePotenziali_.open("output/" + fileNameV_, std::ios::binary | std::ios::out);
    fileFiring_.open("output/" + fileNameF_, std::ios::binary | std::ios::out);
    fileSinapsi_.open("output/" + fileNameS_, std::ios::binary | std::ios::out);

    // - controllo

    if (!filePotenziali_.is_open() || !fileFiring_.is_open() || !fileSinapsi_.is_open()) {
        std::cerr << "[Simulazione] errore: impossibile aprire i file di output.\n";
        return;
    }

    // - calcolo numero di colonne

    int32_t colsV = static_cast<int32_t>(rete_.getNumNeuroni() + 1);
    int32_t colsF = static_cast<int32_t>(rete_.getNumNeuroni() + 1);
    int32_t colsS = static_cast<int32_t>(rete_.getNumSinapsi() + 1);

    // - scrittura dell'header

    filePotenziali_.write(reinterpret_cast<const char*>(&colsV), sizeof(int32_t));
    fileFiring_.write(reinterpret_cast<const char*>(&colsF), sizeof(int32_t));
    fileSinapsi_.write(reinterpret_cast<const char*>(&colsS), sizeof(int32_t));

    // 2. Creazione del buffer

    // - calcolo numero di byte da scrviere ad ogni step

    bytesPerStepV_ = static_cast<size_t>(colsV) * sizeof(double);
    bytesPerStepF_ = static_cast<size_t>(colsF) * sizeof(double);
    bytesPerStepS_ = static_cast<size_t>(colsS) * sizeof(double);

    // - calcolo la RAM disponibile sulla macchina e cosidero il 10% per i 3 buffer

    size_t ramDisponibile = getAvailableRAM(); // detection --> scritta interamente da gemini, da controllare

    size_t bufferTarget = ramDisponibile / 10 / 3;

    // - calcolo il numero di step di simulazione che entrano nel buffer dopo aver calcolato lo spazio disponible

    size_t stepsPerFlushV = bufferTarget / bytesPerStepV_;
    size_t stepsPerFlushF = bufferTarget / bytesPerStepF_;
    size_t stepsPerFlushS = bufferTarget / bytesPerStepS_;

    // - considero il numero di step minore per non allocare più memora del necessario

    size_t stepsPerFlush_ =
        std::min({stepsPerFlushV, stepsPerFlushF, stepsPerFlushS, static_cast<size_t>(stepTotali_)});

    // - controllo

    if (stepsPerFlush_ == 0) {
        std::cerr << "[Simulazione] Warning: RAM disponibile insufficiente o non rilevata. "
                  << "Forzo stepsPerFlush a 1 per evitare il crash.\n";
        stepsPerFlush_ = 1;
    }

    // - calcolo la dimensione buffer : i buffer hanno dimensioni diverse ma si riempiono tutti allo stesso step

    size_t bufferSizeV = stepsPerFlush_ * bytesPerStepV_;
    size_t bufferSizeF = stepsPerFlush_ * bytesPerStepF_;
    size_t bufferSizeS = stepsPerFlush_ * bytesPerStepS_;

    // - imposto la dimensione dei tre buffer

    bufferV_.resize(bufferSizeV);
    bufferF_.resize(bufferSizeF);
    bufferS_.resize(bufferSizeS);

    // Piccolo log a console per vedere quanta RAM sta usando effettivamente (utile per il profiling temporale)
    std::cout << "[Simulazione] RAM allocata per i 3 buffer: "
              << (bufferSizeV + bufferSizeF + bufferSizeS) / (1024 * 1024) << " MB (" << stepsPerFlush_
              << " step per flush)\n";
}

void Simulazione::writeFile() {

    // 1. Scrivo i buffer sui file
    filePotenziali_.write(bufferV_.data(), posizioneBuffer_ * bytesPerStepV_);
    fileFiring_.write(bufferF_.data(), posizioneBuffer_ * bytesPerStepF_);
    fileSinapsi_.write(bufferS_.data(), posizioneBuffer_ * bytesPerStepS_);

    // 2. Reset dell'indice
    // Sovrascrivo il buffer senza svuotarlo
    // non crea problemi perchè quando scrivo su disco utilizzo la posizioneBuffer_ e non scrivo tutto il buffer
    // in quel momento (quando la simualzione si interrope e il buffer enon è stato riempito tutto)
    posizioneBuffer_ = 0;
}

void Simulazione::loadStatoRete(double time) {

    // 1. Controllo se il buffer è pieno, nel caso scrivo su disco
    if (posizioneBuffer_ == stepsPerFlush_)
        writeFile();

    // 2. Calcolo in che poszione sono nel buffer
    size_t offsetV = posizioneBuffer_ * bytesPerStepV_;
    size_t offsetF = posizioneBuffer_ * bytesPerStepF_;
    size_t offsetS = posizioneBuffer_ * bytesPerStepS_;

    // 3. "Converto" il tempo attuale, inserisco il tempo nei tre buffer, vado avanti nella pozione del buffer attuale
    const char* src = reinterpret_cast<const char*>(&time);

    //  - inserisco il tempo nei tre buffer
    std::copy(src, src + sizeof(time), bufferV_.begin() + offsetV);
    std::copy(src, src + sizeof(time), bufferF_.begin() + offsetF);
    std::copy(src, src + sizeof(time), bufferS_.begin() + offsetS);

    // - sposto l'offset pari a nuemro di byte della "variabile" tempo
    offsetV += sizeof(time);
    offsetF += sizeof(time);
    offsetS += sizeof(time);

    // 4. Calcolo quanti BYTE totali occupano gli array
    size_t byteNeuroni = bytesPerStepV_ - sizeof(time);
    size_t byteFiring = bytesPerStepF_ - sizeof(time);
    size_t byteSinapsi = bytesPerStepS_ - sizeof(time);

    // 5. Copio, in blocco, lo stato della rete (neuroni, firing e sinapsi)

    // - prendo il puntatore all'inizio dell'array e lo converto in puntatore a char
    const char* srcV = reinterpret_cast<const char*>(rete_.getPointerStatoNeuroni().data());
    std::copy(srcV, srcV + byteNeuroni, bufferV_.begin() + offsetV);

    const char* srcF = reinterpret_cast<const char*>(rete_.getPointerStatoFiring().data());
    std::copy(srcF, srcF + byteFiring, bufferF_.begin() + offsetF);

    const char* srcS = reinterpret_cast<const char*>(rete_.getPointerStatoSinapsi().data());
    std::copy(srcS, srcS + byteSinapsi, bufferS_.begin() + offsetS);

    // 6. Avanzo la pozione nel buffer dopo aver salvato uno step di simulazione
    posizioneBuffer_++;
}

void Simulazione::avviaSimulazione(const std::string& filenameV, const std::string& filenameF,
                                   const std::string& filenameS) {

    // 1. inizializza il tempo della simulazione e il passo attuale a zero
    double time = 0.0;
    stepCorrente_ = 0;

    // 2. inializzo il nome del file dove salvare l'output della simulazione
    fileNameV_ = filenameV;
    fileNameF_ = filenameF;
    fileNameS_ = filenameS;

    // 3. Creazione dell'header nei tre file di output e creazione del buffer per la scrittura su disco
    inizializzaOutput();

    // 4. Per adesso : creazione della dimensione dei ring per il ritardo sinaptico
    rete_.prepare(dt_);

    for (; stepCorrente_ < stepTotali_; ++stepCorrente_) {

        // 1. Applica gli stimoli al tempo corrente
        valutaStimoli(time);

        // 2. Avanza la rete di un passo
        rete_.step(dt_);
        time += dt_;

        // 3. Aggiorna lo stato e salva
        rete_.aggiornaStatoRete();
        loadStatoRete(time);
    }

    if (posizioneBuffer_ > 0) {
        writeFile();
    }
}
