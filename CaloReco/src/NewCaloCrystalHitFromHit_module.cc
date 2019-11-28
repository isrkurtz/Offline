//Author: S Middleton
//Date: Nov 2019
//Purpose: For the purpose of crystal hit making from online

#include "art/Framework/Core/EDProducer.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Principal/Event.h"
#include "art_root_io/TFileService.h"
#include "art_root_io/TFileDirectory.h"

#include "CalorimeterGeom/inc/Calorimeter.hh"
#include "GeometryService/inc/GeomHandle.hh"
#include "RecoDataProducts/inc/NewCaloCrystalHitCollection.hh"
#include "RecoDataProducts/inc/NewCaloRecoDigiCollection.hh"

#include "TH1F.h"
#include "TH2F.h"

#include <iostream>
#include <string>
#include <cmath>



namespace mu2e {


  class NewCaloCrystalHitFromHit : public art::EDProducer {
  public:

    explicit NewCaloCrystalHitFromHit(fhicl::ParameterSet const& pset) :
      art::EDProducer{pset},
      caloDigisToken_{consumes<NewCaloRecoDigiCollection>(pset.get<std::string>("caloDigisModuleLabel"))},
      time4Merge_          (pset.get<double>     ("time4Merge")),
      diagLevel_           (pset.get<int>        ("diagLevel",0))
    {
      produces<NewCaloCrystalHitCollection>();
    }

    void beginJob() override;
    void produce(art::Event& e) override;

  private:

    typedef art::Ptr<NewCaloRecoDigi> CaloRecoDigiPtr;

    art::ProductToken<NewCaloRecoDigiCollection> const caloDigisToken_;
    double      time4Merge_;
    int         diagLevel_;

    std::vector<std::vector<const NewCaloRecoDigi*>> hitMap_;  

    //TODO - do we need these?
    TH1F*  hEdep_;
    TH1F*  hTime_;
    TH1F*  hNRo_;
    TH1F*  hEdep_Cry_;
    TH1F*  hDelta_;
    TH2F*  hNRo2_;
    TH1F*  hEdep1_;
    TH1F*  hEdep2_;

    void MakeCaloCrystalHits(NewCaloCrystalHitCollection& CaloCrystalHits, const art::ValidHandle<NewCaloRecoDigiCollection>& recoCaloDigisHandle);

    void FillBuffer(int crystalId, int nRoid, double time, double timeErr, double eDep, double eDepErr,
                    std::vector<CaloRecoDigiPtr>& buffer, NewCaloCrystalHitCollection& CaloCrystalHits);
  };


  //--------------------------------------------
  void NewCaloCrystalHitFromHit::beginJob()
  {
    if (diagLevel_ > 2) {
      art::ServiceHandle<art::TFileService> tfs;
      hEdep_     = tfs->make<TH1F>("hEdep",   "Hit energy deposition",        200,   0.,  500);
      hTime_     = tfs->make<TH1F>("hTime",   "Hit time ",                  12000,   0., 2000);
      hNRo_      = tfs->make<TH1F>("hNRo",    "Number RO ",                    10,   0.,   10);
      hEdep_Cry_ = tfs->make<TH1F>("hEdepCry","Energy deposited per crystal",2000,   0., 2000);
      hDelta_    = tfs->make<TH1F>("hDelta",  "Hit time difference",          200, -20,    20);
      hNRo2_     = tfs->make<TH2F>("hNRo2",   "Number RO ",                    5,    0., 5, 50, 0, 50);
      hEdep1_    = tfs->make<TH1F>("hEdep1",  "Hit energy deposition",        200,   0.,  100);
      hEdep2_    = tfs->make<TH1F>("hEdep2",  "Hit energy deposition",        200,   0.,  100);
    }
  }


  //------------------------------------------------------------
  void NewCaloCrystalHitFromHit::produce(art::Event& event)
  {
    //Get by Handle:
    auto const& recoCaloDigisHandle = event.getValidHandle(caloDigisToken_);
    auto CaloCrystalHits = std::make_unique<NewCaloCrystalHitCollection>();
    MakeCaloCrystalHits(*CaloCrystalHits, recoCaloDigisHandle);
    //Move Collection to event:
    event.put(std::move(CaloCrystalHits));
  }


