#include "Rete.hpp"
#include "Simulazione.hpp"
#include "UnitaSI.hpp"

#include <iostream>
#include <string>

// ─────────────────────────────────────────────────────────────────────────────
// Utility di stampa
// ─────────────────────────────────────────────────────────────────────────────

static void intestazione(const std::string& titolo) {
    std::cout << "\n══════════════════════════════════════════\n";
    std::cout << "  " << titolo << "\n";
    std::cout << "══════════════════════════════════════════\n";
}

static void ok(const std::string& msg) { std::cout << "  [OK]     " << msg << "\n"; }
static void info(const std::string& msg) { std::cout << "  [INFO]   " << msg << "\n"; }

// ─────────────────────────────────────────────────────────────────────────────
// TEST 1 — Nuova modalità di creazione rete
//
// Verifica il flusso:
//   1. Rete(N) con neuroni di default
//   2. modificaParametriNeurone su un sottoinsieme
//   3. modificaIntegratoreNeurone su un neurone
//   4. connettiNeuroni (ritorna ID sinapsi)
//   5. modificaSinapsi tramite l'ID restituito
// ─────────────────────────────────────────────────────────────────────────────
static void test_nuova_creazione_rete() {
    intestazione("TEST 1 — Nuova modalità di creazione rete (LIF + CurrentSyn)");

    // 1. Rete con 4 neuroni LIF di default, integratore Eulero
    Rete rete(4, Label_Type_Neuron::LIF, 'E');
    ok("Rete(4, LIF, 'E') creata");

    // 2. Modifica parametri del neurone 0 (soglia più alta)
    configLIF cfgN0;
    cfgN0.V_th = -45.0 * mV;
    cfgN0.R = 2.0 * Mohm;
    rete.modificaParametriNeurone(0, cfgN0);
    ok("modificaParametriNeurone(0) — Vth=-45mV, R=2MOhm");

    // 3. Cambia integratore del neurone 2 a Runge-Kutta
    // rete.modificaIntegratoreNeurone(2, 'R');
    // ok("modificaIntegratoreNeurone(2, 'R') — Runge-Kutta");

    // 4a. Connessione 0→1 (eccitatoria)
    configCurrentSyn cfgSyn01;
    cfgSyn01.peso = 0.8;
    cfgSyn01.Ipeak = 10e-9; // [A]
    cfgSyn01.tau = 5.0 * ms;
    cfgSyn01.delay = 1.0 * ms;
    int idSyn01 = rete.connettiNeuroni(0, 1, cfgSyn01);
    ok("connettiNeuroni(0→1) — ID sinapsi restituito: " + std::to_string(idSyn01));

    // 4b. Seconda sinapsi 0→1 (stessa coppia, ID diverso — test disambiguazione)
    configCurrentSyn cfgSyn01b;
    cfgSyn01b.peso = 0.3;
    cfgSyn01b.Ipeak = 10e-9;
    cfgSyn01b.tau = 3.0 * ms;
    cfgSyn01b.delay = 2.0 * ms;
    int idSyn01b = rete.connettiNeuroni(0, 1, cfgSyn01b);
    ok("connettiNeuroni(0→1) seconda sinapsi — ID: " + std::to_string(idSyn01b) +
       "  (diverso dal precedente: " + (idSyn01b != idSyn01 ? "SI" : "NO") + ")");

    // 4c. Connessione 1→2 (inibitoria, peso negativo)
    configCurrentSyn cfgSyn12;
    cfgSyn12.peso = -0.5;
    cfgSyn12.Ipeak = 10e-9;
    cfgSyn12.tau = 8.0 * ms;
    cfgSyn12.delay = 1.5 * ms;
    int idSyn12 = rete.connettiNeuroni(1, 2, cfgSyn12);
    ok("connettiNeuroni(1→2) inibitoria — ID: " + std::to_string(idSyn12));

    // 5. Modifica della prima sinapsi 0→1 tramite ID
    configCurrentSyn cfgSyn01_mod = cfgSyn01;
    cfgSyn01_mod.peso = 1.0; // aumento il peso
    cfgSyn01_mod.delay = 0.5 * ms;
    rete.modificaSinapsi(idSyn01, cfgSyn01_mod);
    ok("modificaSinapsi(idSyn01) — peso aggiornato a 1.0, delay a 0.5ms");

    // 6. Errore atteso: ID sinapsi inesistente
    info("Tentativo modificaSinapsi con ID inesistente (atteso errore su stderr):");
    rete.modificaSinapsi(999, cfgSyn01_mod);

    // 7. Errore atteso: neurone inesistente
    info("Tentativo modificaParametriNeurone con ID inesistente (atteso errore su stderr):");
    rete.modificaParametriNeurone(99, cfgN0);

    // 8. Simulazione breve per verificare che la rete giri senza crash
    Simulazione sim(rete, 0.1 * ms, 50.0 * ms);

    // I_min per sparare = (Vth - Vrest) / R = 15mV / 1MOhm = 15 nA
    // Usiamo 20 nA per stare comodamente sopra soglia
    std::vector<int> ids = {0};
    std::vector<parametriStimoloCostante> stim = {{0.0, 50.0 * ms, 20e-9}};
    sim.iniettaStimoloCostante(ids, stim);

    sim.avviaSimulazione("test1_V.bin", "test1_F.bin", "test1_S.bin");
    ok("avviaSimulazione completata senza crash");
}

