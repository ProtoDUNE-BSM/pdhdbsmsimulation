/////////////////////////////////////////////////////////////////////////////////////////////////
/// @file    HepMCFileGen_module.cc
/// @brief   Generator module reading HepMC-style text files for HNL simulation.
/// @authors Animesh Chatterjee (original author)
///          Hamza Amar Es-sghir (fixing and refactoring, HNL extensions, ProtoDUNE-HD/NP04 BSM)
///
/// @date    2024–2026
/// @version 2.0
/////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @class evgen::HepMCFileGen
 *
 * Reads an HNL-extended HepMC text file and produces `simb::MCTruth`
 * objects for downstream Geant4 simulation in LArSoft.
 *
 * ### Input file format
 *
 * Each event block begins with a header line:
 * ~~~
 * ### <eventNo>
 * ~~~
 * followed by a line of 20 HNL production/decay parameters:
 * ~~~
 * Pw Nw M4 Ua4 Ln x0 y0 z0 thetaN phiN xf yf zf Gamma_PtoN B_P Gamma_N B_N t_N t_nu PoT_f
 * ~~~
 * and then one or more particle lines, the HNL daughters, with the format:
 * ~~~
 * PDG  E  px  py  pz
 * ~~~
 *
 * Only particles with status code 1 are propagated by Geant4;
 * let Geant4 handle any decays.
 *
 * ### Units
 * - LArSoft uses **cm** for distances and **ns** for time.
 * - Input positions (xf, yf, zf) are in **metres** and converted internally.
 * - The use of `TLorentzVector` does not imply space and time share units
 *   (do not call `TLorentzVector::Boost()`).
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
#include "art/Framework/Principal/Run.h"
#include "art/Framework/Principal/SubRun.h"
#include "art/Framework/Services/Registry/ServiceHandle.h"
#include "art_root_io/TFileService.h"
#include "canvas/Utilities/Exception.h"

// Utility libraries
#include "fhiclcpp/ParameterSet.h"
#include "cetlib_except/exception.h"
#include "messagefacility/MessageLogger/MessageLogger.h"

// ROOT libraries
#include "TH1.h"
#include "TH2.h"
#include "TLorentzVector.h"

// C++ standard library
#include <cmath>
#include <fstream>
#include <limits>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace evgen {

  // Conversion factor from metres (input file) to centimetres (LArSoft).
  constexpr double kMtoCm = 100.0;

  // Holds the 20 HNL production/decay parameters read per event.
  struct HNLParameters {
    int    Pw         = 0;
    double Nw         = 0;
    double M4         = 0;
    double Ua4        = 0;
    double Ln         = 0;
    double x0         = 0;
    double y0         = 0;
    double z0         = 0;
    double thetaN     = 0;
    double phiN       = 0;
    double xf         = 0;
    double yf         = 0;
    double zf         = 0;
    double Gamma_PtoN = 0;
    double B_P        = 0;
    double Gamma_N    = 0;
    double B_N        = 0;
    double t_N        = 0;
    double t_nu       = 0;
    double PoT_f      = 0;
  };

  class HepMCFileGen;

} // namespace evgen

// =============================================================================
// Class declaration
// =============================================================================
class evgen::HepMCFileGen : public art::EDProducer {
public:
  explicit HepMCFileGen(fhicl::ParameterSet const& p);

  void produce(art::Event& e)     override;
  void beginJob()                 override;
  void beginRun(art::Run& run)    override;
  void endSubRun(art::SubRun& sr) override;

private:
  // --- helpers ---------------------------------------------------------------
  std::vector<double> ComputeDetectorLimits() const;
  void                BookHistograms();
  HNLParameters       ReadHNLParameters(std::istringstream& iss,
                                        const std::string& rawLine) const;
  std::vector<std::string> ReadParticleLines();
  void FillHNLHistograms(const HNLParameters& hnl) const;
  void FillParticleHistograms(int pdg, double px, double py, double pz,
                              double energy,
                              double x, double y, double z) const;

  // --- data members ----------------------------------------------------------
  const geo::Geometry* fGeom;               // Geometry service handle
  std::vector<double>  fDetectorLimits;     // Active volume {Xmin,Xmax,Ymin,Ymax,Zmin,Zmax}

  std::string                    fFilename;  // Path to HepMC input file
  std::unique_ptr<std::ifstream> fInputFile; // Input file stream (owned)

  double fEventsPerPOT;    // Events-per-POT scaling factor
  int    fEventsPerSubRun; // Processed event counter (reset per sub-run)

