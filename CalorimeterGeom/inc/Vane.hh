#ifndef VANE_HH
#define VANE_HH

//
// Hold information about one vane in the calorimter.
//

//
// $Id: Vane.hh,v 1.4 2010/09/29 19:37:58 logash Exp $
// $Author: logash $
// $Date: 2010/09/29 19:37:58 $
//
// Original author R, Bernstein and Rob Kutschke
//

#include <vector>
//#include <iostream> 

#include "CLHEP/Vector/Rotation.h"
#include "CLHEP/Vector/ThreeVector.h"

namespace mu2e {

    class Vane{

      friend class Calorimeter;
      friend class CalorimeterMaker;

    public:

      Vane():_id(-1){}
      Vane( int & id ):_id(id){}
      ~Vane(){}
 
      // Compiler generated copy and assignment constructors
      // should be OK.
  
      int Id() const { return _id;}

      // Get position in the global Mu2e frame
      CLHEP::Hep3Vector const& getOrigin() const { return _origin; }
      CLHEP::Hep3Vector const& getOriginLocal() const { return _originLocal; }
      CLHEP::Hep3Vector const& getSize() const { return _size; }
      CLHEP::HepRotation * getRotation() const { 
	return const_cast<CLHEP::HepRotation *>(&_rotation); 
      }

    protected:

      int _id;
      CLHEP::Hep3Vector _origin;
      CLHEP::Hep3Vector _originLocal;
      CLHEP::Hep3Vector _size;
      CLHEP::HepRotation _rotation;
      
    };

} //namespace mu2e

#endif
