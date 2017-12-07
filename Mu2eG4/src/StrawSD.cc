//
// Define a sensitive detector for Straws.
// This version does not use G4HCofThisEvent etc...
// Framwork DataProducts are used instead
//
// This version only works for the TTracker.  It also allows that the tracker may not
// be centered in its mother volume.
//
// $Id: StrawSD.cc,v 1.45 2014/01/06 20:56:15 kutschke Exp $
// $Author: kutschke $
// $Date: 2014/01/06 20:56:15 $
//
// Original author Rob Kutschke
//

#include <cstdio>
#include <limits>
#include <cmath>


// Framework includes
#include "messagefacility/MessageLogger/MessageLogger.h"
#include "cetlib_except/exception.h"

// Mu2e includes
#include "Mu2eG4/inc/StrawSD.hh"
#include "Mu2eG4/inc/Mu2eG4UserHelpers.hh"
#include "Mu2eG4/inc/EventNumberList.hh"
#include "Mu2eG4/inc/PhysicsProcessInfo.hh"
#include "TTrackerGeom/inc/TTracker.hh"
#include "GeometryService/inc/GeometryService.hh"
#include "GeometryService/inc/GeomHandle.hh"
#include "GeometryService/inc/DetectorSystem.hh"
#include "GeneralUtilities/inc/TwoLinePCA.hh"
#include "GeneralUtilities/inc/LinePointPCA.hh"
#include "ConfigTools/inc/SimpleConfig.hh"

// G4 includes
#include "G4RunManager.hh"
#include "G4Step.hh"
#include "G4ThreeVector.hh"
#include "G4ios.hh"

//
// Outstanding questions:
//
// 1) Why is diffAngle so big?
//

using namespace std;

namespace mu2e {

  StrawSD::StrawSD(G4String name, SimpleConfig const & config ):
    Mu2eSensitiveDetector(name,config),
    _nStrawsPerPlane(0),
    _nStrawsPerPanel(0),
    _TrackerVersion(0),
    _supportModel(),
    _verbosityLevel(0)
  {

    art::ServiceHandle<GeometryService> geom;

    if ( !geom->hasElement<TTracker>() ) {
      throw cet::exception("GEOM")
        << "Expected one of Trackers but did not find it.\n";
    }

    if ( geom->hasElement<TTracker>() ) {

      GeomHandle<TTracker> ttracker;

      const Plane& plane = ttracker->getPlane(0);
      const Panel& panel = plane.getPanel(0);
      const Layer&  layer  = panel.getLayer(0);

      _nStrawsPerPanel = panel.nLayers()  * layer.nStraws(); // fixme: use StrawId2
      _nStrawsPerPlane = plane.nPanels() * _nStrawsPerPanel;

      _TrackerVersion = config.getInt("TTrackerVersion",3);

      _npanels  = StrawId2::_npanels;
      _panelsft = StrawId2::_panelsft;
      _planesft = StrawId2::_planesft;

      _verbosityLevel = max(verboseLevel,config.getInt("ttracker.verbosityLevel",0)); // Geant4 SD verboseLevel
      _supportModel   = ttracker->getSupportModel();

      if ( _TrackerVersion < 3 ) {
        throw cet::exception("StrawSD")
          << "Expected TTrackerVersion >= 3 but found " << _TrackerVersion <<endl;
        // esp take a look at the detectorOrigin calculation
      }

      if (_verbosityLevel>2) {
        cout << __func__ << " _nStrawsPerPlane " << _nStrawsPerPlane << endl;
        cout << __func__ << " _nStrawsPerPanel " << _nStrawsPerPanel << endl;
      }

    }

  }