// ─────────────────────────────────────────────────────────────────────────────
// TEST 2 — Neuroni Exp
//
// Verifica che si possano creare neuroni Exp, modificarne i parametri
// e farli girare in simulazione.
// ─────────────────────────────────────────────────────────────────────────────
static void test_neuroni_exp() {
    intestazione("TEST 2 — Neuroni Exp");

    // Rete di 3 neuroni Exp con Runge-Kutta
    Rete rete(3, Label_Type_Neuron::Exp, 'R');
    ok("Rete(3, Exp, 'E') creata");

    // Modifica parametri del neurone 1
    configExp cfgE1;
    cfgE1.V_th = -48.0 * mV;
    cfgE1.V_rest = -65.0 * mV;
    rete.modificaParametriNeurone(1, cfgE1);
    ok("modificaParametriNeurone(1) su Exp — Vth=-48mV");

    // Errore atteso: passare configLIF a un neurone Exp
    info("Tentativo modificaParametriNeurone con config sbagliata (atteso errore su stderr):");
    rete.modificaParametriNeurone(1, configLIF{});

    // Connessione con CurrentSyn — Ipeak scalato coerentemente con lo stimolo
    configCurrentSyn cfgSyn;
    cfgSyn.peso = 0.6;
    cfgSyn.Ipeak = 10e-9; // [A] — contributo sinaptico significativo
    cfgSyn.tau = 5.0 * ms;
    cfgSyn.delay = 1.0 * ms;
    int idSyn = rete.connettiNeuroni(0, 1, cfgSyn);
    ok("connettiNeuroni(0→1) su rete Exp — ID: " + std::to_string(idSyn));

    // Simulazione breve
    Simulazione sim(rete, 0.1 * ms, 50.0 * ms);

    std::vector<int> ids = {0};
    std::vector<parametriStimoloCostante> stim = {{0.0, 50.0 * ms, 20e-9}};
    sim.iniettaStimoloCostante(ids, stim);

    sim.avviaSimulazione("test2_V.bin", "test2_F.bin", "test2_S.bin");
    ok("avviaSimulazione completata senza crash");
}

