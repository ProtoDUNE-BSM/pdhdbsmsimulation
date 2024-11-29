////////////////////////////////////////////////////////////////////////
//// Class:       PDHDTAGenFilter
//// Plugin Type: filter (Unknown Unknown)
//// File:        PDHDTAGenFilter_module.cc
//// Author:      Ciaran Hasnip (CERN)
//// 
//// Filter that removes simulated events that have not generated a TA.
//// It is assumed that the input data has trigger emulation and used the
//// ADCSimpleWindow TA algorithm.
//////////////////////////////////////////////////////////////////////////

#include <iostream>
#include <utility>
#include <set>

#include "lardataobj/RawData/RawDigit.h"
#include "lardataobj/RawData/RDTimeStamp.h"

#include "art/Framework/Core/EDAnalyzer.h" 
#include "art/Framework/Core/EDFilter.h" 
#include "art/Framework/Core/ModuleMacros.h" 
#include "art/Framework/Principal/Event.h" 
#include "art_root_io/TFileService.h"

#include "detdataformats/trigger/TriggerActivityData.hpp"

namespace pdhd {

//-------------------------------------
class PDHDTAGenFilter : public art::EDFilter {
  public:
    explicit PDHDTAGenFilter(fhicl::ParameterSet const & pset);
    virtual ~PDHDTAGenFilter() {};
    virtual bool filter(art::Event& e);
    void beginJob();

  private:

    int fRun;
    int fSubRun;
    unsigned int fEventID;

    std::string fInputLabel;
    bool fDebug;
    
    //std::vector<dunedaq::trgdataformats::TriggerActivityData> fTriggerActivity;
};

//-------------------------------------
PDHDTAGenFilter::PDHDTAGenFilter::PDHDTAGenFilter(fhicl::ParameterSet const & pset):
  EDFilter(pset), 
  fInputLabel(pset.get<std::string>("InputTag")),
  fDebug(pset.get<bool>("Debug")) {}

//-------------------------------------
bool PDHDTAGenFilter::filter(art::Event & evt) {
 
  fRun = evt.run();
  fSubRun = evt.subRun();
  fEventID = evt.id().event();
  
  std::cout << "###PDHDTAGenFilter###"<< std::endl
    << "Seach for TA in Event " << fEventID << " in Run " << fRun << std::endl << std::endl;

  art::Handle<std::vector<dunedaq::trgdataformats::TriggerActivityData>> taHandle;
  if (!evt.getByLabel(fInputLabel, taHandle)) {
    return false;
  } else {
    if (taHandle->size() == 0) return false;
    else {
      std::cout << ">>> Found " << taHandle->size() << " TAs in Event " << fEventID << std::endl;
      return true;
    }
  }
}

//-------------------------------------
void PDHDTAGenFilter::beginJob() {}

DEFINE_ART_MODULE(PDHDTAGenFilter)

}
