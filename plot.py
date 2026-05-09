import matplotlib.pyplot as plt
import numpy as np

# ==========================================================
# LOAD DATA
# ==========================================================

pot_data = np.loadtxt("potenziali.txt")
fire_data = np.loadtxt("firing.txt")
syn_data = np.loadtxt("sinapsi.txt")

# ==========================================================
# EXTRACT DATA
# ==========================================================

# time in seconds -> ms
time = pot_data[:, 0] * 1000.0

# membrane potentials in Volt -> mV
potentials = pot_data[:, 1:] * 1000.0

# spikes
spikes = fire_data[:, 1:]

# synaptic currents in A -> nA
synapses = syn_data[:, 1:] * 1e9

# fix dimensions if only one neuron/synapse
if potentials.ndim == 1:
    potentials = potentials[:, np.newaxis]

if spikes.ndim == 1:
    spikes = spikes[:, np.newaxis]

if synapses.ndim == 1:
    synapses = synapses[:, np.newaxis]

n_neurons = spikes.shape[1]
n_synapses = synapses.shape[1]

# ==========================================================
# BUILD RASTER DATA
# ==========================================================

spike_times = []

for neuron_idx in range(n_neurons):

    neuron_spikes = time[spikes[:, neuron_idx] == 1]

    spike_times.append(neuron_spikes)

# ==========================================================
# FIGURE
# ==========================================================

fig, axes = plt.subplots(
    4,
    1,
    figsize=(14, 12),
    sharex=True
)

# ==========================================================
# 1) RASTER PLOT
# ==========================================================

axes[0].eventplot(
    spike_times,
    colors='black',
    lineoffsets=np.arange(n_neurons),
    linelengths=0.8
)

axes[0].set_title("Ring network raster plot")
axes[0].set_ylabel("Neuron")
axes[0].set_yticks(np.arange(n_neurons))

# ==========================================================
# 2) MEMBRANE POTENTIALS
# ==========================================================

for i in range(n_neurons):

    axes[1].plot(
        time,
        potentials[:, i],
        linewidth=1
    )

axes[1].set_title("Membrane potentials")
axes[1].set_ylabel("Potential (mV)")
axes[1].grid(True, alpha=0.3)

# ==========================================================
# 3) SPIKE TRAINS
# ==========================================================

for i in range(n_neurons):

    spike_signal = spikes[:, i]

    axes[2].plot(
        time,
        spike_signal + i * 1.2,
        linewidth=1
    )

axes[2].set_title("Spike trains")
axes[2].set_ylabel("Neuron index")
axes[2].grid(True, alpha=0.3)

# ==========================================================
# 4) SYNAPTIC CURRENTS
# ==========================================================

for i in range(n_synapses):

    axes[3].plot(
        time,
        synapses[:, i],
        linewidth=1
    )

axes[3].set_title("Synaptic currents")
axes[3].set_ylabel("Current (nA)")
axes[3].set_xlabel("Time (ms)")
axes[3].grid(True, alpha=0.3)

# ==========================================================
# LAYOUT
# ==========================================================

plt.tight_layout()
plt.show()