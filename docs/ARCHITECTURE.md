# Architecture

This document describes the high-level architecture of HeaderRN and the interaction between its main components.

The goal is to provide an overview of the framework design without entering implementation details.

---

# Design Overview

HeaderRN follows a classic compiled-library structure: declarations in `.hpp` header files, implementations in separate `.cpp` source files.

The framework is organized around four main components:

* `Neurone`
* `Sinapsi`
* `Rete`
* `Simulazione`

External stimuli are not a separate class but are defined through parameter structs (`parametriStimoloCostante`, `parametriStimoloSeno`) declared in `Input.hpp` and managed internally by `Simulazione`.

Each component has a single well-defined responsibility.

This separation keeps the framework modular, easy to understand and extensible.

---

# Main Components

## Neurone

The `Neurone` class represents a single spiking neuron.

Current implementation:

* Leaky Integrate-and-Fire (LIF) model
* Absolute refractory period
* Explicit biological parameters
* State update through numerical integration (forward Euler)

### Responsibilities

The neuron is responsible for:

- storing its internal state
- updating its membrane potential
- managing the refractory period
- generating spikes

### Knows

The neuron knows:

- its membrane potential
- its biological parameters
- its refractory state

### Does Not Know

The neuron does not know:

- the network topology
- other neurons
- synaptic connections
- simulation time

---

## Sinapsi

The `Sinapsi` class represents a connection between two neurons.

Current implementation:

* Current-based synapse
* Exponential current decay
* Positive weights for excitatory connections
* Negative weights for inhibitory connections


### Responsibilities

The synapse is responsible for:

- propagating activity between neurons
- generating synaptic current
- storing synaptic parameters

### Knows

The synapse knows:

- presynaptic neuron id
- postsynaptic neuron id
- synaptic weight
- synaptic state variables

### Does Not Know

The synapse does not know:

- global network structure
- simulation management
- external inputs

---

## Rete

The `Rete` class stores the network structure and advances its dynamics step by step.

The network is represented as:

* a collection of neurons
* a collection of synaptic connections

rather than by an adjacency matrix.

This representation simplifies future extensions such as:

* transmission delays
* plasticity mechanisms
* state-dependent synapses

### Responsibilities

The network is responsible for:

- storing neurons
- storing synaptic connections
- managing connectivity
- advancing the network state by one time step (`step`)
- accumulating afferent currents (synaptic + external stimuli) into an internal buffer

### Knows

The network knows:

- all neurons
- all synapses
- connectivity relationships

### Does Not Know

The network does not know:

- simulation duration
- output files
- data analysis
- stimulus definitions

---

## Simulazione

The `Simulazione` class coordinates the simulation process.

### Responsibilities

The simulation is responsible for:

- managing simulation time
- evaluating and applying external stimuli at each step
- advancing network dynamics
- buffering and writing simulation outputs to binary files

### External Stimuli

Stimuli are injected before running the simulation via:

- `iniettaStimoloCostante` — constant current over a time window
- `iniettaStimoloSeno` — sinusoidal current over a time window

Stimuli are stored in an internal registry and evaluated on-the-fly at each step. No pre-allocated value arrays are needed.

### Knows

The simulation knows:

- the network
- the simulation parameters
- the stimulus registry
- output destinations and buffer state

### Does Not Know

The simulation does not know:

- the internal implementation details of neurons
- the internal implementation details of synapses

---

# Output Format

Simulation data is exported to three binary files:

- membrane potentials (one value per neuron per step)
- firing states (1.0 = spike, 0.0 = no spike, one per neuron per step)
- synaptic currents (one value per synapse per step)

Each file begins with a 4-byte `int32` header containing the number of columns (time + data). All values are stored as `double` (64-bit). Output is buffered in memory and flushed to disk in blocks to minimize I/O overhead.

---


