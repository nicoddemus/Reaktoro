print '============================================================'
print 'Make sure you have the following Python packages installed: '
print '     numpy, matplotlib, joblib'
print 'These can be installed with pip:'
print '     pip install numpy matplotlib joblib'
print '============================================================'

from reaktoro import *
from numpy import *
import matplotlib.pyplot as plt
from joblib import Parallel, delayed
import os, sys

# Auxiliary time related constants
second = 1
minute = 60
hour = 60 * minute
day = 24 * hour
year = 365 * day

# Parameters for the reactive transport simulation
nsteps = 100      # the number of steps in the reactive transport simulation
ncells = 100      # the number of cells in the discretization
D  = 1.0e-9       # the diffusion coefficinet (in units of m2/s)
v  = 1.0/day      # the fluid pore velocity (in units of m/s)
dt = 10*minute    # the time step (in units of s)
T = 60.0 + 273.15 # the temperature (in units of K)
P = 100 * 1e5     # the pressure (in units of Pa)

# The list of quantities to be output for each mesh cell, each time step
output_quantities = """
    pH
    speciesMolality(H+)
    speciesMolality(Ca++)
    speciesMolality(Mg++)
    speciesMolality(HCO3-)
    speciesMolality(CO2(aq))
    phaseVolume(Calcite)
    phaseVolume(Dolomite)
""".split()


# Perform the reactive transport simulation
def simulate():
    # Construct the chemical system with its phases and species
    editor = ChemicalEditor()
    editor.addAqueousPhase(['H2O(l)', 'H+', 'OH-', 'Na+', 'Cl-', 'Ca++', 'Mg++', 'HCO3-', 'CO2(aq)', 'CO3--']) # Aqueous species individually selected for performance reasons
    editor.addMineralPhase('Quartz')
    editor.addMineralPhase('Calcite')
    editor.addMineralPhase('Dolomite')

    # Create the ChemicalSystem object using the configured editor
    system = ChemicalSystem(editor)

    # Define the inicial condition of the reactive transport modeling problem
    problem_ic = EquilibriumProblem(system)
    problem_ic.add('H2O', 1.0, 'kg')
    problem_ic.add('NaCl', 0.7, 'mol')
    problem_ic.add('CaCO3', 10, 'mol')
    problem_ic.add('SiO2', 10, 'mol')

    # Define the boundary condition of the reactive transport modeling problem
    problem_bc = EquilibriumProblem(system)
    problem_bc.add('H2O', 1.0, 'kg')
    problem_bc.add('NaCl', 0.90, 'mol')
    problem_bc.add('MgCl2', 0.05, 'mol')
    problem_bc.add('CaCl2', 0.01, 'mol')
    problem_bc.add('CO2', 0.75, 'mol')

    # Calculate the equilibrium states for the initial and boundary conditions
    state_ic = equilibrate(problem_ic)
    state_bc = equilibrate(problem_bc)

    # Scale the phases in the initial condition as required
    state_ic.scalePhaseVolume('Aqueous', 0.1, 'm3')
    state_ic.scalePhaseVolume('Quartz', 0.88, 'm3')
    state_ic.scalePhaseVolume('Calcite', 0.02, 'm3')

    # Scale the boundary condition state to 1 m3
    state_bc.scaleVolume(1.0)

    # The indices of the fluid and solid species
    ifluid_species = system.indicesFluidSpecies()
    isolid_species = system.indicesSolidSpecies()

    # The concentrations of each element in each mesh cell
    b = zeros((ncells, system.numElements()))

    # The concentrations of each element in the fluid partition, in each mesh cell
    bfluid = zeros((ncells, system.numElements()))

    # The concentrations of each element in the solid partition, in each mesh cell
    bsolid = zeros((ncells, system.numElements()))

    # The concentrations of each element in each mesh cell in the previous time step
    bprev = zeros((ncells, system.numElements()))

    # Initialize the concentrations of the elements in each mesh cell
    b[:] = state_ic.elementAmounts().array()

    # Initialize the concentrations of each element on the boundary
    b_bc = state_bc.elementAmounts().array()

    # The list of chemical states, one for each cell, initialized to initial state
    states = [state_ic.clone() for _ in xrange(ncells)]

    # The interval [0, 1] split into ncells
    x = linspace(0.0, 1.0, ncells + 1)

    # The length of each mesh cell (in units of m)
    dx = 1.0/ncells

    # Create the equilibrium solver object for repeated equilibrium calculation
    solver = EquilibriumSolver(system)

    # The current step number
    step = 0

    # The current time (in seconds)
    t = 0.0

    # The auxiliary function to create an output file each time step
    def outputstate():
        ndigits = len(str(nsteps))
        output = ChemicalOutput(system)
        output.filename('results/{}.txt'.format(str(step).zfill(ndigits)))
        output.add('tag', 'x') # The value of the center coordinates of the cells
        for quantity in output_quantities:
            output.add(quantity)
        output.open()
        for state, tag in zip(states, x):
            output.update(state, tag)
        output.close()

    # Start the reactive transport simulation loop
    while step <= nsteps:
        # Print the progress of the simulation
        print "Progress: {}/{} steps, {} min".format(step, nsteps, t/minute)

        # Ouput the current state of the reactive transport calculation
        outputstate()

        # Collect the amounts of elements from fluid and solid partitions
        for icell in xrange(ncells):
            bfluid[icell] = states[icell].elementAmountsInSpecies(ifluid_species).array()
            bsolid[icell] = states[icell].elementAmountsInSpecies(isolid_species).array()

        # Transport each element in the fluid phase
        for j in xrange(system.numElements()):
            transport(bfluid[:, j], dt, dx, v, D, b_bc[j])

        # Update the amounts of elements in both fluid and solid partitions
        b[:] = bsolid + bfluid

        # Equilibrate all cells with the updated element amounts
        for icell in xrange(ncells):
            solver.solve(states[icell], T, P, b[icell])

        # Update the amounts of elements at previous time step
        bprev[:] = b

        # Increment time step and number of time steps
        t += dt
        step += 1

    print "Finished!"


