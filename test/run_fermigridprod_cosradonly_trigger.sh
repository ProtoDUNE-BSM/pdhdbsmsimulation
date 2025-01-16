#!/bin/bash

LOGYLOG () {
  echo ${1}
}

# Input parameters
FCL_FILE="${1}"
NEvents="${2}"
SkipEvents="${2}"

echo "$FCL_FILE, $NEVENTS and $NSKIP"

WOBDIR=${WOBCONF}

# dCache directories for input/ouput
SCRATCHDIR=/pnfs/dune/scratch/users/chasnip/ProtoDUNEBSM/CosmicSimTest
XROOTDSCRATCHDIR=root://fndca1.fnal.gov:1094/pnfs/fnal.gov/usr/dune/scratch/users/chasnip/ProtoDUNEBSM/CosmicSimTest

# Names of ouput files for different stages
GENOUTFILENAME="cosmic_gen_${SkipEvents}_${NEvents}_${CLUSTER}_${PROCESS}.root"
G41OUTFILENAME="cosmic_g41gen_${SkipEvents}_${NEvents}_${CLUSTER}_${PROCESS}.root"
G42OUTFILENAME="cosmic_g42gen_${SkipEvents}_${NEvents}_${CLUSTER}_${PROCESS}.root"
DETSIM1OUTFILENAME="cosmic_detsim1g4gen_${SkipEvents}_${NEvents}_${CLUSTER}_${PROCESS}.root"
DETSIM2OUTFILENAME="cosmic_detsim2g4gen_${SkipEvents}_${NEvents}_${CLUSTER}_${PROCESS}.root"
TRIGOUTFILENAME="cosmic_triggereddetsim2g4gen_${SkipEvents}_${NEvents}_${CLUSTER}_${PROCESS}.root"
RECOOUTFILENAME="cosmic_recotriggereddetsim2g4gen_${SkipEvents}_${NEvents}_${CLUSTER}_${PROCESS}.root"

# Check grid node directories
LOGYLOG "We have tarball ${INPUT_TAR_DIR_LOCAL}."
ls ${INPUT_TAR_DIR_LOCAL}
LOGYLOG "Condor DIR = $_CONDOR_SCRATCH_DIR"
ls $_CONDOR_SCRATCH_DIR
LOGYLOG "_CONDOR_JOB_IWD = ${_CONDOR_JOB_IWD}"
ls ${_CONDOR_JOB_IWD}

if [ $? -ne 0 ]; then
  LOGYLOG "cannot read local products."
  exit 10
fi

# Source the environment to do job
source /cvmfs/dune.opensciencegrid.org/products/dune/setup_dune.sh
export DUNESW_VERSION=v10_01_04d00
export QUAL="e26:prof"
setup dunesw $DUNESW_VERSION -q ${QUAL}
# set local prods
source ${INPUT_TAR_DIR_LOCAL}/protodunedm/localProducts_larsoft_*/setup_grid
echo "mrbslp"
mrbslp

# Check the local products setup
LOGYLOG "ls MRB_TOP:"
ls $MRB_TOP
LOGYLOG "ls MRB_SOURCE:"
ls $MRB_SOURCE
LOGYLOG "ls MRB_BUILDDIR:"
ls $MRB_BUILDDIR
LOGYLOG "ls MRB_INSTALL:"
ls $MRB_INSTALL
LOGYLOG "ls CETPKG_INSTALL:"
ls $CETPKG_INSTALL

# This path should contain your local products
LOGYLOG ">>> echo CET_PLUGIN_PATH"
echo $CET_PLUGIN_PATH

# Needed to edit electronics gain to be 7.8 instead of 14 mV/fC
LOGYLOG "Update WIRECELL_PATH to include local configs"
#export WIRECELL_PATH=`pwd`/srcs/pdhdbsmsimulation/wire-cell-cfg:$WIRECELL_PATH
export WIRECELL_PATH=$MRB_SOURCE/pdhdbsmsimulation/wire-cell-cfg:$WIRECELL_PATH
echo ${WIRECELL_PATH}

##==========================
## Do Production ===========
##==========================

# Generate neutrino + cosmics + radiological backgrounds
lar -c ${INPUT_TAR_DIR_LOCAL}/protodunedm/srcs/pdhdbsmsimulation/example/$FCL_FILE -n ${NEvents} -o "${GENOUTFILENAME}"
# Check output was successfully produced
if [ ! -e ${GENOUTFILENAME} ]; then
  LOGYLOG "[ERROR]: Failed to produce expected gen file."
  exit 1
fi

