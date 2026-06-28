"""
visualizza_test.py — Visualizzazione e verifica dei file di output di SpikeNNsim
=================================================================================

Formato binario atteso (da Simulazione.cpp):
  - Header: 4 byte (int32) con numero di colonne ncols = 1 + N_neuroni (o N_sinapsi)
  - Dati: sequenza di righe da ncols * 8 byte (float64), prima colonna = tempo [s]

Eseguire dalla cartella che contiene la cartella output/:
  python visualizza_test.py
"""

import struct
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.gridspec as gridspec
import os
import sys


# ─────────────────────────────────────────────────────────────────────────────
# Parser binario
# ─────────────────────────────────────────────────────────────────────────────

def leggi_file(path: str) -> np.ndarray:
    """
    Legge un file binario prodotto da Simulazione.
    Ritorna array shape (n_step, ncols): colonna 0 = tempo, resto = dati.
    """
    if not os.path.exists(path):
        print(f"  [MANCANTE] {path}")
        return None

    with open(path, "rb") as f:
        raw = f.read()

    # Header: primo int32 = numero colonne
    ncols = struct.unpack_from("<i", raw, 0)[0]
    dati_raw = raw[4:]  # tutto il resto sono double

    n_valori = len(dati_raw) // 8
    if n_valori % ncols != 0:
        print(f"  [ERRORE] {path}: n_valori={n_valori} non divisibile per ncols={ncols}")
        return None

    arr = np.frombuffer(dati_raw, dtype="<f8").reshape(-1, ncols)
    print(f"  [OK] {os.path.basename(path)}: {arr.shape[0]} step, {ncols-1} canali dati")
    return arr


def split(arr):
    """Separa tempo e dati."""
    return arr[:, 0], arr[:, 1:]


# ─────────────────────────────────────────────────────────────────────────────
# Funzioni di verifica (stampano esito su console)
# ─────────────────────────────────────────────────────────────────────────────

def verifica_tempo_monotono(t, nome):
    if np.all(np.diff(t) > 0):
        print(f"  [OK] {nome}: tempo strettamente crescente")
    else:
        print(f"  [ERRORE] {nome}: tempo NON monotono")

def verifica_potenziali_range(V, nome, V_min=-0.120, V_max=0.060):
    """Valori in volt: range biologico ragionevole [-120 mV, +60 mV]."""
    fuori = np.sum((V < V_min) | (V > V_max))
    if fuori == 0:
        print(f"  [OK] {nome}: tutti i potenziali nel range [{V_min*1e3:.0f}, {V_max*1e3:.0f}] mV")
    else:
        print(f"  [ATTENZIONE] {nome}: {fuori} valori fuori range biologico")

def verifica_firing_binario(F, nome):
    """I valori di firing devono essere solo 0.0 o 1.0."""
    valori_unici = np.unique(F)
    if np.all(np.isin(valori_unici, [0.0, 1.0])):
        n_spike = int(np.sum(F))
        print(f"  [OK] {nome}: firing binario, spike totali = {n_spike}")
    else:
        print(f"  [ERRORE] {nome}: valori firing non binari: {valori_unici}")

def verifica_sinapsi_decadimento(S, nome):
    """
    Verifica qualitativa: la corrente sinaptica non dovrebbe divergere.
    Dopo un burst iniziale ci si aspetta un decadimento verso zero.
    """
    picco = np.max(np.abs(S))
    if picco == 0.0:
        print(f"  [ATTENZIONE] {nome}: corrente sinaptica sempre zero (nessuno spike pre?)")
    elif picco < 1e-6:   # sotto 1 µA è ragionevole per correnti sinaptiche
        print(f"  [OK] {nome}: picco corrente sinaptica = {picco*1e12:.2f} pA")
    else:
        print(f"  [ATTENZIONE] {nome}: picco corrente sinaptica molto alto = {picco:.3e} A")