  // --- Momentum histograms ---------------------------------------------------
  TH1D* fhPx          = nullptr;
  TH1D* fhPy          = nullptr;
  TH1D* fhPz          = nullptr;
  TH1D* fhPxLep       = nullptr;
  TH1D* fhPyLep       = nullptr;
  TH1D* fhPzLep       = nullptr;
  TH1D* fhPxPi        = nullptr;
  TH1D* fhPyPi        = nullptr;
  TH1D* fhPzPi        = nullptr;
  TH2D* fhPxyLep      = nullptr;
  TH2D* fhPxyPi       = nullptr;

  // --- Position histograms ---------------------------------------------------
  TH1D* fhPosX        = nullptr;
  TH1D* fhPosY        = nullptr;
  TH1D* fhPosZ        = nullptr;
  TH2D* fhXY          = nullptr;
  TH2D* fhZY          = nullptr;
  TH2D* fhXZ          = nullptr;

  // --- Energy histograms -----------------------------------------------------
  TH1D* fhEnergyTot   = nullptr;
  TH1D* fhEnergyLep   = nullptr;
  TH1D* fhEnergyPi    = nullptr;

  // --- HNL parameter histograms ----------------------------------------------
  TH1D* fhPw          = nullptr;
  TH1D* fhNw          = nullptr;
  TH1D* fhM4          = nullptr;
  TH1D* fhUa4         = nullptr;
  TH1D* fhLn          = nullptr;
  TH1D* fhThetaN      = nullptr;
  TH1D* fhPhiN        = nullptr;
  TH1D* fhXf          = nullptr;
  TH1D* fhYf          = nullptr;
  TH1D* fhZf          = nullptr;
  TH1D* fhGammaPtoN   = nullptr;
  TH1D* fhBP          = nullptr;
  TH1D* fhGammaN      = nullptr;
  TH1D* fhBN          = nullptr;
  TH1D* fhTN          = nullptr;
  TH1D* fhTNu         = nullptr;
  TH1D* fhPoTf        = nullptr;
};

// =============================================================================
// Constructor
// =============================================================================
evgen::HepMCFileGen::HepMCFileGen(fhicl::ParameterSet const& p)
  : EDProducer{p}
  , fFilename{p.get<std::string>("filename")}
  , fEventsPerPOT{p.get<double>("EventsPerPOT", -1.0)}
  , fEventsPerSubRun{0}
{
  fGeom = &*art::ServiceHandle<geo::Geometry>();
  fDetectorLimits = ComputeDetectorLimits();

  produces<std::vector<simb::MCTruth>>();
  produces<sumdata::RunData, art::InRun>();
  produces<sumdata::POTSummary, art::InSubRun>();
}

// =============================================================================
// beginJob – open input file and book histograms
// =============================================================================
void evgen::HepMCFileGen::beginJob()
{
  mf::LogInfo("HepMCFileGen") << "Opening input file: " << fFilename;

  fInputFile = std::make_unique<std::ifstream>(fFilename, std::ifstream::in);
  if (!fInputFile->good()) {
    throw cet::exception("HepMCFileGen")
      << "Input text file " << fFilename << " cannot be read.\n";
  }

  BookHistograms();
}

// =============================================================================
// beginRun
// =============================================================================
void evgen::HepMCFileGen::beginRun(art::Run& run)
{
  fEventsPerSubRun = 0;
  art::ServiceHandle<geo::Geometry const> geo;
  run.put(std::make_unique<sumdata::RunData>(geo->DetectorName()), art::fullRun());
}

// =============================================================================
// endSubRun – write POT summary
// =============================================================================
void evgen::HepMCFileGen::endSubRun(art::SubRun& subrun)
{
  auto pot = std::make_unique<sumdata::POTSummary>();
  pot->totpot     = fEventsPerSubRun * fEventsPerPOT;
  pot->totgoodpot = fEventsPerSubRun * fEventsPerPOT;
  subrun.put(std::move(pot), art::subRunFragment());
}

