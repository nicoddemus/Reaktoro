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

#include "NonlinearSolver.hpp"

// Eigen includes
#include <Reaktoro/Eigen/LU>
#include <Reaktoro/Eigen/SVD>

// Reaktoro includes
#include <Reaktoro/Common/Exception.hpp>
#include <Reaktoro/Common/Outputter.hpp>
#include <Reaktoro/Common/TimeUtils.hpp>
#include <Reaktoro/Math/MathUtils.hpp>
#include <Reaktoro/Optimization/Utils.hpp>

namespace Reaktoro {

auto NonlinearOutput::operator=(bool activate) -> NonlinearOutput&
{
    OutputterOptions::operator=(activate);
    return *this;
}

struct NonlinearSolver::Impl
{
    /// The residual of the non-linear function.
    NonlinearResidual residual;

    /// The Newton step for the unknowns `x`
    Vector dx;

    /// The trial iterate `x`
    Vector xtrial;

    /// The outputter instance
    Outputter outputter;

    /// Solve the optimization problem.
    auto solve(const NonlinearProblem& problem, Vector& x, const NonlinearOptions& options) -> NonlinearResult
    {
        // Start timing the calculation
        Time begin = time();

        // Initialize the outputter instance
        outputter = Outputter();
        outputter.setOptions(options.output);

        // The result of the calculation
        NonlinearResult result;

        // Auxiliary references to problem data
        const auto& n = problem.n;
        const auto& m = problem.m;
        const auto& A = problem.A;
        const auto& b = problem.b;

        // Auxiliary references to residual value and jacobian
        auto& F = residual.val;
        auto& J = residual.jacobian;

        // Define auxiliary references to general options
        const auto tol = options.tolerance;
        const auto tolx = options.tolerancex;
        const auto maxiters = options.max_iterations;
        const auto tau = options.tau;

        // Define some auxiliary references to result variables
        auto& error = result.error;
        auto& iterations = result.iterations;
        auto& succeeded = result.succeeded = false;

        // Ensure the initial guess for `x` has adequate dimension
        if(Index(x.size()) != n) x = zeros(n);

        // The function that outputs the header and initial state of the solution
        auto output_initial_state = [&]()
        {
            if(!options.output.active) return;

            outputter.addEntry("Iteration");
            outputter.addEntries(options.output.xprefix, n, options.output.xnames);
            outputter.addEntries(options.output.fprefix, m, options.output.fnames);
            outputter.addEntry("Error");

            outputter.outputHeader();
            outputter.addValue(iterations);
            outputter.addValues(x);
            outputter.addValues(residual.val);
            outputter.addValue(error);
            outputter.outputState();
        };

        // The function that outputs the current state of the solution
        auto output_state = [&]()
        {
            if(!options.output.active) return;

            outputter.addValue(iterations);
            outputter.addValues(x);
            outputter.addValues(residual.val);
            outputter.addValue(error);
            outputter.outputState();
        };

        // Return true if the result of a calculation failed
        auto failed = [&](bool succeeded)
        {
            return !succeeded;
        };

        // The function that initialize the state of some variables
        auto initialize = [&]()
        {
            // Initialize xtrial
            xtrial.resize(n);

            // Evaluate the non-linear function
            residual = problem.f(x);

            // Update the residuals of the calculation
            error = max(abs(F));
        };

        // The function that computes the Newton step
        auto compute_newton_step = [&]()
        {
            // Check if the last residual calculation succeeded
            if(!residual.succeeded)
                return false;

            // Compute the Newton step `dx`
            if(n == m)
                dx = -J.lu().solve(F);
            else
                dx = -J.jacobiSvd(Eigen::ComputeThinU | Eigen::ComputeThinV).solve(F);

            // Return true if the calculation succeeded
            return dx.allFinite();
        };

        // The function that performs an update in the iterates
        auto update_iterates = [&]()
        {
            // Initialize the step length factor for Newton step dx with the largest possible value
            double alphax = fractionToTheBoundary(x, dx, A, b, tau);

            // Initialize the step length factor
            double alpha = 1.0;

            // The number of tentatives to find a trial iterate that results in finite objective result
            unsigned tentatives = 0;

            // Repeat until a suitable xtrial iterate if found such that f(xtrial) is finite
            for(; tentatives < 6; ++tentatives)
            {
                // Calculate the current trial iterate for x
                xtrial = x + alpha*alphax*dx;

                // Evaluate the objective function at the trial iterate
                residual = problem.f(xtrial);

                // Leave the loop if evaluation of f(xtrial) has succeeded
                if(residual.succeeded)
                    break;

                // Decrease alpha in a hope that a shorter step results in f(xtrial) succeeded
                alpha *= 0.1;
            }

            // Return false if xtrial could not be found s.t. f(xtrial) succeeds
            if(tentatives == 6)
                return false;

            // Update the iterate x from xtrial
            x = xtrial;

            // Update the residuals of the calculation
            error = max(abs(F));

            // Return true as found xtrial results in finite f(xtrial)
            return true;
        };

        auto converged = [&]()
        {
            // Check if the calculation should stop based on max variation of x
            if(tolx && max(abs(dx)) < tolx)
                return true;

            // Check if the calculation should stop based on residual tolerance
            return error < tol;
        };

        initialize();
        output_initial_state();

        for(iterations = 1; iterations <= maxiters && !succeeded; ++iterations)
        {
            if(failed(compute_newton_step()))
                break;
            if(failed(update_iterates()))
                break;
            output_state();
            succeeded = converged();
        }

        // Output a final header
        outputter.outputHeader();

        // Finish timing the calculation
        result.time = elapsed(begin);

        return result;
    }
};

NonlinearSolver::NonlinearSolver()
: pimpl(new Impl())
{}

NonlinearSolver::NonlinearSolver(const NonlinearSolver& other)
: pimpl(new Impl(*other.pimpl))
{}

NonlinearSolver::~NonlinearSolver()
{}

auto NonlinearSolver::operator=(NonlinearSolver other) -> NonlinearSolver&
{
    pimpl = std::move(other.pimpl);
    return *this;
}

auto NonlinearSolver::solve(const NonlinearProblem& problem, Vector& x) -> NonlinearResult
{
    return pimpl->solve(problem, x, {});
}

auto NonlinearSolver::solve(const NonlinearProblem& problem, Vector& x, const NonlinearOptions& options) -> NonlinearResult
{
    return pimpl->solve(problem, x, options);
}

} // namespace Reaktoro