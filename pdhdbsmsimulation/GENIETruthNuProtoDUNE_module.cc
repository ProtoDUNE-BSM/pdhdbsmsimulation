////////////////////////////////////////////////////////////////////////
// Class:       GENIETruthNuProtoDUNE
// Plugin Type: analyzer (Unknown Unknown)
// File:        GENIETruthNuProtoDUNE_module.cc
//
// Generated at Mon Apr  8 02:24:48 2024 by Ciaran Hasnip using cetskelgen
// from cetlib version 3.18.02.
////////////////////////////////////////////////////////////////////////

#include "larcore/CoreUtils/ServiceUtil.h"
#include "larcore/Geometry/Geometry.h"
#include "larcorealg/Geometry/GeometryCore.h"
#include "larcoreobj/SimpleTypesAndConstants/geo_types.h"
//#include "lardata/DetectorInfoServices/DetectorClocksService.h"
#include "lardataobj/RecoBase/Cluster.h"
#include "lardataobj/RecoBase/Hit.h"
#include "lardataobj/Simulation/SimChannel.h"
#include "larsim/Simulation/LArG4Parameters.h"
#include "nusimdata/SimulationBase/MCParticle.h"
#include "nusimdata/SimulationBase/MCTruth.h"

#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"
#include "art/Framework/Principal/Run.h"
#include "art/Framework/Principal/SubRun.h"
#include "canvas/Utilities/InputTag.h"
#include "fhiclcpp/ParameterSet.h"
#include "messagefacility/MessageLogger/MessageLogger.h"

// Additional Framework includes
#include "art_root_io/TFileService.h"

// ROOT includes
#include <TTree.h>
#include <TH1.h>

#include <string>

namespace ana {
  class GENIETruthNuProtoDUNE;
}

// Define analyser class
class ana::GENIETruthNuProtoDUNE : public art::EDAnalyzer {
public:
  explicit GENIETruthNuProtoDUNE(fhicl::ParameterSet const& p);
  // The compiler-generated destructor is fine for non-base
  // classes without bare pointers or other resource use.

  // Plugins should not be copied or assigned.
  GENIETruthNuProtoDUNE(GENIETruthNuProtoDUNE const&) = delete;
  GENIETruthNuProtoDUNE(GENIETruthNuProtoDUNE&&) = delete;
  GENIETruthNuProtoDUNE& operator=(GENIETruthNuProtoDUNE const&) = delete;
  GENIETruthNuProtoDUNE& operator=(GENIETruthNuProtoDUNE&&) = delete;

  // Required functions.
  void analyze(art::Event const& e) override;

  // Selected optional functions.
  void beginJob() override;
  void endJob() override;

private:

  TTree *fSimulationNtuple;

  // Declare member data here.
  unsigned int fEventID;
  unsigned int fRun;
  unsigned int fSubRun;

  art::InputTag fMCTruthLabel; ///< The name of the producer that tracked
                                          ///< simulated particles through the detector
  

  int fSimPDG;     ///< PDG ID of the particle being processed
  int fSimTrackID; ///< GEANT ID of the particle being processed
  int fCCNC; ///< Is neutrino interaction a CC or NC interaction
  unsigned int fTPCID; ///< TPC ID where neutrino interacts

  double fE;
  double fnuStartX;
  double fnuStartY;
  double fnuStartZ;
  double fnuVertexX;
  double fnuVertexY;
  double fnuVertexZ;

  bool fInFV;
  
  geo::GeometryCore const* fGeometryService; ///< pointer to Geometry provider
  std::vector<double> fFiducialBoundaries;
  double fZedge;
};


// Analyser class constructor
ana::GENIETruthNuProtoDUNE::GENIETruthNuProtoDUNE(fhicl::ParameterSet const& p)
  : EDAnalyzer{p} 
  , fMCTruthLabel(p.get<std::string>("MCTruthLabel"))

  // More initializers here.
{
  
  // Get a pointer to the geometry service provider.
  fGeometryService = lar::providerFrom<geo::Geometry>();
  // TPC 1 is the first proper TPC - TPC 0 is for track stubs
  const geo::TPCGeo& tpc = fGeometryService->Cryostat().TPC(1);
  fFiducialBoundaries.push_back(0.); // central x
  fFiducialBoundaries.push_back(tpc.Width() - 0.05*tpc.Width()); // outer x
  fFiducialBoundaries.push_back(0.05*tpc.Height()); // bottom y
  fFiducialBoundaries.push_back(tpc.Height() - 0.05*tpc.Height()); // top y
  fFiducialBoundaries.push_back(0.05*(tpc.Length()*2.));
  fFiducialBoundaries.push_back((tpc.Length()*2.) - 0.05*(tpc.Length()*2));

  for (size_t i=0; i<fFiducialBoundaries.size(); i++) {
    std::cout << "\n bound = " << fFiducialBoundaries.at(i);
  }

  fZedge = tpc.Length()*2.;

  // Call appropriate consumes<>() for any products to be retrieved by this module.
  consumes<std::vector<simb::MCTruth>>(fMCTruthLabel);
}

