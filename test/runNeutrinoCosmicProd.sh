#!/bin/bash

WORKDIR="/exp/dune/app/users/chasnip/CERN_Fellowship/protodunedm_mc_simulation"
SRCS="srcs/pdhdbsmsimulation/example"

PARAMETER="FluxFiles" # edit to get correct flux file
PDGPARAM="GenFlavors" # edit to get correct pdg
FCL_FILE="${1}"
WOBCONF="${2}" # e.g. wnp04
FLAV="${3}" # e.g. numu
DECAY="${4}" # e.g. 0,1,2,3,4
NEVENTS="${5}"
NSKIP="${6}"

OUTDIR="/exp/dune/data/users/chasnip/ProtoDUNESimulation/UpdatedNeutrinoSim/${WOBCONF}/gen"

GEN="pdhd_gen_nu_cosmic_rad_${WOBCONF}_${FLAV}_decaymode${DECAY}_${NEVENTS}_${NSKIP}.root"
G41="pdhd_g41gen_nu_cosmic_rad_${WOBCONF}_${FLAV}_decaymode${DECAY}_${NEVENTS}_${NSKIP}.root"
G42="pdhd_g42gen_nu_cosmic_rad_${WOBCONF}_${FLAV}_decaymode${DECAY}_${NEVENTS}_${NSKIP}.root"
DET1="pdhd_detsim1g4gen_nu_cosmic_rad_${WOBCONF}_${FLAV}_decaymode${DECAY}_${NEVENTS}_${NSKIP}.root"
DET2="pdhd_detsim2g4gen_nu_cosmic_rad_${WOBCONF}_${FLAV}_decaymode${DECAY}_${NEVENTS}_${NSKIP}.root"
RECO="pdhd_recodetsimg4gen_nu_cosmic_rad_${WOBCONF}_${FLAV}_decaymode${DECAY}_${NEVENTS}_${NSKIP}.root"

if [[ ! -f "${WORKDIR}/${SRCS}/$FCL_FILE" ]]; then
  echo "Error: File $FCL_FILE not found!"
  exit 1
fi

if [[ -z "$WOBCONF" || -z "$FLAV" || -z "$DECAY" ]]; then
  echo "Usage: $0 <fcl_file> <parameter> <new_value>"
  echo "$WOBCONF and $FLAV  and $DECAY"
  exit 1
fi

if [[ -z "$NEVENTS" || -z "$NSKIP" ]]; then
  NEVENTS=1
  NSKIP=1
fi

# Edit the parameter in the .fcl file
sed -i "s|^\(\s*.*\.${PARAMETER}:\s*\).*|\1[@local::${WOBCONF}.${FLAV}.decaymode[${DECAY}]]|" "${WORKDIR}/${SRCS}/$FCL_FILE"
echo "Updated $PARAMETER in $FCL_FILE to [@local::${WOBCONF}.${FLAV}.decaymode[${DECAY}]]"

sed -i "s|^\(\s*.*\.${PDGPARAM}:\s*\).*|\1[@local::${WOBCONF}.${FLAV}.pdg]|" "${WORKDIR}/${SRCS}/$FCL_FILE"
echo "Updated $PDGPARAM in $FCL_FILE to [@local::${WOBCONF}.${FLAV}.pdg]"

lar -c ${WORKDIR}/${SRCS}/${FCL_FILE} -n ${NEVENTS} -o "${OUTDIR}/${GEN}"
wait
lar -c standard_g4_protodunehd_stage1.fcl -n ${NEVENTS} -o ${OUTDIR}/${G41} -s "${OUTDIR}/${GEN}"
wait
lar -c standard_g4_protodunehd_stage2.fcl -n ${NEVENTS} -o ${OUTDIR}/${G42} -s "${OUTDIR}/${G41}"
wait
lar -c ${WORKDIR}/${SRCS}/standard_detsim_protodunehd_stage1_edit.fcl -n ${NEVENTS} -o ${OUTDIR}/${DET1} -s "${OUTDIR}/${G42}"
wait
lar -c standard_detsim_protodunehd_stage2.fcl -n ${NEVENTS} -o ${OUTDIR}/${DET2} -s "${OUTDIR}/${DET1}"
wait
lar -c ${WORKDIR}/${SRCS}/example/runPandoraNeutrino.fcl -n ${NEVENTS} -o ${OUTDIR}/${RECO} -s "${OUTDIR}/${DET2}"