  G4bool StrawSD::ProcessHits(G4Step* aStep,G4TouchableHistory*){

    _currentSize += 1;

    if ( _sizeLimit>0 && _currentSize>_sizeLimit ) {
      if( (_currentSize - _sizeLimit)==1 ) {
        mf::LogWarning("G4") << "Maximum number of particles reached in "
                             << SensitiveDetectorName
                             << ": "
                             << _currentSize << endl;
      }
      return false;
    }

    G4double edep = aStep->GetTotalEnergyDeposit();
    G4double step = aStep->GetStepLength();

    // Skip most points with no energy.
    if ( edep == 0 ) {

      // I am not sure why we get these cases but we do.  Skip them.
      if ( step == 0. ) {
        //ProcessCode endCode(_processInfo->
        //                    findAndCount(Mu2eG4UserHelpers::findStepStoppingProcessName(aStep)));
        //cout << "Weird: " << endCode << endl;
        return false;
      }

      // No need to save photons and (anti-)neutrons.
      int pdgId(aStep->GetTrack()->GetDefinition()->GetPDGEncoding());

      if ( pdgId == PDGCode::gamma ||
           std::abs(pdgId) == PDGCode::n0 ) return false;
    }

    const G4TouchableHandle & touchableHandle = aStep->GetPreStepPoint()->GetTouchableHandle();

    static G4ThreeVector detectorOrigin = GetTrackerOrigin();

    // this is Geant4 SD verboseLevel
    if (_verbosityLevel>2) {
      cout << __func__ << " detectorOrigin   " << detectorOrigin  << endl;
      cout << __func__ << " det name         " << touchableHandle->GetVolume(2)->GetName() << endl;
    }

    // Position at start of step point, in world system and in
    // a system in which the center of the tracking detector is the origin.
    G4ThreeVector prePosWorld   = aStep->GetPreStepPoint()->GetPosition();
    G4ThreeVector prePosTracker = prePosWorld - detectorOrigin;

    G4ThreeVector preMomWorld = aStep->GetPreStepPoint()->GetMomentum();

    G4Event const* event = G4RunManager::GetRunManager()->GetCurrentEvent();

    G4int en = event->GetEventID();
    G4int ti = aStep->GetTrack()->GetTrackID();

    if (_verbosityLevel>2) {

      G4int cn = touchableHandle->GetCopyNumber();
      G4int rn = touchableHandle->GetReplicaNumber();

      cout << __func__ << " copy 0 1 2 3 4 " <<
        setw(4) << touchableHandle->GetCopyNumber(0) <<
        setw(4) << touchableHandle->GetCopyNumber(1) <<
        setw(4) << touchableHandle->GetCopyNumber(2) <<
        setw(4) << touchableHandle->GetCopyNumber(3) <<
        setw(4) << touchableHandle->GetCopyNumber(4) << endl;

      cout << __func__ << " PV Name Mother Name " <<
        touchableHandle->GetVolume(0)->GetName() << " " <<
        touchableHandle->GetVolume(1)->GetName() << " " <<
        touchableHandle->GetVolume(2)->GetName() << " " <<
        touchableHandle->GetVolume(3)->GetName() << " " <<
        touchableHandle->GetVolume(4)->GetName() << endl;

      cout << __func__ << " hit info: event track copyn replican:        " <<
        setw(4) << en << " " <<
        setw(4) << ti << " " <<
        setw(4) << cn << " " <<
        setw(4) << rn << endl;

    }

    // getting the panel/plane number

    G4int sdcn(0);
    uint16_t sdcn2(0);
    if ( _TrackerVersion >= 3) {

      if ( _supportModel == SupportModel::simple ){
        sdcn = touchableHandle->GetCopyNumber(1) +
          _nStrawsPerPanel*(touchableHandle->GetCopyNumber(2)) +
          _nStrawsPerPlane*(touchableHandle->GetCopyNumber(3));
      } else {
        sdcn = touchableHandle->GetCopyNumber(0) +
          _nStrawsPerPanel*(touchableHandle->GetCopyNumber(1));

        // sdcn calculated for the use with _allStraws2_p
        // from panel copy number (0..215) and straw number
        // can also use plane copy number;

        // given how StrawId is calculated we need to extract the
        // plane number and panel number in that plane
        // effectively: sdcn2 = 1024*planeNumber + 128*panelNumber + strawNumber;
        uint16_t panelNumber = touchableHandle->GetCopyNumber(1)%_npanels;
        uint16_t planeNumber = touchableHandle->GetCopyNumber(1)/_npanels;
        uint16_t panelNumberShifted = panelNumber << _panelsft;
        uint16_t planeNumberShifted = planeNumber << _planesft;
        sdcn2 = planeNumberShifted + panelNumberShifted +
          touchableHandle->GetCopyNumber(0);

        if (_verbosityLevel>3) {

          GeomHandle<TTracker> ttracker;
          // print out info based on the old StrawID etc... first
           cout << __func__ <<  " sdcn, sdcn2, panelNumber, panelNumberShifted, planeNumber, planeNumberShifted, sid2, osid : "
               << setw(6) << sdcn
               << setw(6) << sdcn2
               << setw(6) << panelNumber
               << setw(6) << panelNumberShifted
               << setw(6) << planeNumber
               << setw(6) << planeNumberShifted;
          const Straw& straw = ttracker->getStraw( StrawIndex(sdcn) );
          cout << setw(7) <<  straw.id2()
               << setw(10) << straw.id()
               << endl;

          // print out info based on the new StrawID2 etc...
          cout << __func__ << " sid2, osid : ";
          const Straw& straw2 = ttracker->straw(StrawIndex2(sdcn2));
          cout << setw(7) <<  straw2.id2()
               << setw(10) << straw2.id()
               << endl;
        }

      }

    } else {

      throw cet::exception("GEOM")
        << "Expected TrackerVersion of 3 but found " << _TrackerVersion << endl;

    }

    if (_verbosityLevel>2) {

      cout << __func__ << " hit info: event track panel plane straw:   " <<
        setw(4) << en << " " <<
        setw(4) << ti << " " <<
        setw(4) << touchableHandle->GetCopyNumber(2) << " " <<
        setw(4) << touchableHandle->GetCopyNumber(3) << " " <<
        setw(6) << sdcn << endl;

      cout << __func__ << " sdcn " << sdcn << endl;
      cout << __func__ << " sdcn2 " << sdcn2 << endl;

    }

    // We add the hit object to the framework strawHit collection created in produce

    // Which process caused this step to end?
    ProcessCode endCode(_processInfo->
                        findAndCount(Mu2eG4UserHelpers::findStepStoppingProcessName(aStep)));


    _collection->push_back( StepPointMC(_spHelper->particlePtr(aStep->GetTrack()),
                                        sdcn,
                                        edep,
                                        aStep->GetNonIonizingEnergyDeposit(),
                                        aStep->GetPreStepPoint()->GetGlobalTime(),
                                        aStep->GetPreStepPoint()->GetProperTime(),
                                        prePosTracker,
                                        preMomWorld,
                                        step,
                                        endCode
                                        ));

    if (_verbosityLevel>3) {

      // checking if the Geant4 and Geometry Service straw positions agree

      art::ServiceHandle<GeometryService> geom;
      GeomHandle<TTracker> ttracker;
      const Straw&  straw = ttracker->getStraw( StrawIndex(sdcn) );
      // const Straw& straw = ttracker->straw(StrawIndex2(sdcn2));

      // will compare straw.getMidPoint() with the straw position according to Geant4

      G4AffineTransform const& toLocal = touchableHandle->GetHistory()->GetTopTransform();
      G4AffineTransform        toWorld = toLocal.Inverse();

      // get the position of the straw in the world and tracker coordinates

      G4ThreeVector strawInWorld   = toWorld.TransformPoint(G4ThreeVector());
      G4ThreeVector strawInTracker = strawInWorld - detectorOrigin;

      G4double diffMag = (straw.getMidPoint() - strawInTracker).mag();
      const G4double tolerance = 1.e-10;

      if ( _verbosityLevel>4 || diffMag>tolerance) {

        const Plane& plane = ttracker->getPlane(straw.id().getPlane());
        const Panel& panel = plane.getPanel(straw.id().getPanel());

        cout << __func__ << " straw info: event track panel plane straw id: " <<
          setw(4) << en << " " <<
          setw(4) << ti << " " <<
          setw(4) << straw.id().getPanel() << " " <<
          setw(4) << straw.id().getPlane() << " " <<
          setw(6) << sdcn << " " <<
          setw(6) << sdcn2 << " " <<
          straw.id() << endl;

        cout << __func__ << " straw pos     "
             << en << " "
             << ti << " "       <<
          " sdcn: "             << sdcn <<
          " sdcn2: "             << sdcn2 <<
          ", straw.MidPoint "   << straw.getMidPoint() <<
          //          ", panel.boxOffset " << panel.boxOffset() <<
          ", plane.origin "    << plane.origin() <<
          ", panel.boxRzAngle " << panel.boxRzAngle()/M_PI*180. <<
          ", plane.rotation "  << plane.rotation() <<
          endl;

        cout << __func__ << " straw pos G4  "
             << en << " "
             << ti << " "       <<
          " sdcn: "             << sdcn <<
          " sdcn2: "             << sdcn2 <<
          ", straw.MidPoint "   << strawInTracker <<
          ", diff magnitude "   << scientific << diffMag  << fixed <<
          endl;

        if (diffMag>tolerance) {
          throw cet::exception("GEOM")
            << "Inconsistent Straw Positions; incorrect ttracker Geant4 construction?" << endl;
        }
      }
    }

    // Some debugging tests.
    if ( !_debugList.inList() ) return true;

    // Transformations between world to local coordinate systems.
    G4AffineTransform const& toLocal = touchableHandle->GetHistory()->GetTopTransform();
    G4AffineTransform        toWorld = toLocal.Inverse();

    G4ThreeVector postPosWorld = aStep->GetPostStepPoint()->GetPosition();
    G4ThreeVector postPosLocal = toLocal.TransformPoint(postPosWorld);

    G4ThreeVector prePosLocal  = toLocal.TransformPoint(prePosWorld);

    G4ThreeVector preMomLocal = toLocal.TransformAxis(preMomWorld);

    // Compute the directed chord in both local and world coordinates.
    G4ThreeVector deltaWorld = postPosWorld - prePosWorld;
    G4ThreeVector deltaLocal = postPosLocal - prePosLocal;

    // Angle between the directed chord and the momentum.
    G4ThreeVector dT( deltaWorld.x(), deltaWorld.y(), 0.);
    G4ThreeVector pT( preMomWorld.x(), preMomWorld.y(), 0. );
    //G4double angle = dT.angle(pT);

    G4ThreeVector dTLocal( deltaLocal.x(), deltaLocal.y(), 0.);
    G4ThreeVector pTLocal( preMomLocal.x(), preMomLocal.y(), 0. );
    //G4double angleLocal = dTLocal.angle(pTLocal);

    // This is too big. O(1.e-5 CLHEP::radians) or about 1% of the value. Why?
    //G4double diffAngle = angle-angleLocal;

    G4ThreeVector localOrigin(0.,0.,0.);
    G4ThreeVector worldOrigin = toWorld.TransformPoint(localOrigin) - detectorOrigin;

    G4ThreeVector localZUnit(0.,0.,1.);
    G4ThreeVector worldZUnit = toWorld.TransformAxis(localZUnit);

    // make sure it works with the constructTTrackerv3
    //    int copy = touchableHandle->GetCopyNumber();
    int copy = sdcn2;
    /*
    int eventNo = event->GetEventID();
    // Works for both TTracker.
    printf ( "Addhit: %4d %4d %6d %3d %3d | %10.2f %10.2f %10.2f | %10.2f %10.2f %10.2f | %10.7f %10.7f\n",
    eventNo,  _collection->size(), copy,
    aStep->IsFirstStepInVolume(), aStep->IsLastStepInVolume(),
    prePosTracker.x(), prePosTracker.y(), prePosTracker.z(),
    preMomWorld.x(),   preMomWorld.y(),   preMomWorld.z(),
    prePosLocal.perp(),  postPosLocal.perp()  );
    fflush(stdout);
    */

    // Reconstruction Geometry for the TTracker.
    art::ServiceHandle<GeometryService> geom;
    if ( geom->hasElement<TTracker>() ) {

      GeomHandle<TTracker> ttracker;
      Straw const& straw = ttracker->getStraw( StrawIndex(copy) );
      // const Straw& straw = ttracker->straw(StrawIndex2(copy));
      G4ThreeVector mid  = straw.getMidPoint();
      G4ThreeVector w    = straw.getDirection();

      // Point of closest approach of track to wire.
      // Straight line approximation.
      TwoLinePCA pca( mid, w, prePosTracker, preMomWorld);

      // Check that the radius of the reference point in the local
      // coordinates of the straw.  Should be 2.5 mm.
      double s = w.dot(prePosTracker-mid);
      CLHEP::Hep3Vector point = prePosTracker - (mid + s*w);

      //      this works with transporOnly.py
      //      StrawDetail const& strawDetail = straw.getDetail();
      //       if (pca.dca()>strawDetail.outerRadius() || abs(point.mag()-strawDetail.outerRadius())>1.e-6 ) {

      //         cerr << "*** Bad hit?: eid "
      //              << event->GetEventID()    << ", cs "
      //              << _collection->GetSize() << " tid "
      //              << newHit->trackId()      << " vid "
      //              << newHit->volumeId()     << " "
      //              << straw.id()         << " | "
      //              << pca.dca()          << " "
      //              << prePosTracker      << " "
      //              << preMomWorld        << " "
      //              << point.mag()        << " "
      //              << newHit->eDep()     << " "
      //              << s                  << " | "
      //              << mid
      //              << endl;
      //       }

    }

    // End of debug section.

    //    newHit->Print();
    //    newHit->Draw();

    return true;

  }


  // The previous version of this code assumed that the tracker was centered in its mother.
  // That is no longer true.
  G4ThreeVector StrawSD::GetTrackerOrigin() {

    art::ServiceHandle<GeometryService> geom;
    if ( geom->hasElement<TTracker>() ) {
      GeomHandle<DetectorSystem> det;
      CLHEP::Hep3Vector val = det->toMu2e(CLHEP::Hep3Vector());
      if (_verbosityLevel>1) {
        cout << __func__ << " Detector origin used for making straw hits is: " << val << endl;
      }
      return val;
    }

    throw cet::exception("GEOM")
      << "StrawSD::GetTrackerOrigin this version only supports the TTracker.\n";
  }

} //namespace mu2e
