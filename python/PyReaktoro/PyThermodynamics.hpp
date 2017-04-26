// Reaktoro is a unified framework for modeling chemically reactive systems.
//
// Copyright (C) 2014-2015 Allan Leal
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

// PyReaktoro includes
#include <PyReaktoro/Thermodynamics/PyCore.hpp>
#include <PyReaktoro/Thermodynamics/PyModels.hpp>
#include <PyReaktoro/Thermodynamics/PyPhases.hpp>
#include <PyReaktoro/Thermodynamics/PySpecies.hpp>

namespace Reaktoro {

inline auto export_Thermodynamics() -> void
{
    export_ThermodynamicsCore();
    export_ThermodynamicsModels();
    export_ThermodynamicsPhases();
    export_ThermodynamicsSpecies();
}

} // namespace Reaktoro