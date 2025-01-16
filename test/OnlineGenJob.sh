#!/bin/bash

TOTALEVENTS=0
EVENTSPERFILE=1

WORKDIR="/exp/dune/app/users/chasnip/CERN_Fellowship/protodunedm_mc_simulation_v10_01_04"
SRCS="srcs/pdhdbsmsimulation/example"
FCL_FILE="prod_beamneutrino_cosmic_protodunehd.fcl"
if [[ ! -f "${WORKDIR}/${SRCS}/$FCL_FILE" ]]; then
  echo "Error: File $FCL_FILE not found!"
  #exit 1
fi

for (( NSKIP=0; NSKIP<=${TOTALEVENTS}; NSKIP+=${EVENTSPERFILE} ))
do
  for wob in "wnp04"
  do
    for flav in "numu"
    do
      for decay in 1
      do
        echo "./runNeutrinoCosmicProd.sh $FCL_FILE $wob $flav $decay $EVENTSPERFILE $NSKIP"
        ./srcs/pdhdbsmsimulation/test/runNeutrinoCosmicProd.sh $FCL_FILE $wob $flav $decay $EVENTSPERFILE $NSKIP
      done
    done
  done
done