# ─────────────────────────────────────────────────────────────────────────────
# Plot helpers
# ─────────────────────────────────────────────────────────────────────────────

COLORI = ["#4C72B0", "#DD8452", "#55A868", "#C44E52",
          "#8172B3", "#937860", "#DA8BC3", "#8C8C8C"]

def colore(i):
    return COLORI[i % len(COLORI)]

def plot_potenziali(ax, t, V, etichette=None):
    ms = t * 1e3
    for i in range(V.shape[1]):
        label = etichette[i] if etichette else f"N{i}"
        ax.plot(ms, V[:, i] * 1e3, color=colore(i), lw=0.9, label=label)
    ax.set_ylabel("V (mV)")
    ax.legend(fontsize=7, loc="upper right")
    ax.grid(True, alpha=0.3)

def plot_raster(ax, t, F, etichette=None):
    ms = t * 1e3
    for i in range(F.shape[1]):
        spike_t = ms[F[:, i] == 1.0]
        label = etichette[i] if etichette else f"N{i}"
        ax.scatter(spike_t, np.full_like(spike_t, i),
                   marker="|", s=60, color=colore(i), label=label)
    ax.set_ylabel("Neurone")
    ax.set_yticks(range(F.shape[1]))
    ax.set_yticklabels(etichette if etichette else [f"N{i}" for i in range(F.shape[1])], fontsize=7)
    ax.grid(True, alpha=0.3)

def plot_sinapsi(ax, t, S, etichette=None):
    ms = t * 1e3
    for i in range(S.shape[1]):
        label = etichette[i] if etichette else f"Syn{i}"
        ax.plot(ms, S[:, i] * 1e12, color=colore(i), lw=0.9, label=label)
    ax.set_ylabel("I_syn (pA)")
    ax.set_xlabel("Tempo (ms)")
    ax.legend(fontsize=7, loc="upper right")
    ax.grid(True, alpha=0.3)


def crea_figura(titolo, nrighe=3):
    fig = plt.figure(figsize=(12, 3 * nrighe))
    fig.suptitle(titolo, fontsize=11, fontweight="bold")
    gs = gridspec.GridSpec(nrighe, 1, hspace=0.45)
    assi = [fig.add_subplot(gs[i]) for i in range(nrighe)]
    return fig, assi


# ─────────────────────────────────────────────────────────────────────────────
# TEST 1 — LIF + CurrentSyn, nuova modalità di creazione
#
# Rete: 4 neuroni LIF (ID 0-3)
# Stimolo costante su N0 per tutta la simulazione (50 ms)
# Sinapsi:
#   Syn0: 0→1 (eccitatoria, peso=1.0 dopo modifica)
#   Syn1: 0→1 (eccitatoria, peso=0.3, seconda sinapsi stessa coppia)
#   Syn2: 1→2 (inibitoria, peso=-0.5)
# Atteso:
#   N0 spara regolarmente (stimolato direttamente)
#   N1 riceve corrente eccitatoria da N0 → dovrebbe sparare
#   N2 riceve corrente inibitoria da N1 → difficilmente spara
#   N3 silenzioso (nessun input)
# ─────────────────────────────────────────────────────────────────────────────