// ─────────────────────────────────────────────────────────────────────────────
// TEST 3 — ConductanceSyn
//
// Verifica la sinapsi conductance-based:
//   - creazione con configConductanceSyn
//   - modifica tramite ID
//   - errore atteso se si passa config sbagliata
//   - simulazione con sinapsi miste (Current + Conductance sulla stessa rete)
// ─────────────────────────────────────────────────────────────────────────────
static void test_conductance_syn() {
    intestazione("TEST 3 — ConductanceSyn (sinapsi miste)");

    // Rete di 4 neuroni LIF
    Rete rete(4, Label_Type_Neuron::LIF, 'E');
    ok("Rete(4, LIF, 'E') creata");

    // Sinapsi eccitatoria conductance-based 0→1  (E_rev ~ 0 mV, AMPA-like)
    // I = g * (V_post - E_rev) ~ 1e-8 S * 0.065 V = 0.65 nA per spike — significativo
    configConductanceSyn cfgExc;
    cfgExc.peso = 1.0;
    cfgExc.gpeak = 10e-9; // [S]
    cfgExc.tau = 5.0 * ms;
    cfgExc.delay = 1.0 * ms;
    cfgExc.E_rev = 0.0 * mV;
    int idExc = rete.connettiNeuroni(0, 1, cfgExc);
    ok("connettiNeuroni(0→1) ConductanceSyn eccitatoria (E_rev=0mV) — ID: " + std::to_string(idExc));

    // Sinapsi inibitoria conductance-based 2→1  (E_rev ~ -70 mV, GABA-A-like)
    configConductanceSyn cfgInh;
    cfgInh.peso = 1.0;
    cfgInh.gpeak = 10e-9;
    cfgInh.tau = 10.0 * ms;
    cfgInh.delay = 1.0 * ms;
    cfgInh.E_rev = -70.0 * mV;
    int idInh = rete.connettiNeuroni(2, 1, cfgInh);
    ok("connettiNeuroni(2→1) ConductanceSyn inibitoria (E_rev=-70mV) — ID: " + std::to_string(idInh));

    // Sinapsi CurrentSyn sulla stessa rete (test coesistenza)
    configCurrentSyn cfgCur;
    cfgCur.peso = 0.5;
    cfgCur.Ipeak = 10e-9;
    cfgCur.tau = 5.0 * ms;
    cfgCur.delay = 1.0 * ms;
    int idCur = rete.connettiNeuroni(3, 1, cfgCur);
    ok("connettiNeuroni(3→1) CurrentSyn (coesistenza con ConductanceSyn) — ID: " + std::to_string(idCur));

    // Modifica della sinapsi eccitatoria
    configConductanceSyn cfgExc_mod = cfgExc;
    cfgExc_mod.gpeak = 20e-9;
    cfgExc_mod.delay = 0.5 * ms;
    rete.modificaSinapsi(idExc, cfgExc_mod);
    ok("modificaSinapsi(idExc) — gpeak aggiornato a 20e-9 S, delay a 0.5ms");

    // Errore atteso: passare configCurrentSyn a una ConductanceSyn
    info("Tentativo modificaSinapsi con config sbagliata (atteso errore su stderr):");
    rete.modificaSinapsi(idExc, configCurrentSyn{});

    // Errore atteso: passare configConductanceSyn a una CurrentSyn
    info("Tentativo modificaSinapsi con config sbagliata inversa (atteso errore su stderr):");
    rete.modificaSinapsi(idCur, configConductanceSyn{});

    // Simulazione con stimolo su 0 e 2 per attivare entrambe le sinapsi conductance
    Simulazione sim(rete, 0.1 * ms, 100.0 * ms);

    std::vector<int> ids = {0, 2, 3};
    std::vector<parametriStimoloCostante> stim = {
        {0.0, 100.0 * ms, 20e-9}, {0.0, 100.0 * ms, 20e-9}, {0.0, 100.0 * ms, 20e-9}};
    sim.iniettaStimoloCostante(ids, stim);

    sim.avviaSimulazione("test3_V.bin", "test3_F.bin", "test3_S.bin");
    ok("avviaSimulazione con sinapsi miste completata senza crash");
}

