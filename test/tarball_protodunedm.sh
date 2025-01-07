#!/bin/bash

# Bash script to make the tarball that is given to the FermiGrid production job

# DMWORKDIR=/exp/dune/app/users/chasnip/CERN_Fellowship/protodunedm_mc_simulation
# Hopefully you are currently in the working directory!
DMWORKDIR=${PWD}

LOCALPRODDIR=${DMWORKDIR}/localProducts_larsoft_*

if [ -z "${LOCALPRODDIR}" ]; then
  echo "[ERROR]: not source localproducts is not set up, cannot tar up required binaries."
  exit 1
fi

mkdir tar_state; cd tar_state
mkdir protodunedm
cp -r ${LOCALPRODDIR} ./protodunedm/
mkdir protodunedm/srcs
cp -r ${DMWORKDIR}/srcs/pdhdbsmsimulation ./protodunedm/srcs
#cp -r ${DMWORKDIR}/srcs/dunetrigger ./protodunedm/srcs

tar --exclude '.git' -zcvf LocalProdNumu.Blob.tar.gz ./*

cd ..

mv tar_state/LocalProdNumu.Blob.tar.gz .
rm -rf tar_state

