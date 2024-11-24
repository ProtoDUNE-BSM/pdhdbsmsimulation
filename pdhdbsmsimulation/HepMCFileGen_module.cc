////////////////////////////////////////////////////////////////////////
//
//// Class:       HepMCFileGen
//// Module Type: producer
//// File:         HepMCFileGen_module.cc
////
////
//// Animesh Chatterjee
////
//////////////////////////////////////////////////////////////////////////

/**
 * @class evgen::HepMCFileGen
 *
 *  This module assumes that the input file has the hepevt format for
 *  each event to be simulated. In brief each event contains at least two
 *  lines. The first line contains two entries, the event number (which is
 *  ignored in ART/LArSoft) and the number of particles in the event. Each
 *  following line containes 15 entries to describe each particle. The entries
 *  are:
 *
 *  1.  status code (should be set to 1 for any particle to be tracked, others
 *      won't be tracked)
 *  2.  the pdg code for the particle
 *  3.  the entry of the first mother for this particle in the event,
 *      0 means no mother
 *  4.  the entry of the second mother for this particle in the event,
 *      0 means no mother
 *  5. the entry of the first daughter for this particle in the event,
 *      0 means no daughter
 *  6. the entry of the second daughter for this particle in the event,
 *      0 means no daughter
 *  7. x component of the particle momentum
 *  8. y component of the particle momentum
 *  9. z component of the particle momentum
 *  10. energy of the particle
 *  11. mass of the particle
 *  12. x position of the particle initial position
 *  13. y position of the particle initial position
 *  14. z position of the particle initial position
 *  15. time of the particle production
 *
 *  For example, if you want to simulate a single muon with a 5 GeV energy
 *  moving only in the z direction, the entry would be (see onemuon.hepmc):
 *  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *  0 1
 *  1 13 0 0 0 0 0. 0. 1.0 5.0011 0.105 1.0 1.0 1.0 0.0
 *  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *  Or if you want to simulate a muon neutrino event (see oneneutrino.hepmc): 
 *  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *  0 3
 *  0 14 0 0 0 0 0.00350383 0.002469 0.589751 0.589766 0 208.939 63.9671 10.9272 4026.32
 *  1 13 1 0 0 0 -0.168856 -0.0498011 0.44465 0.489765 105.658 208.939 63.9671 10.9272 4026.32
 *  1 2212 1 0 0 0 0.151902 -0.124578 0.0497377 0.959907 938.272 208.939 63.9671 10.9272 4026.32
 *  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *  here the first particle is the initial state neutrino (status code 0, meaning initial state, 
 *  not to be prooagated by GEANT). The other particles are the initial state particles.
 *  For the neutrino, write ccnc in place of 1st daugther, and mode (qe, res, ...) 
 *  in place of 2nd daugther.
 *
 *  There are some assumptions that go into using this format that may not
 *  be obvious.  The first is that only particles with status code = 1
 *  are tracked in the LArSoft/Geant4 combination making the mother daughter
 *  relations somewhat irrelevant. That also means that you should let
 *  Geant4 handle any decays.
 *
 *  The units in LArSoft are cm for distances and ns for time.
 *  The use of `TLorentzVector` below does not imply space and time have the same units
 *   (do not use `TLorentzVector::Boost()`).
 */

// LArSoft includes
#include "larcore/CoreUtils/ServiceUtil.h"
#include "larcore/Geometry/Geometry.h"
#include "larcorealg/Geometry/GeometryCore.h"
#include "larcoreobj/SimpleTypesAndConstants/geo_types.h"
#include "larcoreobj/SummaryData/RunData.h"
#include "larcoreobj/SummaryData/POTSummary.h"
#include "nusimdata/SimulationBase/MCTruth.h"
#include "nusimdata/SimulationBase/MCParticle.h"

// Framework includes
#include "art/Framework/Core/EDProducer.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"
#include "art/Framework/Principal/Run.h"
#include "art/Framework/Principal/SubRun.h"
#include "art/Framework/Services/Registry/ServiceHandle.h"
#include "art_root_io/TFileService.h"
#include "canvas/Persistency/Common/FindManyP.h"
#include "canvas/Utilities/Exception.h"

