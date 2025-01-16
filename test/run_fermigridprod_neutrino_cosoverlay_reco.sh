#!/bin/bash

LOGYLOG () {
  echo ${1}
}

# Input parameters
PARAMETER="FluxFiles"
PDGPARAM="GenFlavors"
FCL_FILE="${1}"
WOBCONF="${2}" # e.g. wnp04
FLAV="${3}" # e.g. numu
DECAY="${4}" # e.g. 0,1,2,3,4
NEvents="${5}"
SkipEvents="${6}"

echo "$FCL_FILE, $WOBCONF, $FLAV, $DECAY, $NEVENTS and $NSKIP"

WOBDIR=${WOBCONF}

# dCache directories for input/ouput
#SCRATCHDIR=/pnfs/dune/scratch/users/chasnip/ProtoDUNEBSM/NeutrinoSim
#XROOTDSCRATCHDIR=root://fndca1.fnal.gov:1094/pnfs/fnal.gov/usr/dune/scratch/users/chasnip/ProtoDUNEBSM/NeutrinoSim
SCRATCHDIR=/pnfs/dune/scratch/users/chasnip/ProtoDUNEBSM/NeutrinoSim_v2
XROOTDSCRATCHDIR=root://fndca1.fnal.gov:1094/pnfs/fnal.gov/usr/dune/scratch/users/chasnip/ProtoDUNEBSM/NeutrinoSim_v2

# Names of ouput files for different stages

FLAVDIR="${FLAV}"
DECAYDIR="decay${DECAY}"

GENDIR="gen"
DETSIMDIR="detsim"
RECODIR="reco"

GENOUTFILENAME="${WOBCONF}_${FLAV}_decaymode${DECAY}_gen_${SkipEvents}_${NEvents}_${CLUSTER}_${PROCESS}.root"
G41OUTFILENAME="${WOBCONF}_${FLAV}_decaymode${DECAY}_g41gen_${SkipEvents}_${NEvents}_${CLUSTER}_${PROCESS}.root"
G42OUTFILENAME="${WOBCONF}_${FLAV}_decaymode${DECAY}_g42gen_${SkipEvents}_${NEvents}_${CLUSTER}_${PROCESS}.root"
DETSIM1OUTFILENAME="${WOBCONF}_${FLAV}_decaymode${DECAY}_detsim1g4gen_${SkipEvents}_${NEvents}_${CLUSTER}_${PROCESS}.root"
DETSIM2OUTFILENAME="${WOBCONF}_${FLAV}_decaymode${DECAY}_detsim2g4gen_${SkipEvents}_${NEvents}_${CLUSTER}_${PROCESS}.root"
TRIGOUTFILENAME="${WOBCONF}_${FLAV}_decaymode${DECAY}_triggereddetsim2g4gen_${SkipEvents}_${NEvents}_${CLUSTER}_${PROCESS}.root"
RECOOUTFILENAME="${WOBCONF}_${FLAV}_decaymode${DECAY}_recotriggereddetsim2g4gen_${SkipEvents}_${NEvents}_${CLUSTER}_${PROCESS}.root"

GENPATH=${WOBDIR}/${FLAVDIR}/${DECAYDIR}/${GENDIR}
DETSIMPATH=${WOBDIR}/${FLAVDIR}/${DECAYDIR}/${DETSIMDIR}
RECOPATH=${WOBDIR}/${FLAVDIR}/${DECAYDIR}/${RECODIR}

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
export WIRECELL_PATH=$MRB_SOURCE/pdhdbsmsimulation/wire-cell-cfg:$WIRECELL_PATH
echo ${WIRECELL_PATH}

# Files in INPUT_TAR_DIR_LOCAL are read-only - copy to condor job directory for editing
cp ${INPUT_TAR_DIR_LOCAL}/protodunedm/srcs/pdhdbsmsimulation/example/$FCL_FILE mytmpfcl  # do a cp, not mv
rm ${INPUT_TAR_DIR_LOCAL}/protodunedm/srcs/pdhdbsmsimulation/example/$FCL_FILE # This really just removes the link
mv mytmpfcl ${_CONDOR_JOB_IWD}/$FCL_FILE # now it's available as an editable regular file.

# May not need to do this when get an update in dunereco session
#cp ${INPUT_TAR_DIR_LOCAL}/protodunedm/srcs/pdhdbsmsimulation/example/PandoraSettings_Master_ProtoDUNE_HD_Neutrino.xml ${_CONDOR_JOB_IWD}

# Edit the FluxFiles and GenFlavors parameters in the generator fcl file to select the correct flux file and corresponding neutrino flavour
sed -i "s|^\(\s*.*\.${PARAMETER}:\s*\).*|\1[@local::${WOBCONF}.${FLAV}.decaymode[${DECAY}]]|" "$FCL_FILE"
echo "Updated $PARAMETER in $FCL_FILE to [@local::${WOBCONF}.${FLAV}.decaymode[${DECAY}]]"

sed -i "s|^\(\s*.*\.${PDGPARAM}:\s*\).*|\1[@local::${WOBCONF}.${FLAV}.pdg]|" "$FCL_FILE"
echo "Updated $PDGPARAM in $FCL_FILE to [@local::${WOBCONF}.${FLAV}.pdg]"
# Finish editing generator fcl file

##==========================
## Do Production ===========
##==========================

# Generate neutrino + cosmics + radiological backgrounds
LOGYLOG "lar -c ${FCL_FILE} -n ${NEvents} -o "${GENOUTFILENAME}""
lar -c ${FCL_FILE} -n ${NEvents} -o "${GENOUTFILENAME}"
# Check output was successfully produced
if [ ! -e ${GENOUTFILENAME} ]; then
  LOGYLOG "[ERROR]: Failed to produce expected gen file."
  exit 1
fi
LOGYLOG "ifdh cp -D ${GENOUTFILENAME} ${SCRATCHDIR}/${GENPATH}"
ifdh cp -D ${GENOUTFILENAME} ${SCRATCHDIR}/${GENPATH}
rm ${GENOUTFILENAME}

# Do G4 simulation
#lar -c standard_g4_protodunehd_stage1.fcl -n ${NEvents} -o ${G41OUTFILENAME} -s ${XROOTDSCRATCHDIR}/${GENPATH}/${GENOUTFILENAME}
lar -c ${INPUT_TAR_DIR_LOCAL}/protodunedm/srcs/pdhdbsmsimulation/example/standard_g4_protodunehd_edit.fcl -n ${NEvents} -o ${G41OUTFILENAME} -s ${XROOTDSCRATCHDIR}/${GENPATH}/${GENOUTFILENAME}
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

# Do reconstruction (with filtering)
lar -c ${INPUT_TAR_DIR_LOCAL}/protodunedm/srcs/pdhdbsmsimulation/example/runPandoraNeutrinoMC.fcl -n ${NEvents} -o ${RECOOUTFILENAME} -s ${TRIGOUTFILENAME}
rm ${TRIGOUTFILENAME}
# Check output was successfully produced
if [ ! -e ${RECOOUTFILENAME} ]; then
  LOGYLOG "[ERROR]: Failed to produce expected reco file."
  exit 1
fi
# Copy reco file to output area
LOGYLOG "ifdh cp -D ${RECOOUTFILENAME} ${SCRATCHDIR}/${RECOPATH}"
ifdh cp -D ${RECOOUTFILENAME} ${SCRATCHDIR}/${RECOPATH}
rm ${RECOOUTFILENAME}

##==========================
## End Production ==========
##==========================