void ana::GENIETruthNuProtoDUNE::analyze(art::Event const& e)
{
  // Implementation of required member function here.
  fEventID = e.id().event();
  fRun = e.run();
  fSubRun = e.subRun();

  fInFV = false;

  // Define a "handle" to point to a vector of the objects.
  auto truthHandle = e.getValidHandle<std::vector<simb::MCTruth>>(fMCTruthLabel);

  for (auto const& truth : (*truthHandle)) {
    if (truth.NeutrinoSet()) {
      const auto &nu = truth.GetNeutrino();
      const auto &neutrino = nu.Nu();

      std::cout << "Get neutrino info." << std::endl;

      fSimPDG = neutrino.PdgCode();

      fCCNC = nu.CCNC();

      fE = neutrino.E();


      double fPrimaryStart[4];
      double fPrimaryVertex[4];

      const size_t numberTrajectoryPoints = neutrino.NumberTrajectoryPoints();
      const int last = numberTrajectoryPoints - 1;
      const TLorentzVector& positionStart = neutrino.Position(0);
      const TLorentzVector& positionEnd = neutrino.Position(last);
      // Set the vertex position - it should be the same value for each event	
      positionStart.GetXYZT(fPrimaryStart);
      positionEnd.GetXYZT(fPrimaryVertex);

      fnuStartX = fPrimaryStart[0];
      fnuStartY = fPrimaryStart[1];
      fnuStartZ = fPrimaryStart[2];
      fnuVertexX = fPrimaryVertex[0];
      fnuVertexY = fPrimaryVertex[1];
      fnuVertexZ = fPrimaryVertex[2];

      if (std::fabs(fnuVertexX) < fFiducialBoundaries.at(1)) {
        if (fnuVertexY > fFiducialBoundaries.at(2) && 
            fnuVertexY < fFiducialBoundaries.at(3)) {
          if (fnuVertexZ > fFiducialBoundaries.at(4) && 
              fnuVertexZ < fFiducialBoundaries.at(5)) {
            fInFV = true;
          }
        }
      }

      geo::Point_t nuV_point(fnuVertexX, fnuVertexY, fnuVertexZ);
      fTPCID = fGeometryService->FindTPCAtPosition(nuV_point).TPC;
      if (fTPCID > 7 || fTPCID < 0) fTPCID = -999;
      
      // Store total event outputs in the TTree
      fSimulationNtuple->Fill();
    }
  }
}

// Define outputs at start of the job
void ana::GENIETruthNuProtoDUNE::beginJob() {
  // Implementation of optional member function here.
  art::ServiceHandle<art::TFileService> tfs;

  // Get TFileService to create an output tree
  fSimulationNtuple = tfs->make<TTree>("tree", "DM Output Tree");
  fSimulationNtuple->SetAutoSave(0);

  // Add branches to TTree
  fSimulationNtuple->Branch("eventID", &fEventID);
  fSimulationNtuple->Branch("SubRun", &fSubRun, "SubRun/I");
  fSimulationNtuple->Branch("Run", &fRun, "Run/I");
  fSimulationNtuple->Branch("PDG", &fSimPDG, "PDG/I");
  fSimulationNtuple->Branch("CCNC", &fCCNC, "CCNC/I");
  fSimulationNtuple->Branch("TPCID", &fTPCID);

  fSimulationNtuple->Branch("E", &fE, "E/D");
  fSimulationNtuple->Branch("nuStartX", &fnuStartX, "nuStartX/D");
  fSimulationNtuple->Branch("nuStartY", &fnuStartY, "nuStartY/D");
  fSimulationNtuple->Branch("nuStartZ", &fnuStartZ, "nuStartZ/D");
  fSimulationNtuple->Branch("nuVertexX", &fnuVertexX, "nuVertexX/D");
  fSimulationNtuple->Branch("nuVertexY", &fnuVertexY, "nuVertexY/D");
  fSimulationNtuple->Branch("nuVertexZ", &fnuVertexZ, "nuVertexZ/D");
  fSimulationNtuple->Branch("InFV", &fInFV, "InFV/B");

}

void ana::GENIETruthNuProtoDUNE::endJob()
{
  // Implementation of optional member function here.
}

DEFINE_ART_MODULE(ana::GENIETruthNuProtoDUNE)
