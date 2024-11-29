# LArSoftSimulationTools
LArSoft product to handle data processing of PD-HD data from hdf5 format to reconstructed art::ROOT files.

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
VERSION=v10_01_03d00
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
mrb g https://github.com/ProtoDUNE-BSM/LArSoftSimulationTools.git

# You may want to also include the trigger simulation software
mrb g https://github.com/wesketchum/dunetrigger.git@develop

cd ${MRB_BUILDDIR}

mrbsetenv

mrb i -j4
mrbslp
```

Once the development area has been created you can set up the environment withe following script:

```
VERSION=v10_01_03d00
QUALS=e26:prof

source /cvmfs/dune.opensciencegrid.org/products/dune/setup_dune.sh
setup dunesw ${VERSION} -q ${QUALS}
echo $DUNESW_DIR

source localProducts_*/setup
mrbslp
```
