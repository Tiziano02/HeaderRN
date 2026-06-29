#ifndef SINAPSI_HPP
#define SINAPSI_HPP

#include "CurrentSyn.hpp"
#include "ConductanceSyn.hpp"

#include <variant>

enum class Label_Type_Syn { Current, Conductance };

// Variant dei tipi di configurazione (passato dall'utente a connettiNeuroni)
using TypeConfigSyn = std::variant<configCurrentSyn, configConductanceSyn>;

// Variant dei tipi di sinapsi (contenuto interno di Rete)
using TypeSyn = std::variant<CurrentSyn, ConductanceSyn>;

#endif // SINAPSI_HPP