// ─────────────────────────────────────────────────────────────────────────────
// main
// ─────────────────────────────────────────────────────────────────────────────
int main() {
    test_nuova_creazione_rete();
    test_neuroni_exp();
    // test_conductance_syn();

    std::cout << "\n══════════════════════════════════════════\n";
    std::cout << "  Tutti i test completati.\n";
    std::cout << "══════════════════════════════════════════\n\n";

    return 0;
}
/*
#include "Exp.hpp"
#include "Input.hpp"
#include "Neurone.hpp"
#include "Rete.hpp"
#include "Simulazione.hpp"
#include "UnitaSI.hpp"

#include <iostream>
#include <print>
#include <random>
#include <vector>

double rumoreGaussiano(double media, double deviazioneStandard) {
    static std::random_device rd;
    static std::mt19937 gen(rd());

    std::normal_distribution<double> distribuzione(media, deviazioneStandard);

    return distribuzione(gen);
}

double rumoreUniforme(double centro, double ampiezza) {

    static std::random_device rd;
    static std::mt19937 gen(rd());

    std::uniform_real_distribution<double> distribuzione(centro - ampiezza, centro + ampiezza);

    return distribuzione(gen);
}

int main() {

    int N = 100;
    Rete rete(N, Label_Type_Neuron::Exp);


    for (int i = 0; i < N / 2; i++) {
        configExp configPersonalizzaNeuroneExp;
        configPersonalizzaNeuroneExp.C = 100 * i;

        rete.modificaParametriNeurone(i, configPersonalizzaNeuroneExp);
    }

    s
    Simulazione sim(rete, 0.01 * ms, 30 * ms);

    parametriStimoloCostante p;
    p.timeStart = 5.0 * ms;
    p.timeEnd = 15.0 * ms;
    p.ampiezza = 20.0 * n * A;

    std::vector<int> v(1, 0);
    std::vector<parametriStimoloCostante> vp;
    vp.push_back(p);
    sim.iniettaStimoloCostante(v, vp);

    sim.avviaSimulazione("potenzialitest1.bin", "firingtest1.bin", "sinapsitest1.bin");

    std::cout << "\nFINE SIMULAZIONE \n";
    */