// Utility libraries
#include "fhiclcpp/ParameterSet.h"
#include "cetlib_except/exception.h"
#include "messagefacility/MessageLogger/MessageLogger.h"
//#include "CLHEP/Random/RandFlat.h"

// ROOT libraries
#include "TH1.h"
#include "TH2.h"
#include "TVector3.h"
#include "TLorentzVector.h"
 
//#include "nutools/RandomUtils/NuRandomService.h"
//#include "CLHEP/Random/RandFlat.h"

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>


/* #include "ifdh.h"
#include "ifdh_art/IFDHService/IFDH_service.h" // ifdh_ns::IFDH
#undef USE_IFDH_SERVICE // ifdh for now
*/
namespace evgen {
  class HepMCFileGen;
}
class evgen::HepMCFileGen : public art::EDProducer {
public:
  explicit HepMCFileGen(fhicl::ParameterSet const & p);
  void produce(art::Event & e)                    override;
  void beginJob()                                 override;
  void endJob()                                   override;
  void beginRun(art::Run & run)                   override;
  void endSubRun(art::SubRun& sr)                 override;
private:

  void GetDetectorLimits();
  void DefDetectorLimits();
  geo::GeometryCore const *fTotalGeom;
  std::stringstream geo_ss;

  std::string fFilename;
  std::ifstream* fInputFile;
  
  double         fEventsPerPOT;     ///< Number of events per POT (to be set)
  int            fEventsPerSubRun;  ///< Keeps track of the number of processed events per subrun
  // ifdh_ns::ifdh* fIFDH;             ///< (optional) flux file handling

  // Need to translate to LArSoft geometry coordinates
  // Hard code dimensions of generated volume for events
  //std::vector<double> fGenVolDimensions = {600., 700., 600.};
  std::vector<double> fGenVolDimensions = {600., 600., 600.};
  std::vector<double> fOriginTranslate;

  int inLArBefore;
  int outLArBefore;
  int inLArAfter;
  int outLArAfter;

