# LArSoftSimulationTools
LArSoft product for simulating events from generation through to reconstruction.

## Apptainer Container

You will need an SL7 container to run LArSoft. This can be done on gpvm and lxplus.

Run the following command when on gpvm:
```
/cvmfs/oasis.opensciencegrid.org/mis/apptainer/current/bin/apptainer shell --shell=/bin/bash -B /cvmfs,/exp,/nashome,/pnfs/dune,/opt,/run/user,/etc/hostname,/etc/hosts,/etc/krb5.conf --ipc --pid /cvmfs/singularity.opensciencegrid.org/fermilab/fnal-dev-sl7:latest
```
And this command when on lxplus (they are the same command, just different directories need to be made accessible:
```
/cvmfs/oasis.opensciencegrid.org/mis/apptainer/current/bin/apptainer shell --shell=/bin/bash -B /cvmfs,/eos,/afs/cern.ch/work,/opt,/run/user,/etc/hostname,/etc/hosts,/etc/krb5.conf --ipc --pid /cvmfs/singularity.opensciencegrid.org/fermilab/fnal-dev-sl7:latest
```

## Setting up Code and Development Area

You need to set up LArSoft, dunesw, pull the code from GitHub and create your development area. Follow this script:

```
VERSION=v10_01_04d00
QUALS=e26:prof
DIRECTORY=protodunedm_mc_simulation
export WORKDIR=/exp/dune/app/users/$USER/ # or on lxplus /afs/cern.ch/work/c/${USER}/public/

source /cvmfs/dune.opensciencegrid.org/products/dune/setup_dune.sh
export UPS_OVERRIDE="-H Linux64bit+3.10-2.17"
setup dunesw -v ${VERSION} -q ${QUALS}

cd ${WORKDIR}
mkdir -p ${DIRECTORY}
cd ${DIRECTORY}

# Create a new development area using mrb
mrb newDev -v ${VERSION} -q ${QUALS}
source ${WORKDIR}/${DIRECTORY}/localProducts*/setup

# Clone LArSoftDataTools from ProtoDUNE-BSM GitHub
mrb g https://github.com/ProtoDUNE-BSM/pdhdbsmsimulation.git

# You will want to also include the trigger emulation software
mrb g https://github.com/wesketchum/dunetrigger.git@develop

cd ${MRB_BUILDDIR}

mrbsetenv

mrb i -j4
mrbslp
```

Once the development area has been created you can set up the environment withe following script:

```
VERSION=v10_01_04d00
QUALS=e26:prof

source /cvmfs/dune.opensciencegrid.org/products/dune/setup_dune.sh
setup dunesw ${VERSION} -q ${QUALS}
echo $DUNESW_DIR

source localProducts_*/setup
mrbslp
```

## Simulation Workflow

The simulation follows the typical LArSoft simulation workflow. Custom configuration files are used for generate, reconstruction and the 1st stage of the detector simulation. The process is:
```
Generator -> GEANT4 -> Detsim -> Reconstruction
```
This workflow is produce a neutrino interaction with cosmic and radiological backgrounds. The fcl file `prod_beamneutrino_cosmic_protodunehd.fcl` reads in a neutrino flux file and generates a neutrino, cosmics and radioactivity. The following commands can be run online to generate a sample of neutrino events in PD-HD and process them to reconstruction.

```
#!/bin/bash

NEVENTS=10

lar -c srcs/pdhdbsmsimulation/example/prod_beamneutrino_cosmic_protodunehd.fcl -n ${NEVENTS} -o nugen.root

lar -c standard_g4_protodunehd_stage1.fcl -n ${NEVENTS} -o nug41.root -s nugen.root

lar -c standard_g4_protodunehd_stage2.fcl -n ${NEVENTS} -o nug42.root -s nug41.root

lar -c srcs/pdhdbsmsimulation/example/standard_detsim_protodunehd_stage1_edit.fcl -n ${NEVENTS} -o nudetsim1.root -s nug42.root

lar -c standard_detsim_protodunehd_stage2.fcl -n ${NEVENTS} -o nudetsim2.root -s nudetsim1.root

lar -c srcs/pdhdbsmsimulation/example/example/runPandoraNeutrino.fcl -n ${NEVENTS} -o nureco.root -s nudetsim2.root
```
The custom detsim fcl file `standard_detsim_protodunehd_stage1_edit.fcl` is needed due to a bug in the v10 dunesw.

# Neutrino Flux Files.

The neutrino generator requires fluxes. These are provided in the form of GSimpleNtpFlux files. You can find some documentation [here](https://cdcvs.fnal.gov/redmine/projects/nutools/wiki/GENIEHelper_Flux). One issue with this flux file format is: "It is important that users not mix gsimple files that were created with different flux windows and/or different POTs/file; doing so may lead to incorrect overall POT accounting when generating GENIE events."

Many of the gsimple files currently do have different POTs/file. Therefore, a separate gsimple exists for each wobbling configuration, neutrino flavour and hadron decay mode combination. A configuration file system has been set up that allows the user to choose the flux file they want. in `runPandoraNeutrinoMC.fcl` you can find the parameters:
```
physics.producers.generator.FluxFiles: [@local::wnp04.nue.decaymode[4]]
physics.producers.generator.GenFlavors: [@local::wnp04.nue.pdg]
```
So the user can change the wobbing configuration `wnp04, w133 or w000`, the neutrino flavour `numu, nue, numubar and nuebar` and decay mode `0, 1, 2, 3 or 4` (for numu/nue, 0-2 for numubar/nuebar). It is possible to load all the gsimple files at once, but the GENIE POT counting might be wrong.

There are bash scripts included in the `test` directory that shows how a bash script could be used to run all the different flux configurations. The bash scripts use `sed` to edit the parameters `physics.producers.generator.FluxFiles` and `physics.producers.generator.GenFlavors`. The script `OnlineGenJob.sh` should loop though the different flux configurations and generate MC samples for each.

# Running the Simulation on FermiGrid

To generate large samples of neutrinos the grid is needed. So far, scripts have been written to run neutrino production jobs from generation through to reconstruction on FermiGrid. For further details on running jobs on FermiGrid please consult this [tutorial](https://dune.github.io/computing-basics/07-grid-job-submission/index.html). First, login to your favourite gpvm node and make sure you have your grid certificate and proxy.
```
kinit -f <USER>
kx509
voms-proxy-init --noregen -rfc -voms dune:/dune/Role=Analysis
```
To run on the grid you will need to tar up your local environment and send it to the grid node along with the execution script. One trick is that the local products setup script needs to be altered to effectively setup the work envirnment on the grid node. Run `cp localProducts_*/setup localProducts_*/setup_grid` - this gives you a new setup script that will be called by the job script. Chane the following environment variables in `localProducts_*/setup_grid` so they look like this:
```
setenv MRB_PROJECT "larsoft"
setenv MRB_PROJECT_VERSION "v10_01_04"
setenv MRB_QUALS "e26:prof"
setenv MRB_TOP "${INPUT_TAR_DIR_LOCAL}/protodunedm"
setenv MRB_TOP_BUILD "${INPUT_TAR_DIR_LOCAL}/protodunedm"
setenv MRB_SOURCE "${INPUT_TAR_DIR_LOCAL}/protodunedm/srcs"
setenv MRB_INSTALL "${INPUT_TAR_DIR_LOCAL}/protodunedm/localProducts_larsoft_v10_01_04d00_e26_prof"
setenv PRODUCTS "${MRB_INSTALL}:${PRODUCTS}"
setenv CETPKG_INSTALL "${INPUT_TAR_DIR_LOCAL}/protodunedm_mc_simulation/localProducts_larsoft_v10_01_04d00_e26_prof"
```
Once this is edited you can run the script `tarball_protodunedm.sh` located in the `test` directory. This gives you a tarball of your work environment that is passed to the grid job. Note that if you change your code you will need to re-create this tarball before giving it the grid job.

Next you can simply run `jobsub_nucosradsim.sh` as a bash executable. In order to alter the type and number of jobs being submitted you first need to edit this script. There are some comments in the script that hopefully explain what needs to change. In the future these parameter changes might be done via command line arguments. It is recommended that the number of events per job is kept to 1 for now in order to stop the RAM usage blowing up. Simulating neutrinos along with the cosmic overlay is quite resource-intensive.

The job submission script `jobsub_nucosradsim.sh` submits the script `run_fermigridprod_neutrino_cosoverlay_reco.sh` with our corresponding tarball to the grid. Open `run_fermigridprod_neutrino_cosoverlay_reco.sh` to see what is being run. You should be able to see each step of the sim/reco of neutrinos being executed. 