def analizza_test1(cartella):
    print("\n══════════════════════════════════════════")
    print("  TEST 1 — LIF + CurrentSyn")
    print("══════════════════════════════════════════")

    V_arr = leggi_file(f"{cartella}/test1_V.bin")
    F_arr = leggi_file(f"{cartella}/test1_F.bin")
    S_arr = leggi_file(f"{cartella}/test1_S.bin")

    if V_arr is None or F_arr is None or S_arr is None:
        print("  File mancanti, test saltato.")
        return

    t, V = split(V_arr)
    _, F = split(F_arr)
    _, S = split(S_arr)

    # Verifiche
    verifica_tempo_monotono(t, "test1_V")
    verifica_potenziali_range(V, "test1")
    verifica_firing_binario(F, "test1")
    verifica_sinapsi_decadimento(S, "test1")

    # Verifica qualitativa attesa
    spike_N0 = int(np.sum(F[:, 0]))
    spike_N3 = int(np.sum(F[:, 3]))
    print(f"  [INFO] N0 (stimolato): {spike_N0} spike — atteso > 0")
    print(f"  [INFO] N3 (silenzioso): {spike_N3} spike — atteso = 0")
    if spike_N0 > 0:
        print("  [OK] N0 spara come atteso")
    else:
        print("  [ATTENZIONE] N0 non ha sparato — controllare stimolo o parametri")
    if spike_N3 == 0:
        print("  [OK] N3 silenzioso come atteso")
    else:
        print("  [ATTENZIONE] N3 ha sparato inaspettatamente")

    # Plot
    etN = ["N0 (stim)", "N1", "N2", "N3 (sil.)"]
    etS = ["Syn0: 0→1 (exc)", "Syn1: 0→1 (exc2)", "Syn2: 1→2 (inh)"]

    fig, assi = crea_figura("TEST 1 — LIF + CurrentSyn  |  nuova modalità creazione rete")
    plot_potenziali(assi[0], t, V, etN)
    plot_raster(assi[1], t, F, etN)
    plot_sinapsi(assi[2], t, S, etS)
    assi[0].set_title("Potenziali di membrana", fontsize=9)
    assi[1].set_title("Raster plot (spike)", fontsize=9)
    assi[2].set_title("Correnti sinaptiche", fontsize=9)
    #plt.savefig(f"{cartella}/test1_plot.png", dpi=150, bbox_inches="tight")
    #print(f"  [SALVATO] {cartella}/test1_plot.png")
    #plt.close(fig)
    plt.show()


# ─────────────────────────────────────────────────────────────────────────────
# TEST 2 — Neuroni Exp + Runge-Kutta
#
# Rete: 3 neuroni Exp con RK4 (ID 0-2)
# Stimolo costante su N0 per tutta la simulazione (50 ms)
# Sinapsi:
#   Syn0: 0→1 (CurrentSyn, eccitatoria)
# Atteso:
#   N0 spara (modello Exp ha la nonlinearità esponenziale → soglia più morbida)
#   N1 riceve corrente da N0 → dovrebbe sparare
#   N2 silenzioso
# ─────────────────────────────────────────────────────────────────────────────

def analizza_test2(cartella):
    print("\n══════════════════════════════════════════")
    print("  TEST 2 — Neuroni Exp + Runge-Kutta")
    print("══════════════════════════════════════════")

    V_arr = leggi_file(f"{cartella}/test2_V.bin")
    F_arr = leggi_file(f"{cartella}/test2_F.bin")
    S_arr = leggi_file(f"{cartella}/test2_S.bin")

    if V_arr is None or F_arr is None or S_arr is None:
        print("  File mancanti, test saltato.")
        return

    t, V = split(V_arr)
    _, F = split(F_arr)
    _, S = split(S_arr)

    verifica_tempo_monotono(t, "test2_V")
    verifica_potenziali_range(V, "test2")
    verifica_firing_binario(F, "test2")
    verifica_sinapsi_decadimento(S, "test2")

    etN = ["N0 (stim, Exp)", "N1 (Exp)", "N2 (sil., Exp)"]
    etS = ["Syn0: 0→1 (CurrentSyn)"]

    spike_N0 = int(np.sum(F[:, 0]))
    spike_N2 = int(np.sum(F[:, 2]))
    print(f"  [INFO] N0 (stimolato, Exp+RK4): {spike_N0} spike — atteso > 0")
    print(f"  [INFO] N2 (silenzioso): {spike_N2} spike — atteso = 0")
    if spike_N0 > 0:
        print("  [OK] N0 Exp spara come atteso")
    else:
        print("  [ATTENZIONE] N0 Exp non ha sparato")
    if spike_N2 == 0:
        print("  [OK] N2 silenzioso come atteso")
    else:
        print("  [ATTENZIONE] N2 ha sparato inaspettatamente")

    fig, assi = crea_figura("TEST 2 — Neuroni Exp + Runge-Kutta")
    plot_potenziali(assi[0], t, V, etN)
    plot_raster(assi[1], t, F, etN)
    plot_sinapsi(assi[2], t, S, etS)
    assi[0].set_title("Potenziali di membrana", fontsize=9)
    assi[1].set_title("Raster plot (spike)", fontsize=9)
    assi[2].set_title("Correnti sinaptiche (CurrentSyn)", fontsize=9)
    #plt.savefig(f"{cartella}/test2_plot.png", dpi=150, bbox_inches="tight")
    #print(f"  [SALVATO] {cartella}/test2_plot.png")
    #plt.close(fig)
    plt.show()




  