  // Visible particle energies
  TH1D* fPx;
  TH1D* fPy;
  TH1D* fPz;
  TH1D* fPxLep;
  TH1D* fPyLep;
  TH1D* fPzLep;
  TH1D* fPxPi;
  TH1D* fPyPi;
  TH1D* fPzPi;
  TH2D* fPxyLep;
  TH2D* fPxyPi;
  TH1D* fbx;
  TH1D* fby;
  TH1D* fbz;
  TH1D* fx;
  TH1D* fy;
  TH1D* fz;
  TH1D* fTPCx;
  TH1D* fTPCy;
  TH1D* fTPCz;
  TH2D* fxy;
  TH2D* fzy;
  TH2D* fxz;
  TH1D* fEnergyTot;
  TH1D* fEnergyLep;
  TH1D* fEnergyPi;
};
//------------------------------------------------------------------------------
evgen::HepMCFileGen::HepMCFileGen(fhicl::ParameterSet const & p)
  : EDProducer{p}
  , fFilename(p.get<std::string>("filename"))
  , fInputFile(nullptr)
  , fEventsPerPOT{p.get<double>("EventsPerPOT", -1.)}
  , fEventsPerSubRun(0)
{
  this->DefDetectorLimits();
  produces< std::vector<simb::MCTruth>   >();
  produces< sumdata::RunData, art::InRun >();
  produces< sumdata::POTSummary, art::InSubRun >();
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
void evgen::HepMCFileGen::beginJob()
{
  std::cout << "Filename : " << fFilename << std::endl;
  fInputFile = new std::ifstream(fFilename.c_str(), std::ifstream::in);

  // check that the file is a good one
  if( !fInputFile->good() )
    throw cet::exception("HepMCFileGen") << "input text file "
					<< fFilename
					<< " cannot be read.\n";

  inLArBefore = 0;
  outLArBefore = 0;
  inLArAfter = 0;
  outLArAfter = 0;

  art::ServiceHandle<art::TFileService> tfs;
  fPx = tfs->make<TH1D>("hPx",";x-Momentum (GeV/c)",20,0,2);
  fPy = tfs->make<TH1D>("hPy",";y-Momentum (GeV/c)",20,0,2);
  fPz = tfs->make<TH1D>("hPz",";z-Momentum (GeV/c)",50,0,200);
  fPxLep = tfs->make<TH1D>("hPxLep",";x-Momentum Lep. (GeV/c)",20,0,2);
  fPyLep = tfs->make<TH1D>("hPyLep",";y-Momentum Lep. (GeV/c)",20,0,2);
  fPzLep = tfs->make<TH1D>("hPzLep",";z-Momentum Lep. GeV/c)",50,0,200);
  fPxPi = tfs->make<TH1D>("hPxPi",";x-Momentum #pi (GeV/c)",20,0,2);
  fPyPi = tfs->make<TH1D>("hPyPi",";y-Momentum #pi (GeV/c)",20,0,2);
  fPzPi = tfs->make<TH1D>("hPzPi",";z-Momentum #pi (GeV/c)",50,0,200);
  fPxyLep = tfs->make<TH2D>("hPxyLep",";x-Momentum Lep. (GeV/c);y-Momentum Lep. (GeV/c)",20,0, 2, 20, 0, 2);  
  fPxyPi = tfs->make<TH2D>("hPxyPi",";x-Momentum #pi (GeV/c);y-Momentum #pi (GeV/c)",20,0, 2, 20, 0, 2);  
  fbx = tfs->make<TH1D>("hbx",";x-Position (cm)",40,-400,400);
  fby = tfs->make<TH1D>("hby",";y-Position (cm)",40,-400,400);
  fbz = tfs->make<TH1D>("hbz",";z-Position (cm)",40,-400,400);
  fx = tfs->make<TH1D>("hx",";x-Position (cm)",40,-400,400);
  fy = tfs->make<TH1D>("hy",";y-Position (cm)",50,-100,900);
  fz = tfs->make<TH1D>("hz",";z-Position (cm)",50,-100,900);
  fTPCx = tfs->make<TH1D>("hTPCx",";x-Position in TPC (cm)",40,-400,400);
  fTPCy = tfs->make<TH1D>("hTPCy",";y-Position in TPC (cm)",50,-100,900);
  fTPCz = tfs->make<TH1D>("hTPCz",";z-Position in TPC (cm)",50,-100,900);
  fxy = tfs->make<TH2D>("hxy",";x-Position (cm);y-Position (cm)",40,-400,400,50,-100,900);
  fzy = tfs->make<TH2D>("hzy",";z-Position (cm);y-Position (cm)",50,-100,900,50,-100,900);
  fxz = tfs->make<TH2D>("hxz",";x-Position (cm);z-Position (cm)",40,-400,400,50,-100,900);
  fEnergyTot = tfs->make<TH1D>("hEnergyTot",";Total Energy (GeV)",50,0,200);
  fEnergyLep = tfs->make<TH1D>("hEnergyLep",";Total Energy Lep. (GeV)",50,0,200);
  fEnergyPi = tfs->make<TH1D>("hEnergyPi",";Total Energy #pi (GeV)",50,0,200);

  return;
  
}
//------------------------------------------------------------------------------
void evgen::HepMCFileGen::beginRun(art::Run& run)
{
    fEventsPerSubRun = 0;
    art::ServiceHandle<geo::Geometry const> geo;
    run.put(std::make_unique<sumdata::RunData>(geo->DetectorName()), art::fullRun());
  }
//------------------------------------------------------------------------------
void evgen::HepMCFileGen::endSubRun(art::SubRun& sr)
  {
    auto p = std::make_unique<sumdata::POTSummary>();
    p->totpot     = fEventsPerSubRun * fEventsPerPOT;
    p->totgoodpot = fEventsPerSubRun * fEventsPerPOT;
    sr.put(std::move(p), art::subRunFragment());
    return;
  }
//------------------------------------------------------------------------------
void evgen::HepMCFileGen::produce(art::Event & e)
{
  if( !fInputFile->good() || fInputFile->peek() == EOF) {
    throw cet::exception("HepMCFileGen") << "input text file "
                                        << " cannot be read in produce().\n";
  }
  std::unique_ptr< std::vector<simb::MCTruth> > truthcol(new std::vector<simb::MCTruth>);
  simb::MCTruth truth;
  // declare the variables for reading in the event record
  int            event               = 0;
  unsigned short nParticles          = 0;
  int            status              = 0;
  int            pdg                 = 0;
  int            firstMother         = 0;
  int            secondMother        = 0;
  int            firstDaughter       = 0;
  int            secondDaughter      = 0;
  double         xMomentum           = 0.;
  double         yMomentum           = 0.;
  double         zMomentum           = 0.;
  double         energy              = 0.;
  double         mass                = 0.;
  double         xPosition           = 0.;
  double         yPosition           = 0.;
  double         zPosition           = 0.;
  double         time                = 0.;
  // read in line to get event number and number of particles
  std::string oneLine;
  std::getline(*fInputFile, oneLine);
  std::istringstream inputLine;
  inputLine.str(oneLine);
  inputLine >> event >> nParticles;

  //bool lowPiE(false);
  double total_energy(0);
  // now read in all the lines for the particles
  // in this interaction. only particles with
  // status = 1 get tracked in Geant4. see GENIE GHepStatus
  for(unsigned short i = 0; i < nParticles; ++i){
    std::getline(*fInputFile, oneLine);
    inputLine.clear();
    inputLine.str(oneLine);
    inputLine >> status >> pdg
              >> firstMother >> secondMother >> firstDaughter >> secondDaughter
              >> xMomentum   >> yMomentum    >> zMomentum     >> energy >> mass
              >> xPosition   >> yPosition    >> zPosition     >> time;
    //std::cout<< "pdg_all" << pdg << "xmomentum_all" << xMomentum << "ymomentum_all" << 
	//	yMomentum << "zposition_all" << zPosition << std::endl;

    std::string vol_before = fTotalGeom->VolumeName({xPosition*0.1, 
			                                 yPosition*0.1, 
											 zPosition*0.1});

	std::string vol_sub_before =  vol_before.substr(0, vol_before.find("_"));
    if (vol_sub_before == "volCryostat" || vol_sub_before == "volTPCActiveInner") inLArBefore++;
	else outLArBefore++;
    
	fbx->Fill(xPosition*0.1);
    fby->Fill(yPosition*0.1);
    fbz->Fill(zPosition*0.1);

	xPosition = xPosition * 0.1 + fOriginTranslate.at(0);
	yPosition = yPosition * 0.1 + fOriginTranslate.at(1);
    zPosition = zPosition * 0.1 + fOriginTranslate.at(2);
    std::string vol_after = fTotalGeom->VolumeName({xPosition, yPosition, zPosition});
	
	std::string vol_sub_after =  vol_after.substr(0, vol_after.find("_"));
    if (vol_sub_after == "volCryostat" || vol_sub_after == "volTPCActiveInner") inLArAfter++;
	else outLArAfter++;

	//std::cout << "vol BEFORE: " << vol_before << "; AFTER: " << vol_after << std::endl;

    TLorentzVector pos(xPosition, yPosition, zPosition, time);
    TLorentzVector mom(xMomentum, yMomentum, zMomentum, energy);
    simb::MCParticle part(i, pdg, "primary", firstMother, mass, status);
    part.AddTrajectoryPoint(pos, mom); // file is in mm but we want cm

	//if (std::fabs(pdg) == 211 && energy < 10.) lowPiE = true; // Skip if pion too high energy

    truth.Add(part);
    // std::cout << i << "  Particle added with Pdg " << part.PdgCode() << ", Mother " << 
	//	 part.Mother() << ", track id " << part.TrackId() << ", ene " << part.E() << std::endl;

    fPx->Fill(xMomentum);
    fPy->Fill(yMomentum);
    fPz->Fill(zMomentum);
    fx->Fill(xPosition);
    fy->Fill(yPosition);
    fz->Fill(zPosition);
	if (vol_sub_after == "volTPCActiveInner") {
      fTPCx->Fill(xPosition);
      fTPCy->Fill(yPosition);
      fTPCz->Fill(zPosition);
	}
    fxy->Fill(xPosition, yPosition);
    fzy->Fill(zPosition, yPosition);
    fxz->Fill(xPosition, zPosition);
    //fEnergyTot->Fill(energy);
	total_energy += energy;

	if (std::fabs(pdg) == 13 || std::fabs(pdg) == 11) {
      fPxLep->Fill(xMomentum);
      fPyLep->Fill(yMomentum);
      fPzLep->Fill(zMomentum);
      fPxyLep->Fill(xMomentum, yMomentum);
	  fEnergyLep->Fill(energy);
    } else if (std::fabs(pdg) == 211) {
      fPxPi->Fill(xMomentum);
      fPyPi->Fill(yMomentum);
      fPzPi->Fill(zMomentum);
      fPxyPi->Fill(xMomentum, yMomentum);
	  fEnergyPi->Fill(energy);
    }
  }
  fEnergyTot->Fill(total_energy);
  //if (lowPiE) truthcol->push_back(truth);

  truthcol->push_back(truth);
  e.put(std::move(truthcol));
  fEventsPerSubRun++;
  return;
}

//------------------------------------------------------------------------------
void evgen::HepMCFileGen::endJob() {
  std::cout << "\n Before translation: InLAr = " << inLArBefore << "; OutLAr = " << outLArBefore << std::endl;
  std::cout << "\n After translation: InLAr = " << inLArAfter << "; OutLAr = " << outLArAfter << std::endl;
}

//------------------------------------------------------------------------------
void evgen::HepMCFileGen::DefDetectorLimits()
{
  this->GetDetectorLimits();     
	
  std::cout << geo_ss.str();
			                                   
  return;                        
}                                

//------------------------------------------------------------------------------
void evgen::HepMCFileGen::GetDetectorLimits(){

  fTotalGeom = lar::providerFrom<geo::Geometry>();

  geo::BoxBoundedGeo fDetGeom = fTotalGeom->DetectorEnclosureBox("volDetEnclosure");

  geo_ss << "\n" << "Det enclosure: " << fDetGeom.Min() << " -- "
     << fDetGeom.Max() << " cm => ( " << fDetGeom.SizeX() << " x "
     << fDetGeom.SizeY() << " x " << fDetGeom.SizeZ() << " ) cm^3"
	 << "Det Centre: (" << fDetGeom.CenterX() << ", " << fDetGeom.CenterY()
	 << ", " << fDetGeom.CenterZ() << ")"; 

  for (auto const& cryostat : fTotalGeom->Iterate<geo::CryostatGeo>()) {
    geo_ss << "\n" << "Cryostat centre point: " << cryostat.GetCenter() << "\n";
	cryostat.PrintCryostatInfo(geo_ss, "  ", cryostat.MaxVerbosity);

	double totalTPCVol(0);
    double totalGenVol = fGenVolDimensions.at(0) * fGenVolDimensions.at(1) * fGenVolDimensions.at(2);

    const unsigned int nTPCs = cryostat.NTPC();
	for (unsigned int t = 0; t < nTPCs; ++t) {
      const geo::TPCGeo& tpc = cryostat.TPC(t);
      totalTPCVol += tpc.TotalVolume()->Capacity();

	  geo_ss << "\n" << "    ";
	  tpc.PrintTPCInfo(geo_ss, "    ", tpc.MaxVerbosity);
	  if (t == 1) {
	    fOriginTranslate.push_back(0.);
	    fOriginTranslate.push_back(fGenVolDimensions.at(1) / 2.);
	    fOriginTranslate.push_back(fGenVolDimensions.at(2) / 2.);
	  }
	}
	geo_ss << "\n Translation coords: " << fOriginTranslate.at(0) << ", " 
		<< fOriginTranslate.at(1) << ", " << fOriginTranslate.at(2) << std::endl;
	geo_ss << "\n Events generated in volume: " << totalGenVol << " cm^3"
		<< "\n Total TPC volume: " << totalTPCVol  << " cm^3" << std::endl;
  }

}

DEFINE_ART_MODULE(evgen::HepMCFileGen)