// =============================================================================
// produce – main event loop body
// =============================================================================
void evgen::HepMCFileGen::produce(art::Event& e)
{
  if (!fInputFile->good() || fInputFile->peek() == EOF) {
    throw cet::exception("HepMCFileGen")
      << "Input text file cannot be read in produce().\n";
  }

  auto truthcol = std::make_unique<std::vector<simb::MCTruth>>();
  simb::MCTruth truth;

  // --- Read event header (### eventNo) ---------------------------------------
  std::string line;
  std::getline(*fInputFile, line);
  std::istringstream iss(line);

  std::string hashToken;
  int eventNo = 0;
  iss >> hashToken >> eventNo;
  mf::LogInfo("HepMCFileGen") << "Event number: " << eventNo + 1;

  // --- Read HNL parameters (20 values) --------------------------------------
  std::getline(*fInputFile, line);
  iss.clear();
  iss.str(line);
  const HNLParameters hnl = ReadHNLParameters(iss, line);

  mf::LogInfo("HepMCFileGen")
    << "HNL decay position (m): xf=" << hnl.xf
    << ", yf=" << hnl.yf << ", zf=" << hnl.zf;

  // --- Read daughter particle lines ------------------------------------------
  const auto particleLines = ReadParticleLines();

  // --- Process each particle -------------------------------------------------
  const double zHalfLength = (fDetectorLimits[5] - fDetectorLimits[4]) / 2.0;
  double totalEnergy = 0.0;
  int particleIndex = 0;

  for (const auto& pLine : particleLines) {
    iss.clear();
    iss.str(pLine);

    int    pdg = 0;
    double eLab = 0, px = 0, py = 0, pz = 0;
    if (!(iss >> pdg >> eLab >> px >> py >> pz)) {
      mf::LogWarning("HepMCFileGen")
        << "Failed to parse particle line: " << pLine;
      continue;
    }

    mf::LogDebug("HepMCFileGen")
      << "Particle " << particleIndex << ": PDG=" << pdg
      << ", E=" << eLab << " GeV, Px=" << px
      << ", Py=" << py << ", Pz=" << pz << " GeV/c";

    // Convert decay position from metres to cm; centre in z
    const double posX = hnl.xf * kMtoCm;
    const double posY = hnl.yf * kMtoCm;
    const double posZ = hnl.zf * kMtoCm + zHalfLength;

    MF_LOG_DEBUG("HepMCFileGen")
      << "  TPC Z length (cm): " << 2.0 * zHalfLength
      << "  Z limits (cm): [" << fDetectorLimits[4]
      << ", " << fDetectorLimits[5] << "]";

    // Build MCParticle and add to truth record
    TLorentzVector position(posX, posY, posZ, 0.0);
    TLorentzVector momentum(px, py, pz, eLab);

    simb::MCParticle particle(particleIndex, pdg, "primary");
    particle.AddTrajectoryPoint(position, momentum);
    truth.Add(particle);

    // Fill histograms
    FillParticleHistograms(pdg, px, py, pz, eLab, posX, posY, posZ);
    FillHNLHistograms(hnl);

    totalEnergy += eLab;
    ++particleIndex;
  }

  fhEnergyTot->Fill(totalEnergy);
  truthcol->push_back(truth);
  e.put(std::move(truthcol));
  ++fEventsPerSubRun;
}

// =============================================================================
// ReadHNLParameters – parse the 20-value HNL parameter line
// =============================================================================
evgen::HNLParameters evgen::HepMCFileGen::ReadHNLParameters(
    std::istringstream& iss, const std::string& rawLine) const
{
  HNLParameters h;
  if (!(iss >> h.Pw >> h.Nw >> h.M4 >> h.Ua4 >> h.Ln
            >> h.x0 >> h.y0 >> h.z0 >> h.thetaN >> h.phiN
            >> h.xf >> h.yf >> h.zf >> h.Gamma_PtoN >> h.B_P
            >> h.Gamma_N >> h.B_N >> h.t_N >> h.t_nu >> h.PoT_f)) {
    throw cet::exception("HepMCFileGen")
      << "Failed to parse HNL parameters from line: " << rawLine << "\n";
  }
  return h;
}

// =============================================================================
// ReadParticleLines – collect lines until the next "###" or EOF
// =============================================================================
std::vector<std::string> evgen::HepMCFileGen::ReadParticleLines()
{
  std::vector<std::string> lines;
  std::string line;

  while (std::getline(*fInputFile, line)) {
    if (line.find("###") != std::string::npos) {
      // Rewind so the next call to produce() sees this header
      fInputFile->seekg(
        -static_cast<std::streamoff>(line.length() + 1), std::ios::cur);
      break;
    }
    if (!line.empty()) {
      lines.push_back(line);
    }
  }
  return lines;
}