# Return a string for the title of a figure in the format Time: #h##m
def titlestr(t):
    t = t / minute   # Convert from seconds to minutes
    h = int(t) / 60  # The number of hours
    m = int(t) % 60  # The number of remaining minutes
    return 'Time: {:>3}h{:>2}m'.format(h, str(m).zfill(2))


# Generate figures for a result file
def plotfile(file):
    step = int(file.split('.')[0])

    print 'Plotting figure', step, '...'

    t = step * dt
    filearray = loadtxt('results/' + file, skiprows=1)
    data = filearray.T

    ndigits = len(str(nsteps))

    plt.figure()
    plt.xlim(xmin=-0.02, xmax=0.52)
    plt.ylim(ymin=2.5, ymax=10.5)
    plt.title(titlestr(t))
    plt.xlabel('Distance [m]')
    plt.ylabel('pH')
    plt.plot(data[0], data[1])
    plt.tight_layout()
    plt.savefig('figures/ph/{}.png'.format(str(step).zfill(ndigits)))

    plt.figure()
    plt.xlim(xmin=-0.02, xmax=0.52)
    plt.ylim(ymin=-0.1, ymax=2.1)
    plt.ticklabel_format(style='sci', axis='y', scilimits=(0,0))
    plt.title(titlestr(t))
    plt.xlabel('Distance [m]')
    plt.ylabel('Mineral Volume [%$_{\mathsf{vol}}$]')
    plt.plot(data[0], data[7] * 100, label='Calcite')
    plt.plot(data[0], data[8] * 100, label='Dolomite')
    plt.legend(loc='center right')
    plt.tight_layout()
    plt.savefig('figures/calcite-dolomite/{}.png'.format(str(step).zfill(ndigits)))

    plt.figure()
    plt.yscale('log')
    plt.xlim(xmin=-0.02, xmax=0.52)
    plt.ylim(ymin=0.5e-5, ymax=2)
    plt.title(titlestr(t))
    plt.xlabel('Distance [m]')
    plt.ylabel('Concentration [molal]')
    plt.plot(data[0], data[3], label='Ca++')
    plt.plot(data[0], data[4], label='Mg++')
    plt.plot(data[0], data[5], label='HCO3-')
    plt.plot(data[0], data[6], label='CO2(aq)')
    plt.plot(data[0], data[2], label='H+')
    plt.legend(loc='lower right')
    plt.tight_layout()
    plt.savefig('figures/aqueous-species/{}.png'.format(str(step).zfill(ndigits)))

    plt.close('all')


# Plot all result files and generate a video
def plot():
    # Plot all result files
    files = sorted(os.listdir('results'))
    Parallel(n_jobs=16)(delayed(plotfile)(file) for file in files)

    # Create videos for the figures
    ffmpegstr = 'ffmpeg -y -r 30 -i figures/{0}/%03d.png -codec:v mpeg4 -flags:v +qscale -global_quality:v 0 videos/{0}.mp4'
    os.system(ffmpegstr.format('calcite-dolomite'))
    os.system(ffmpegstr.format('aqueous-species'))
    os.system(ffmpegstr.format('ph'))


# Define the main function
def main():
    os.system('mkdir -p results')
    os.system('mkdir -p figures/ph')
    os.system('mkdir -p figures/aqueous-species')
    os.system('mkdir -p figures/calcite-dolomite')
    os.system('mkdir -p videos')
    simulate()
    plot()


# Solve a tridiagonal matrix equation using Thomas algorithm.
def thomas(a, b, c, d):
    n = len(d)
    c[0] /= b[0]
    for i in xrange(1, n - 1):
        c[i] /= b[i] - a[i]*c[i - 1]
    d[0] /= b[0]
    for i in xrange(1, n):
        d[i] = (d[i] - a[i]*d[i - 1])/(b[i] - a[i]*c[i - 1])
    x = d
    for i in reversed(xrange(0, n - 1)):
        x[i] -= c[i]*x[i + 1]
    return x


# Perform a transport step
def transport(u, dt, dx, v, D, g):
    n = len(u)
    alpha = D*dt/dx**2
    beta = v*dt/dx
    a = full(n, -beta - alpha)
    b = full(n, 1 + beta + 2*alpha)
    c = full(n, -alpha)
    # Uncomment for Dirichlet boundary conditions
    # b[0] = 1.0
    # c[0] = 0.0
    # u[0] = g
    #---------------------------------------------
    # Uncomment for flux boundary conditions
    u[0] += beta*g
    b[0] = 1 + beta + alpha
    b[-1] = 1 + beta + alpha
    #---------------------------------------------
    thomas(a, b, c, u)


if __name__ == '__main__':
    main()