  //--------------------------------------------------------------------------------------------------------------
  void NewCaloCrystalHitFromHit::MakeCaloCrystalHits(NewCaloCrystalHitCollection& CaloCrystalHits, art::ValidHandle<NewCaloRecoDigiCollection> const& recoCaloDigisHandle)
  {
    Calorimeter const &cal = *(GeomHandle<Calorimeter>());
    auto const& recoCaloDigis = *recoCaloDigisHandle;
    NewCaloRecoDigi const* base = &recoCaloDigis.front(); // What if recoCaloDigis is empty?


    //extend hitMap_ if needed and clear it
    if (cal.nRO() > int(hitMap_.size()))
      for (int i = hitMap_.size(); i<= cal.nRO(); ++i) hitMap_.push_back(std::vector<const NewCaloRecoDigi*>());
    for (size_t i=0; i<hitMap_.size(); ++i) hitMap_[i].clear();

    // fill the map that associate for each crystal the corresponding CaloRecoDigi indexes
    for (unsigned int i=0; i< recoCaloDigis.size(); ++i)
      {
        int crystalId = cal.caloInfo().crystalByRO(recoCaloDigis[i].ROid());
        hitMap_[crystalId].push_back(&recoCaloDigis[i]);
      }


    for (unsigned int crystalId=0;crystalId<hitMap_.size();++crystalId)
      {
        std::vector<const NewCaloRecoDigi*> &hits = hitMap_[crystalId];
        
        //check if empty:
        if (hits.empty()) continue;

        //sort hits:
        std::sort(hits.begin(),hits.end(),[](const auto a, const auto b){return a->time() < b->time();});

        //find first and last hit:
        auto startHit = hits.begin();
        auto endHit   = hits.begin();

        //creat a buffer
        std::vector<CaloRecoDigiPtr> buffer;
        double timeW(0);
        double eDepTot(0),eDepTotErr(0);
        int nRoid(0);

        //loop through hits:
        while (endHit != hits.end())
          {
            //time:
            double deltaTime = (*endHit)->time()-(*startHit)->time();
            //fill histograms if requested:
            if (diagLevel_ > 2) hDelta_->Fill(deltaTime);
            //if > than set merge time:
            if (deltaTime > time4Merge_) 
              {
                double time = timeW/nRoid;
                double timeErr = 0;
                //fill that buffer: //TODO -->peakpos, flags etc.
                FillBuffer(crystalId, nRoid, time, timeErr, eDepTot/nRoid, eDepTotErr/nRoid, buffer, CaloCrystalHits);
                //clear:
                buffer.clear();
                timeW      = 0.0;
                eDepTot    = 0.0;
                eDepTotErr = 0.0;
                nRoid      = 0;
                startHit   = endHit;
              }
            else //if not then:
              {
                //add up times
                timeW      += (*endHit)->time();
                //add up energies
                eDepTot    += (*endHit)->energyDep();
                eDepTotErr += (*endHit)->energyDepErr() * (*endHit)->energyDepErr();
                //increment no. ROs
                ++nRoid;
                //get index:
                size_t index = *endHit - base;
                //add to end of hit buffer:
                buffer.push_back(art::Ptr<NewCaloRecoDigi>(recoCaloDigisHandle, index));
                //increment:
                ++endHit;
              }

          }
        //get time and error
        double time = timeW/nRoid;
        double timeErr = 0;
        //Finally fill buffer:
        FillBuffer(crystalId, nRoid, time, timeErr, eDepTot/nRoid, eDepTotErr/nRoid, buffer, CaloCrystalHits);
      }


    if ( diagLevel_ > 0 )
      {
        printf("[NewCaloCrystalHitFromHit::produce] produced RecoCrystalHits ");
        printf(": CaloCrystalHits.size()  = %i \n", int(CaloCrystalHits.size()));
      }

  }

  void NewCaloCrystalHitFromHit::FillBuffer(int const crystalId,
                                         int const nRoid,
                                         double const time,
                                         double const timeErr,
                                         double const eDep,
                                         double const eDepErr,
                                         std::vector<CaloRecoDigiPtr>& buffer,
                                         NewCaloCrystalHitCollection& CaloCrystalHits)
  {
    //TODO--> flags etc.?
    CaloCrystalHits.emplace_back(NewCaloCrystalHit(crystalId, nRoid, time, timeErr, eDep, eDepErr, buffer));

    if (diagLevel_ > 1)
      {
        std::cout<<"[NewCaloCrystalHitFromHit] created hit in crystal id="<<crystalId<<"\t with time="<<time<<"\t eDep="<<eDep<<"\t  from "<<nRoid<<" RO"<<std::endl;

        if (diagLevel_ > 2)
          {
            hTime_->Fill(time);
            hEdep_->Fill(eDep);
            hNRo_->Fill(nRoid);
            hEdep_Cry_->Fill(crystalId,eDep);
            hNRo2_->Fill(nRoid,eDep);
            if (nRoid==1) hEdep1_->Fill(eDep);
            if (nRoid==2) hEdep2_->Fill(eDep);
          }
      }
  }


}

DEFINE_ART_MODULE(mu2e::NewCaloCrystalHitFromHit);