// =============================================================================
// FillHNLHistograms
// =============================================================================
void evgen::HepMCFileGen::FillHNLHistograms(const HNLParameters& h) const
{
  fhPw->Fill(h.Pw);
  fhNw->Fill(h.Nw);
  fhM4->Fill(h.M4);
  fhUa4->Fill(h.Ua4);
  fhLn->Fill(h.Ln);
  fhThetaN->Fill(h.thetaN);
  fhPhiN->Fill(h.phiN);
  fhXf->Fill(h.xf);
  fhYf->Fill(h.yf);
  fhZf->Fill(h.zf);
  fhGammaPtoN->Fill(h.Gamma_PtoN);
  fhBP->Fill(h.B_P);
  fhGammaN->Fill(h.Gamma_N);
  fhBN->Fill(h.B_N);
  fhTN->Fill(h.t_N);
  fhTNu->Fill(h.t_nu);
  fhPoTf->Fill(h.PoT_f);
}

// =============================================================================
// FillParticleHistograms
// =============================================================================
void evgen::HepMCFileGen::FillParticleHistograms(
    int pdg, double px, double py, double pz,
    double energy, double x, double y, double z) const
{
  fhPx->Fill(px);
  fhPy->Fill(py);
  fhPz->Fill(pz);

  fhPosX->Fill(x);
  fhPosY->Fill(y);
  fhPosZ->Fill(z);

  fhXY->Fill(x, y);
  fhZY->Fill(z, y);
  fhXZ->Fill(x, z);

  const int absPdg = std::abs(pdg);

  // This must be replaced or generalized if more particle types are added to the input file
  // This does not affect the MCTruth record, which is built directly from the input file without filtering
  if (absPdg == 13 || absPdg == 11) {
    // Lepton (mu or e)
    fhPxLep->Fill(px);
    fhPyLep->Fill(py);
    fhPzLep->Fill(pz);
    fhPxyLep->Fill(px, py);
    fhEnergyLep->Fill(energy);
  } else if (absPdg == 211) {
    // Charged pion
    fhPxPi->Fill(px);
    fhPyPi->Fill(py);
    fhPzPi->Fill(pz);
    fhPxyPi->Fill(px, py);
    fhEnergyPi->Fill(energy);
  }
}

