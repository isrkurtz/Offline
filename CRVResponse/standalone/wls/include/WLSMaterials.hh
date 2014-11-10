#ifndef WLSMaterials_h
#define WLSMaterials_h 1

#include "globals.hh"
#include "G4Material.hh"
#include "G4NistManager.hh"

class WLSMaterials
{
  public:

    ~WLSMaterials();
 
    static WLSMaterials* GetInstance();

    G4Material* GetMaterial(const G4String);
 
  private:
 
    WLSMaterials();

    void CreateMaterials();

  private:

    static WLSMaterials* instance;

    G4NistManager*     nistMan;

    G4Material*        Air;

    G4Material*        PMMA;
    G4Material*        Pethylene;
    G4Material*        FPethylene;
    G4Material*        Polystyrene;
    G4Material*        Silicone;
    G4Material*        Coating;

};

#endif /*WLSMaterials_h*/