# ─────────────────────────────────────────────────────────────────────────────
# TEST 3 — ConductanceSyn (sinapsi miste)
#
# Rete: 4 neuroni LIF (ID 0-3)
# Stimolo costante su N0, N2, N3 per tutta la simulazione (100 ms)
# Sinapsi:
#   Syn0: 0→1 ConductanceSyn eccitatoria  (E_rev=0 mV)
#   Syn1: 2→1 ConductanceSyn inibitoria   (E_rev=-70 mV)
#   Syn2: 3→1 CurrentSyn     eccitatoria
# Atteso:
#   N0, N2, N3 sparano (tutti stimolati)
#   N1 riceve input eccitatorio da N0 e N3, inibitorio da N2
#     → dipende dal bilancio, ma almeno le correnti devono essere non nulle
#   Corrente Syn0 e Syn1: dipendono da V_post di N1 → non sono fisse
#   Corrente Syn2: CurrentSyn, indipendente da V_post
# ─────────────────────────────────────────────────────────────────────────────

def analizza_test3(cartella):
    print("\n══════════════════════════════════════════")
    print("  TEST 3 — ConductanceSyn (sinapsi miste)")
    print("══════════════════════════════════════════")

    V_arr = leggi_file(f"{cartella}/test3_V.bin")
    F_arr = leggi_file(f"{cartella}/test3_F.bin")
    S_arr = leggi_file(f"{cartella}/test3_S.bin")

    if V_arr is None or F_arr is None or S_arr is None:
        print("  File mancanti, test saltato.")
        return

    t, V = split(V_arr)
    _, F = split(F_arr)
    _, S = split(S_arr)

    verifica_tempo_monotono(t, "test3_V")
    verifica_potenziali_range(V, "test3")
    verifica_firing_binario(F, "test3")

    # Le correnti ConductanceSyn cambiano segno in base a V_post: non usiamo
    # verifica_sinapsi_decadimento generica ma verifichiamo caso per caso
    for i, nome in enumerate(["Syn0 (Cond.exc)", "Syn1 (Cond.inh)", "Syn2 (Curr.exc)"]):
        picco = np.max(np.abs(S[:, i]))
        print(f"  [INFO] {nome}: picco |I_syn| = {picco*1e12:.2f} pA")

    # Verifica attesa: N0, N2, N3 devono sparare
    for idx, nome in [(0, "N0"), (2, "N2"), (3, "N3")]:
        n = int(np.sum(F[:, idx]))
        stato = "OK" if n > 0 else "ATTENZIONE"
        print(f"  [{stato}] {nome} (stimolato): {n} spike")

    # Verifica ConductanceSyn: la corrente di Syn0 (eccitatoria) deve essere
    # positiva quando V_post < E_rev=0, cioè quasi sempre per un LIF a riposo
    frac_pos_syn0 = np.mean(S[:, 0] > 0)
    frac_neg_syn1 = np.mean(S[:, 1] < 0)
    print(f"  [INFO] Syn0 (exc, E_rev=0mV): {frac_pos_syn0*100:.1f}% del tempo corrente positiva — atteso ~100%")
    print(f"  [INFO] Syn1 (inh, E_rev=-70mV): {frac_neg_syn1*100:.1f}% del tempo corrente negativa — atteso ~100%")
    if frac_pos_syn0 > 0.9:
        print("  [OK] Syn0 eccitatoria: segno corretto")
    else:
        print("  [ATTENZIONE] Syn0: segno inatteso, controllare E_rev")
    if frac_neg_syn1 > 0.9:
        print("  [OK] Syn1 inibitoria: segno corretto")
    else:
        print("  [ATTENZIONE] Syn1: segno inatteso, controllare E_rev")

    etN = ["N0 (stim)", "N1", "N2 (stim)", "N3 (stim)"]
    etS = ["Syn0: 0→1 Cond.exc (E_rev=0mV)",
           "Syn1: 2→1 Cond.inh (E_rev=-70mV)",
           "Syn2: 3→1 Curr.exc"]

    # Plot: 4 pannelli — potenziali, raster, sinapsi conductance, sinapsi current
    fig = plt.figure(figsize=(12, 12))
    fig.suptitle("TEST 3 — ConductanceSyn (sinapsi miste)", fontsize=11, fontweight="bold")
    gs = gridspec.GridSpec(4, 1, hspace=0.45)
    assi = [fig.add_subplot(gs[i]) for i in range(4)]

    plot_potenziali(assi[0], t, V, etN)
    assi[0].set_title("Potenziali di membrana", fontsize=9)

    plot_raster(assi[1], t, F, etN)
    assi[1].set_title("Raster plot (spike)", fontsize=9)

    # Syn0 e Syn1 (conductance-based) — stesso pannello per confronto
    ms = t * 1e3
    assi[2].plot(ms, S[:, 0] * 1e12, color=colore(0), lw=0.9, label=etS[0])
    assi[2].plot(ms, S[:, 1] * 1e12, color=colore(1), lw=0.9, label=etS[1])
    assi[2].axhline(0, color="k", lw=0.5, ls="--")
    assi[2].set_ylabel("I_syn (pA)")
    assi[2].set_title("Correnti ConductanceSyn (segno dipende da V_post - E_rev)", fontsize=9)
    assi[2].legend(fontsize=7)
    assi[2].grid(True, alpha=0.3)

    # Syn2 (current-based) — separata per evidenziare la differenza di comportamento
    assi[3].plot(ms, S[:, 2] * 1e12, color=colore(2), lw=0.9, label=etS[2])
    assi[3].set_ylabel("I_syn (pA)")
    assi[3].set_xlabel("Tempo (ms)")
    assi[3].set_title("Corrente CurrentSyn (indipendente da V_post)", fontsize=9)
    assi[3].legend(fontsize=7)
    assi[3].grid(True, alpha=0.3)

    plt.savefig(f"{cartella}/test3_plot.png", dpi=150, bbox_inches="tight")
    print(f"  [SALVATO] {cartella}/test3_plot.png")
    plt.close(fig)


# ─────────────────────────────────────────────────────────────────────────────
# main
# ─────────────────────────────────────────────────────────────────────────────

def main():
    cartella = "output"
    if len(sys.argv) > 1:
        cartella = sys.argv[1]

    if not os.path.isdir(cartella):
        print(f"Cartella '{cartella}' non trovata.")
        print("Uso: python visualizza_test.py [cartella_output]")
        sys.exit(1)

    analizza_test1(cartella)
    analizza_test2(cartella)
    analizza_test3(cartella)

    print("\n══════════════════════════════════════════")
    print("  Analisi completata.")
    print("  Plot salvati in:", cartella)
    print("══════════════════════════════════════════\n")


if __name__ == "__main__":
    main()