# Do G4 simulation
#lar -c standard_g4_protodunehd_stage1.fcl -n ${NEvents} -o ${G41OUTFILENAME} -s ${GENOUTFILENAME}
lar -c ${INPUT_TAR_DIR_LOCAL}/protodunedm/srcs/pdhdbsmsimulation/example/standard_g4_protodunehd_edit.fcl -n ${NEvents} -o ${G41OUTFILENAME} -s ${GENOUTFILENAME}
rm ${GENOUTFILENAME}
# Check output was successfully produced
if [ ! -e ${G41OUTFILENAME} ]; then
  LOGYLOG "[ERROR]: Failed to produce expected g4 file."
  exit 1
fi

#lar -c standard_g4_protodunehd_stage2.fcl -n ${NEvents} -o ${G42OUTFILENAME} -s ${G41OUTFILENAME}
#rm ${G41OUTFILENAME}
# Check output was successfully produced
#if [ ! -e ${G42OUTFILENAME} ]; then
#  LOGYLOG "[ERROR]: Failed to produce expected g4 file."
#  exit 1
#fi

# Do detsim stage 1 simulation
#lar -c ${INPUT_TAR_DIR_LOCAL}/protodunedm/srcs/pdhdbsmsimulation/example/standard_detsim_protodunehd_stage1_edit.fcl -n ${NEvents} -o ${DETSIM1OUTFILENAME} -s ${G42OUTFILENAME}
lar -c ${INPUT_TAR_DIR_LOCAL}/protodunedm/srcs/pdhdbsmsimulation/example/standard_detsim_protodunehd_stage1_edit.fcl -n ${NEvents} -o ${DETSIM1OUTFILENAME} -s ${G41OUTFILENAME}
#rm ${G42OUTFILENAME}
rm ${G41OUTFILENAME}
# Check output was successfully produced
if [ ! -e ${DETSIM1OUTFILENAME} ]; then
  LOGYLOG "[ERROR]: Failed to produce expected detsim file."
  exit 1
fi

# Do detsim stage 2 simulation
#lar -c standard_detsim_protodunehd_stage2.fcl -n ${NEvents} -o ${DETSIM2OUTFILENAME} -s ${DETSIM1OUTFILENAME}
#rm ${DETSIM1OUTFILENAME}
# Check output was successfully produced
#if [ ! -e ${DETSIM2OUTFILENAME} ]; then
#  LOGYLOG "[ERROR]: Failed to produce expected detsim file."
#  exit 1
#fi

# Do trigger simulation
#lar -c ${INPUT_TAR_DIR_LOCAL}/protodunedm/srcs/pdhdbsmsimulation/example/run_tpalg_taalg_tcalg.fcl -n ${NEvents} -o ${TRIGOUTFILENAME} -s ${DETSIM2OUTFILENAME}
lar -c ${INPUT_TAR_DIR_LOCAL}/protodunedm/srcs/pdhdbsmsimulation/example/run_tpalg_taalg_tcalg.fcl -n ${NEvents} -o ${TRIGOUTFILENAME} -s ${DETSIM1OUTFILENAME}
#rm ${DETSIM2OUTFILENAME}
rm ${DETSIM1OUTFILENAME}
# Check output was successfully produced
if [ ! -e ${TRIGOUTFILENAME} ]; then
  LOGYLOG "[ERROR]: Failed to produce expected triggered file."
  exit 1
fi
# Copy reco file to output area
LOGYLOG "ifdh cp -D ${TRIGOUTFILENAME} ${SCRATCHDIR}"
ifdh cp -D ${TRIGOUTFILENAME} ${SCRATCHDIR}
rm ${TRIGOUTFILENAME}

# Do reconstruction (with filtering)
#lar -c ${INPUT_TAR_DIR_LOCAL}/protodunedm/srcs/pdhdbsmsimulation/example/runPandoraNeutrinoMC.fcl -n ${NEvents} -o ${RECOOUTFILENAME} -s ${TRIGOUTFILENAME}
#rm ${TRIGOUTFILENAME}
# Check output was successfully produced
#if [ ! -e ${RECOOUTFILENAME} ]; then
#  LOGYLOG "[ERROR]: Failed to produce expected reco file."
#  exit 1
#fi
# Copy reco file to output area
#LOGYLOG "ifdh cp -D ${RECOOUTFILENAME} ${SCRATCHDIR}/${RECOPATH}"
#ifdh cp -D ${RECOOUTFILENAME} ${SCRATCHDIR}/${RECOPATH}
#rm ${RECOOUTFILENAME}

##==========================
## End Production ==========
##==========================
