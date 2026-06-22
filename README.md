# HeaderRN

HeaderRN is a C++ framework for simulating spiking neural networks.

The library follows a classic compiled-library structure: declarations in `.hpp` header files, implementations in separate `.cpp` source files.

The library provides a modular architecture for building networks of neurons and synapses, running time-domain simulations, and exporting data for external analysis.

HeaderRN was originally developed as a personal project to understand how spiking neural network simulations work at a low level.

Instead of relying on existing simulation frameworks, the goal was to build the fundamental components from scratch, including neurons, synapses, network topology and simulation management.

Its design is guided by three main principles:

- clarity and educational value;
- simplicity of use;
- extensibility toward more advanced neural models.


## Features

Current features include:

- Leaky Integrate-and-Fire (LIF) neurons
- Absolute refractory period
- Current-based synapses with exponential decay
- Excitatory and inhibitory synaptic connections
- Arbitrary network topologies
- Time-dependent external stimuli (constant and sinusoidal)
- Explicit physical units system
- Time-domain simulations
- Buffered binary data export for post-processing
- Modular object-oriented architecture

## Design Philosophy

HeaderRN follows a simple object-oriented design.

The main components of a simulation are represented by dedicated classes:

- `Rete`
- `Neurone`
- `Sinapsi`
- `Simulazione`

This separation aims to keep the code readable and extensible.

---

## Library Structure

```text
include/
  Neurone.hpp      -> neuron model declarations
  Sinapsi.hpp      -> synapse model declarations
  Rete.hpp         -> network topology declarations
  Simulazione.hpp  -> simulation engine declarations
  Input.hpp        -> external stimulus types
  UnitaSI.hpp      -> physical units
  Utility.hpp      -> system utilities

src/
  Neurone.cpp      -> neuron model implementation
  Sinapsi.cpp      -> synapse model implementation
  Rete.cpp         -> network topology implementation
  Simulazione.cpp  -> simulation engine implementation
  Utility.cpp      -> system utilities implementation
```

## Example

A minimal simulation consists of:

1. Creating a network.
2. Adding neurons.
3. Connecting neurons through synapses.
4. Creating a simulation object.
5. Injecting external stimuli.
6. Running the simulation.

```cpp
// Create a network of 10 default LIF neurons
Rete rete(10);

// Connect neuron 0 -> neuron 1 with an excitatory synapse
Sinapsi s(0.5, 10 * n * A, 0, 1, 5 * ms);
rete.connettiNeuroni(s);

// Create a 500 ms simulation with dt = 0.1 ms
Simulazione sim(rete, 0.1 * ms, 500.0 * ms);

// Inject a constant current into neuron 0 for the first 100 ms
std::vector<int> ids = {0};
std::vector<parametriStimoloCostante> params = {{0.0, 100.0 * ms, 0.5 * n * A}};
sim.iniettaStimoloCostante(ids, params);

// Run and export
sim.avviaSimulazione("potenziali.bin", "firing.bin", "sinapsi.bin");
```

## External Stimuli

Stimuli are defined by parameter structs and injected into the simulation before running.

Available stimulus types:

- `parametriStimoloCostante` — constant current over a time window `[timeStart, timeEnd]`
- `parametriStimoloSeno` — sinusoidal current with amplitude, frequency, and phase over a time window

Stimuli are evaluated on-the-fly at each simulation step; no pre-allocated value arrays are needed.

## Installation

Clone the repository and compile with a C++17 compatible compiler:

```bash
git clone https://github.com/Tiziano02/HeaderRN.git
cd HeaderRN
make
```

## Roadmap

Future developments include:

- [ ] Transmission delays
- [ ] Synaptic plasticity
- [ ] Additional neuron models
- [ ] Relative refractory period
- [ ] Adjacency list for network topology
- [ ] Complete framework for data analysis
- [ ] Additional stimulus types (stochastic, arbitrary waveforms)

A complete roadmap is available in TODO.md.

## Documentation

- [Architecture](docs/ARCHITECTURE.md)
- [Roadmap](docs/TODO.md)

## License

Released under the GNU GPL v3.0 License.

See [LICENSE](LICENSE) for details.
