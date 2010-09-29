#ifndef CaloCrystalSD_h
#define CaloCrystalSD_h 1
//
// Define a sensitive detector for virtual detectors (like G4Beamline)
// 
// Original author Ivan Logashenko
//
#include <map>
#include <vector>

// Mu2e includes
#include "Mu2eG4/inc/StepPointG4.hh"
#include "Mu2eG4/inc/EventNumberList.hh"
#include "Mu2eUtilities/inc/SimpleConfig.hh"

// G4 includes
#include "G4VSensitiveDetector.hh"

class G4Step;
class G4HCofThisEvent;

namespace mu2e {

  // Forward declarations in mu2e namespace
  class SimpleConfig;

  // Utility class to hold intermediate data

  class ReadoutHitG4 {
  public:
    int _idro;
    double _edep;
    double _edepMC;
    double _t1;
    double _t2;
    int _ncharge;
    ReadoutHitG4(int idro, double edep, double edepMC, double t1, double t2, int ncharge) :
      _idro(idro), _edep(edep),_edepMC(edepMC), _t1(t1), _t2(t2), _ncharge(ncharge) { }
  };

  typedef std::map<int, std::vector<ReadoutHitG4> > ROHitsCollection;

  class CaloCrystalSD : public G4VSensitiveDetector{

  public:
    CaloCrystalSD(G4String, const SimpleConfig& config);
    ~CaloCrystalSD();
    
    void Initialize(G4HCofThisEvent*);
    G4bool ProcessHits(G4Step*, G4TouchableHistory*);
    void EndOfEvent(G4HCofThisEvent*);

    void AddReadoutHit(G4Step*,int,double,double);

    static void setMu2eOriginInWorld(const G4ThreeVector &origin) {
      _mu2eOrigin = origin;
    }

  private:

    StepPointG4Collection* _collection;

    // List of events for which to enable debug printout.
    EventNumberList _debugList;

    // Mu2e point of origin
    static G4ThreeVector _mu2eOrigin;

    // Limit maximum size of the steps collection
    int _sizeLimit;
    int _currentSize;
  };

} // namespace mu2e

#endif
