#ifndef NEURONE_HPP
#define NEURONE_HPP

#include "LIF.hpp"
#include "Exp.hpp"
#include <variant>
// #include "AdExp.hpp"
// #include "Izhikevich.hpp"

enum class NeuronModel { LIF, Exp };

using TypeConfig = std::variant<configLIF, configExp>;

using TypeNeuron = std::variant<LIF, Exp>;

#endif