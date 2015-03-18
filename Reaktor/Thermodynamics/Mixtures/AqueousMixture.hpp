// Reaktor is a C++ library for computational reaction modelling.
//
// Copyright (C) 2014 Allan Leal
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#pragma once

// C++ includes
#include <set>

// Reaktor includes
#include <Reaktor/Common/Index.hpp>
#include <Reaktor/Common/ChemicalScalar.hpp>
#include <Reaktor/Common/ChemicalVector.hpp>
#include <Reaktor/Thermodynamics/Species/AqueousSpecies.hpp>
#include <Reaktor/Thermodynamics/Mixtures/GeneralMixture.hpp>

namespace Reaktor {

/// Provide a computational representation of an aqueous mixture.
/// The AqueousMixture class is defined as a collection of AqueousSpecies objects,
/// representing, therefore, a mixture of aqueous species. Its main purpose is to
/// provide the necessary operations in the calculation of activities of aqueous
/// species. It implements methods for the calculation of molar fractions, molalities,
/// stoichiometric molalities, and effective and stoichiometric ionic strengths.
/// In addition, it provides methods that retrives information about the ionic, neutral and
/// complex species.
/// @see AqueousSpecies
/// @ingroup Mixtures
class AqueousMixture : public GeneralMixture<AqueousSpecies>
{
public:
    /// Construct a default AqueousMixture instance.
    AqueousMixture();

    /// Construct an AqueousMixture instance with given species.
    /// @param species The species that compose the aqueous mixture
    AqueousMixture(const std::vector<AqueousSpecies>& species);

    /// Destroy the AqueousMixture instance.
    virtual ~AqueousMixture();

    /// Return the number of neutral aqueous species in the aqueous mixture.
    auto numNeutralSpecies() const -> unsigned;

    /// Return the number of charged aqueous species in the aqueous mixture.
    auto numChargedSpecies() const -> unsigned;

    /// Return the indices of the neutral aqueous species in the aqueous mixture.
    auto indicesNeutralSpecies() const -> const Indices&;

    /// Return the indices of the charged aqueous species in the aqueous mixture.
    auto indicesChargedSpecies() const -> const Indices&;

    /// Return the indices of the cations in the aqueous mixture.
    auto indicesCations() const -> const Indices&;

    /// Return the indices of the anions in the aqueous mixture.
    auto indicesAnions() const -> const Indices&;

    /// Return the index of the water species @f$\ce{H2O(l)}@f$..
    auto indexWater() const -> Index;

    /// Return the local index of a neutral species among the neutral species in the aqueous mixture.
    /// @param name The name of the neutral species
    /// @return The local index of the neutral species if found. The number of neutral species otherwise.
    auto indexNeutralSpecies(const std::string& name) const -> Index;

    /// Return the local index of a charged species among the charged species in the aqueous mixture.
    /// @param name The name of the charged species
    /// @return The local index of the charged species if found. The number of charged species otherwise.
    auto indexChargedSpecies(const std::string& name) const -> Index;

    /// Return the local index of a cation among the cations in the aqueous mixture.
    /// @param name The name of the cation
    /// @return The local index of the cation if found. The number of cations otherwise.
    auto indexCation(const std::string& name) const -> Index;

    /// Return the local index of an anion among the anions in the aqueous mixture.
    /// @param name The name of the anion
    /// @return The local index of the anion if found. The number of anions otherwise.
    auto indexAnion(const std::string& name) const -> Index;

    /// Return the names of the neutral species in the aqueous mixture.
    auto namesNeutralSpecies() const -> std::vector<std::string>;

    /// Return the names of the charged species in the aqueous mixture.
    auto namesChargedSpecies() const -> std::vector<std::string>;

    /// Return the names of the cations in the aqueous mixture.
    auto namesCations() const -> std::vector<std::string>;

    /// Return the names of the cations in the aqueous mixture.
    auto namesAnions() const -> std::vector<std::string>;

    /// Return the dissociation matrix of the aqueous complexes into ions.
    /// This the matrix defines the stoichiometric relationship between the aqueous complexes and the
    /// ions produced from their dissociation. For example, the stoichiometry of the *j*-th ion in
    /// the dissociation reaction of the i*-th aqueous complex is given by the (*i*, *j*)-th entry in
    /// the matrix.
    auto dissociationMatrix() const -> const Matrix&;

    /// Calculate the molalities of the aqueous species and its molar derivatives.
    /// @param n The molar abundance of species (in units of mol)
    /// @return The molalities and their molar derivatives
    auto molalities(const Vector& n) const -> ChemicalVector;

    /// Calculate the stoichiometric molalities of the ions and its molar derivatives.
    /// @param m The molalities of the aqueous species and their molar derivatives
    /// @return The stoichiometric molalities and their molar derivatives
    auto stoichiometricMolalities(const ChemicalVector& m) const -> ChemicalVector;

    /// Calculate the effective ionic strength of the aqueous mixture and its molar derivatives.
    /// @param m The molalities of the aqueous species and their molar derivatives
    /// @return The effective ionic strength of the aqueous mixture and its molar derivatives
    auto effectiveIonicStrength(const ChemicalVector& m) const -> ChemicalScalar;

    /// Calculate the stoichiometric ionic strength of the aqueous mixture and its molar derivatives.
    /// @param ms The stoichiometric molalities of the ions and their molar derivatives
    /// @return The stoichiometric ionic strength of the aqueous mixture and its molar derivatives
    auto stoichiometricIonicStrength(const ChemicalVector& ms) const -> ChemicalScalar;

private:
    /// The index of the water species
    Index idx_water;

    /// The indices of the neutral aqueous species
    Indices idx_neutral_species;

    /// The indices of the charged aqueous species
    Indices idx_charged_species;

    /// The indices of the cations
    Indices idx_cations;

    /// The indices of the anions
    Indices idx_anions;

    /// The matrix that represents the dissociation of the aqueous complexes into ions
    Matrix dissociation_matrix;
};

} // namespace Reaktor
