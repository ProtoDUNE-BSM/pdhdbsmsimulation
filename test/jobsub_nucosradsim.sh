#!/bin/bash

# Default production strategy is to generate 1 neutrino event per job for a specific wobbling
# configuration, neutrino flavour and hadron decay production mode

#DMWORKDIR=/exp/dune/app/users/chasnip/CERN_Fellowship/protodunedm_mc_simulation
# Hopefully you are in the work dir!
DMWORKDIR=${PWD}
TOTALEVENTS=100 # total events to generate per wob/flav/decay
NEVENTS=1 # number of events per job - probably want to keep this to 1

FCL="prod_beamneutrino_cosmic_protodunehd.fcl"

# Loop over the wobbling configuration "wnp04", "w133" or "w000"
# Loop over the neutrino flavours
# Loop of the hadron decay production machanism for the neutrinos
# numu and nue and have 5 decay modes: 0, 1, 2, 3, 4
# numubar and nuebar have 3 decay modes: 0, 1, 2

# 8GB RAM is enough for numu production, nues require closer to 12GB, which is a little large

for (( NSKIP=0; NSKIP<=${TOTALEVENTS}; NSKIP+=${NEVENTS} ))
do
  for wob in "wnp04"
  do
    for flav in "numu"
    do
      for decay in 0 1 2 3 4
      do
        jobsub_submit --group=dune --singularity-image /cvmfs/singularity.opensciencegrid.org/fermilab/fnal-wn-sl7:latest \
          --append_condor_requirements='(((target.HAS_Singularity == true) && (target.HAS_CVMFS_dune_opensciencegrid_org == true) && (target.HAS_CVMFS_larsoft_opensciencegrid_org == true) && (target.HAS_CVMFS_fifeuser1_opensciencegrid_org == true) && (target.HAS_CVMFS_fifeuser2_opensciencegrid_org == true) && (target.HAS_CVMFS_fifeuser3_opensciencegrid_org == true) && (target.HAS_CVMFS_fifeuser4_opensciencegrid_org == true))) && (TARGET.Arch == "X86_64") && (TARGET.OpSys == "LINUX") && (TARGET.HasFileTransfer) && (TARGET.has_avx2 == true)' \
          -e GFAL_PLUGIN_DIR=/usr/lib64/gfal2-plugins -e GFAL_CONFIG_DIR=/etc/gfal2.d \
          --resource-provides=usage_model=DEDICATED,OPPORTUNISTIC --expected-lifetime=48h --disk=10GB --memory=12GB --cpu=1 -N 1 \
          --tar_file_name=dropbox://${DMWORKDIR}/LocalProdNumu.Blob.tar.gz \
          file://${DMWORKDIR}/srcs/pdhdbsmsimulation/test/run_fermigridprod_neutrino_cosoverlay_reco.sh ${FCL} $wob $flav $decay ${NEVENTS} ${NSKIP}
      done
    done
  done
done
