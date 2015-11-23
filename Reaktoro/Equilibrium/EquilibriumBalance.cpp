// Reaktoro is a C++ library for computational reaction modelling.
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

#include "EquilibriumBalance.hpp"

// Eigen includes
#include <Reaktoro/Eigen/Dense>

// Reaktoro includes
#include <Reaktoro/Core/ChemicalSystem.hpp>
#include <Reaktoro/Core/Partition.hpp>
#include <Reaktoro/Equilibrium/EquilibriumReactions.hpp>

namespace Reaktoro {

struct EquilibriumBalance::Impl
{
    /// The system of equilibrium reactions used to regularize the balance matrix
    EquilibriumReactions reactions;

    /// The regularizer matrix
    Matrix R;

    /// The regularized balance matrix
    Matrix A;

    /// Construct an Impl instance with given reactions
    Impl(const EquilibriumReactions& reactions)
    : reactions(reactions)
    {
        // Initialize the formula matrix of the equilibrium species
        A = reactions.partition().formulaMatrixEquilibriumSpecies();

        // Get the lu decomposition of the formula matrix of equilibrium species
        const auto& lu = reactions.lu();
        
        // Permute the rows of A
        A = lu.P * A;

        // Remove the rows of A past rank
        A.conservativeResize(lu.rank, Eigen::NoChange);

        // Create a reference to the U1 part of U = [U1 U2]
        const auto U1 = lu.U.leftCols(lu.rank).triangularView<Eigen::Upper>();
        
        // Compute the regularizer matrix R
        R = lu.L.triangularView<Eigen::Lower>().solve(identity(lu.rank, lu.rank));
        R = U1.solve(R);

        // Compute the regularized balance matrix
        A = R * A;

        // Clean matrices A and R from round-off errors
        cleanRationalNumbers(A);
        cleanRationalNumbers(R);
    }

    /// Return the regularized right hand side vector b
    auto regularizedVector(Vector b) const -> Vector
    {
        // Get the lu decomposition of the formula matrix of equilibrium species
        const auto& lu = reactions.lu();

        // Permute the rows of b
        b = lu.P * b;

        // Remove the rows of b past rank
        b.conservativeResize(lu.rank);

        // Compute the regularized b
        b = R * b;

        return b;
    }
};

EquilibriumBalance::EquilibriumBalance(const ChemicalSystem& system)
: EquilibriumBalance(system, Partition(system))
{
}

EquilibriumBalance::EquilibriumBalance(const ChemicalSystem& system, const Partition& partition)
: EquilibriumBalance(EquilibriumReactions(system, partition))
{
}

EquilibriumBalance::EquilibriumBalance(const EquilibriumReactions& reactions)
: pimpl(new Impl(reactions))
{
}

EquilibriumBalance::EquilibriumBalance(const EquilibriumBalance& other)
: pimpl(new Impl(*other.pimpl))
{
}

EquilibriumBalance::~EquilibriumBalance()
{
}

auto EquilibriumBalance::operator=(EquilibriumBalance other) -> EquilibriumBalance&
{
    pimpl = std::move(other.pimpl);
    return *this;
}

auto EquilibriumBalance::regularizedMatrix() const -> const Matrix&
{
    return pimpl->A;
}

auto EquilibriumBalance::regularizedVector(Vector b) const -> Vector
{
    return pimpl->regularizedVector(b);
}

} // namespace Reaktoro