/*
std::cout << "=========================================================" << std::endl;
std::cout << "--- BENCHMARK PER SAMPLY: Rete Biologica Sparsa (15k) ---" << std::endl;
std::cout << "=========================================================" << std::endl;

// 1. Parametri di Scala della Rete
const int N = 150;
const int Ne = static_cast<int>(N * 0.8);
//const int Ni = N - Ne;

// Parametri Neuroni
const double eV_th = -50.0 * mV,  eV_rest = -65.0 * mV,  eV_reset = -65.0 * mV;
const double eR = 100.0 * Mohm,  eC = 200.0 * p * F,    eTau_ref = 3.5 * ms;

const double iV_th = -50.0 * mV,  iV_rest = -65.0 * mV,  iV_reset = -65.0 * mV;
const double iR = 50.0 * Mohm,   iC = 200.0 * p * F,    iTau_ref = 0.5 * ms;

Rete rete;

std::cout << "[1/3] Allocazione di " << N << " neuroni..." << std::endl;
for (int i = 0; i < N; ++i) {
    double V_0 = eV_reset + rumoreGaussiano(0, 2.0 * mV);
    if (i < Ne) {
        rete.aggiungiNeurone(Neurone(i, V_0, eV_th, eV_rest, eV_reset, eR, eC, eTau_ref));
    } else {
        rete.aggiungiNeurone(Neurone(i, V_0, iV_th, iV_rest, iV_reset, iR, iC, iTau_ref));
    }
}

// 2. Generazione Connessioni Sparse (Probabilità 10%)
std::cout << "[2/3] Generazione sinapsi sparse (Connettivita' 10%)..." << std::endl;
const double eWeight = +1.0, eIpeak = 1.5 * n * A, eTau_syn = 5.0 * ms;
const double iWeight = -3.0, iIpeak = 1.5 * n * A, iTau_syn = 10.0 * ms;

std::mt19937 gen(12345); // Seed fisso per replicabilità
std::uniform_real_distribution<double> dist_conn(0.0, 1.0);

for (int i = 0; i < N; ++i) {
    for (int j = 0; j < N; ++j) {
        if (dist_conn(gen) < 0.10) { // 10% di probabilità di connessione
            if (i < Ne) {
                Sinapsi s(eWeight, eIpeak, i, j, eTau_syn);
                rete.connettiNeuroni(s);
            } else {
                Sinapsi s(iWeight, iIpeak, i, j, iTau_syn);
                rete.connettiNeuroni(s);
            }
        }
    }
}

std::cout << " -> Totale Sinapsi Istanziate: " << rete.getNumSinapsi() << std::endl;

// 3. Configurazione Simulazione
double dt = 0.1 * ms;
double T = 2.0 * ms; // 5000 step temporali
Simulazione sim(rete, dt, T);

// Stimolo iniziale per attivare la dinamica (sui primi 100 neuroni eccitatori)
std::vector<int> idStimolati;
std::vector<parametriStimoloCostante> paramStimoli;
for(int i = 0; i < 100; ++i) {
    idStimolati.push_back(i);
    parametriStimoloCostante p;
    p.timeStart = 0.0 * ms;
    p.timeEnd = 20.0 * ms;
    p.ampiezza = 5.0 * n * A;
    paramStimoli.push_back(p);
}
sim.iniettaStimoloCostante(idStimolati, paramStimoli);

std::cout << "[3/3] Setup completato. Avvio del calcolo numerico..." << std::endl;

// =========================================================
// ESECUZIONE SIMULAZIONE (IL LOOP COMPLETO SOTTO PROFILING)
// =========================================================
sim.avviaSimulazione("potenziali.bin", "firing.bin", "sinapsi.bin");

std::cout << "=========================================================" << std::endl;
std::cout << " COMPILATION COMPLETED " << std::endl;
std::cout << "=========================================================" << std::endl;

std::cout << "--- Test Ring Network (Propagazione ad Anello) ---" << std::endl;

// 1. Creiamo una rete con 5 neuroni (ID da 0 a 4)
const int numNeuroni = 50;
Rete rete(numNeuroni);

// 2. Connettiamo i neuroni in un anello chiuso (Ring Topology)
std::cout << "[CONFIG] Creazione delle connessioni sinaptiche ad anello..." << std::endl;

// Connettiamo 0->1, 1->2, 2->3, 3->4
for (int i = 0; i < numNeuroni - 1; ++i) {
    Sinapsi s(1.0, 30.0 * n * A, i, i + 1, 4.0 * ms);
    rete.connettiNeuroni(s);
}
// Chiudiamo l'anello connettendo l'ultimo (4) al primo (0)
Sinapsi s_chiusura(1.0, 30.0 * n * A, numNeuroni - 1, 0, 4.0 * ms);
rete.connettiNeuroni(s_chiusura);

// 3. Inizializziamo i parametri della simulazione
double dt = 0.1 * ms;
double T = 500.0 * ms; // Durata sufficiente per vedere più giri dell'anello
Simulazione sim(rete, dt, T);

// 4. CONFIGURAZIONE STIMOLO COSTANTE — SOLO SUL NEURONE 0
std::vector<int> idCostante = {0};
std::vector<parametriStimoloSeno> paramSeno;

parametriStimoloSeno pCost;
pCost.timeStart = 50.0 * ms;   // Lo stimolo parte a 5 ms
pCost.timeEnd = 500.0 * ms;    // Lo stimolo si interrompe a 25 ms
pCost.ampiezza = 40.0 * n * A; // Corrente forte sul primo neurone per innescare il ciclo
pCost.fase = 0.0;
pCost.frequenza = 40 * Hz;
paramSeno.push_back(pCost);

std::cout << "[TEST] Iniezione stimolo costante SOLO sul neurone 0..." << std::endl;
sim.iniettaStimoloSeno(idCostante, paramSeno);

// 5. ESECUZIONE SIMULAZIONE
std::cout << "\n[TEST] Avvio della simulazione ad anello..." << std::endl;
sim.avviaSimulazione("test_potenziali.bin", "test_firing.bin", "test_sinapsi.bin");

std::cout << "--- Simulazione completata! ---" << std::endl;
*/
// =========================================================
// PARAMETRI RETE
// =========================================================
/*
const int N = 1000;

const int Ne = N * 0.8;
const int Ni = N * 0.2;

// Neurone eccitatorio (tipo piramidale)
const double eV_rest = -65.0 * mV;
const double eV_reset = -65.0 * mV;
const double eV_th = -50.0 * mV;
const double eR = 100 * Mohm;
const double eC = 200 * p * F;
const double eTau_m = eR * eC;
const double eTau_ref = 3.5 * ms;

// neurone inibitorio (interneuroni fast-spiking)
const double iV_rest = -65.0 * mV;
const double iV_reset = -65.0 * mV;
const double iV_th = -50.0 * mV;
const double iR = 50 * Mohm;
const double iC = 200 * p * F;
const double iTau_m = iR * iC;
const double iTau_ref = 0.5 * ms;

Rete rete;

// creo i neuroni eccitatori e inibitori
for (int i = 0; i < N; ++i) {
    double V_0 = eV_reset + rumoreGaussiano(0, 2 * mV);
    // std :: cout << "Potenziale iniziale : " << V_0 << " = " << eV_reset << " + " << V_0-eV_reset << "\n";
    if (i < Ne) {
        Neurone neurone(i, V_0, eV_th, eV_rest, eV_reset, eR, eC, eTau_ref);
        rete.aggiungiNeurone(neurone);
    } else {
        Neurone neurone(i, V_0, iV_th, iV_rest, iV_reset, iR, iC, iTau_ref);
        rete.aggiungiNeurone(neurone);
    }
}

// sinapsi eccitatorie
const double eWeight = +0.2;
const double eIpeak = 0.1 * n * A;
const double eTau_syn = 5 * ms;

// sinapsi inibitorie
const double iWeight = -1.0;
const double iIpeak = 0.1 * n * A;
const double iTau_syn = 10 * ms;

for (int i = 0; i < N; ++i) {
    for (int j = 0; j < N; j++) {
        double pCorrente = rumoreUniforme(0.5, 0.5);
        if (pCorrente < 0.2) {
            if (i < Ne) {
                Sinapsi s(eWeight, eIpeak, i, j, eTau_syn);
                rete.connettiNeuroni(s);
            } else {
                Sinapsi s(iWeight, iIpeak, i, j, iTau_syn);
                rete.connettiNeuroni(s);
            }
        }
    }
}

// dati simulazione
double dt = 0.1 * ms;
double T = 500.0 * ms;

Simulazione sim(rete, dt, T);

// input esterno

int stepTotali = static_cast<int>(T / dt);

std::vector<Input> inputEsterno;

double inputCorrente = 0.0;
for(int l=0;l< N; l++){
    Input i;
    std::vector<double> v;
    i.id = l;

    if (l < Ne)
        inputCorrente = 0.15 * n*A + rumoreGaussiano(0,0.02*n*A);
    else
        inputCorrente = 0.30 * n * A + rumoreGaussiano(0,0.02*n*A);

    for(int k=0;k<stepTotali;k++)
        v.push_back(inputCorrente);

    i.valori = v;
    v.clear();
    inputEsterno.push_back(i);
}

sim.aggiungiInputEsterni(inputEsterno);


// =========================================================
// AVVIO SIMULAZIONE
// =========================================================

sim.avviaSimulazione("potenziali.bin", "firing.bin", "sinapsi.bin");

std::cout << "Simulazione completata.\n";
return 0;



// rete e neuroni
const int N = 500;
Rete rete(N);

// sinapsi
for (int i = 0; i < N - 1; ++i) {
    Sinapsi s(1, 20 * n * A, i, i + 1, 5 * ms);
    rete.connettiNeuroni(s);
}

Sinapsi s(1, 20 * n * A, N - 1, 0, 5 * ms);
rete.connettiNeuroni(s);

// simulazione
double dt = 0.1 * ms;
double T = 500.0 * ms;

Simulazione sim(rete, dt, T);

// input esterno
int stepTotali = static_cast<int>(T / dt);

//std::vector<Input> inputEsterno;

std::vector<double> stimolo(stepTotali, 0.0);

std::fill(stimolo.begin() + 300, stimolo.begin() + 300 + 1, 20 * n * A);

//Input i;

//i.id = 0;
//i.valori = stimolo;

//inputEsterno.push_back(i);

//sim.aggiungiInputEsterni(inputEsterno);

sim.avviaSimulazione("potenziali.bin", "firing.bin", "sinapsi.bin");


return 0;
}
*/