// =============================================================================
// BookHistograms – create all TH1/TH2 objects via TFileService
// =============================================================================
void evgen::HepMCFileGen::BookHistograms()
{
  art::ServiceHandle<art::TFileService> tfs;

  // Momentum
  fhPx    = tfs->make<TH1D>("hPx", ";p_{x} (GeV/c)", 20, 0, 2);
  fhPy    = tfs->make<TH1D>("hPy", ";p_{y} (GeV/c)", 20, 0, 2);
  fhPz    = tfs->make<TH1D>("hPz", ";p_{z} (GeV/c)", 50, 0, 200);

  fhPxLep = tfs->make<TH1D>("hPxLep", ";p_{x}^{lep} (GeV/c)", 20, 0, 2);
  fhPyLep = tfs->make<TH1D>("hPyLep", ";p_{y}^{lep} (GeV/c)", 20, 0, 2);
  fhPzLep = tfs->make<TH1D>("hPzLep", ";p_{z}^{lep} (GeV/c)", 50, 0, 200);

  fhPxPi  = tfs->make<TH1D>("hPxPi", ";p_{x}^{#pi} (GeV/c)", 20, 0, 2);
  fhPyPi  = tfs->make<TH1D>("hPyPi", ";p_{y}^{#pi} (GeV/c)", 20, 0, 2);
  fhPzPi  = tfs->make<TH1D>("hPzPi", ";p_{z}^{#pi} (GeV/c)", 50, 0, 200);

  fhPxyLep = tfs->make<TH2D>("hPxyLep",
    ";p_{x}^{lep} (GeV/c);p_{y}^{lep} (GeV/c)", 20, 0, 2, 20, 0, 2);
  fhPxyPi  = tfs->make<TH2D>("hPxyPi",
    ";p_{x}^{#pi} (GeV/c);p_{y}^{#pi} (GeV/c)", 20, 0, 2, 20, 0, 2);

  // Positions (birth / general / TPC)
  fhPosX   = tfs->make<TH1D>("hx",    ";x position (cm)", 40, -400, 400);
  fhPosY   = tfs->make<TH1D>("hy",    ";y position (cm)", 50, -100, 900);
  fhPosZ   = tfs->make<TH1D>("hz",    ";z position (cm)", 50, -100, 900);

  fhXY = tfs->make<TH2D>("hxy", ";x (cm);y (cm)", 40, -400, 400, 50, -100, 900);
  fhZY = tfs->make<TH2D>("hzy", ";z (cm);y (cm)", 50, -100, 900, 50, -100, 900);
  fhXZ = tfs->make<TH2D>("hxz", ";x (cm);z (cm)", 40, -400, 400, 50, -100, 900);

  // Energy
  fhEnergyTot = tfs->make<TH1D>("hEnergyTot", ";E_{tot} (GeV)", 50, 0, 200);
  fhEnergyLep = tfs->make<TH1D>("hEnergyLep", ";E_{lep} (GeV)", 50, 0, 200);
  fhEnergyPi  = tfs->make<TH1D>("hEnergyPi",  ";E_{#pi} (GeV)", 50, 0, 200);

  // HNL parameters
  fhPw        = tfs->make<TH1D>("hPw",          ";P_{w}",           50, 0, 200);
  fhNw        = tfs->make<TH1D>("hNw",          ";N_{w}",           50, 0, 200);
  fhM4        = tfs->make<TH1D>("hM4",          ";M_{4} (GeV)",     50, 0, 200);
  fhUa4       = tfs->make<TH1D>("hUa4",         ";|U_{a4}|^{2}",   50, 0, 200);
  fhLn        = tfs->make<TH1D>("hLn",          ";L_{N} (m)",       50, 0, 200);
  fhThetaN    = tfs->make<TH1D>("hthetaN",       ";#theta_{N}",     50, 0, 200);
  fhPhiN      = tfs->make<TH1D>("hphiN",         ";#phi_{N}",      50, 0, 200);
  fhXf        = tfs->make<TH1D>("hxf",           ";x_{f} (m)",     50, 0, 200);
  fhYf        = tfs->make<TH1D>("hyf",           ";y_{f} (m)",     50, 0, 200);
  fhZf        = tfs->make<TH1D>("hzf",           ";z_{f} (m)",     50, 0, 200);
  fhGammaPtoN = tfs->make<TH1D>("hGamma_PtoN",   ";#Gamma_{P#rightarrowN}", 50, 0, 200);
  fhBP        = tfs->make<TH1D>("hB_P",          ";#beta_{P}",     50, 0, 200);
  fhGammaN    = tfs->make<TH1D>("hGamma_N",      ";#gamma_{N}",    50, 0, 200);
  fhBN        = tfs->make<TH1D>("hB_N",          ";#beta_{N}",     50, 0, 200);
  fhTN        = tfs->make<TH1D>("ht_N",          ";t_{N} (ns)",    50, 0, 200);
  fhTNu       = tfs->make<TH1D>("ht_nu",         ";t_{#nu} (ns)",  50, 0, 200);
  fhPoTf      = tfs->make<TH1D>("hPoT_f",        ";PoT",           50, 0, 200);
}

// =============================================================================
// ComputeDetectorLimits – aggregate active volume from all TPCs
// =============================================================================
std::vector<double> evgen::HepMCFileGen::ComputeDetectorLimits() const
{
  double xMin =  std::numeric_limits<double>::max();
  double xMax = -std::numeric_limits<double>::max();
  double yMin =  std::numeric_limits<double>::max();
  double yMax = -std::numeric_limits<double>::max();
  double zMin =  std::numeric_limits<double>::max();
  double zMax = -std::numeric_limits<double>::max();

  for (const auto& tpc : fGeom->Iterate<geo::TPCGeo>()) {
    const auto centre = tpc.GetCenter();
    const double hw = tpc.HalfWidth();
    const double hh = tpc.HalfHeight();
    const double hl = 0.5 * tpc.Length();

    xMin = std::min(xMin, centre.X() - hw);
    xMax = std::max(xMax, centre.X() + hw);
    yMin = std::min(yMin, centre.Y() - hh);
    yMax = std::max(yMax, centre.Y() + hh);
    zMin = std::min(zMin, centre.Z() - hl);
    zMax = std::max(zMax, centre.Z() + hl);
  }

  mf::LogInfo("HepMCFileGen")
    << "Active volume (cm): "
    << "X=[" << xMin << ", " << xMax << "]  "
    << "Y=[" << yMin << ", " << yMax << "]  "
    << "Z=[" << zMin << ", " << zMax << "]";

  return {xMin, xMax, yMin, yMax, zMin, zMax};
}

DEFINE_ART_MODULE(evgen::HepMCFileGen)
