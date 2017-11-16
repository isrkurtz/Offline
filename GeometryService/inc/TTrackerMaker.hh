#ifndef GeometryService_TTrackerMaker_hh
#define GeometryService_TTrackerMaker_hh
//
// Construct and return a TTracker.
//
// Original author Rob Kutschke
//

#include <memory>
#include <string>
#include <vector>

#include "TTrackerGeom/inc/TTracker.hh"

#include "TrackerGeom/inc/Plane.hh"
#include "TrackerGeom/inc/Layer.hh"
#include "TrackerGeom/inc/Panel.hh"

#include "TTrackerGeom/inc/Station.hh"

#include "CLHEP/Vector/ThreeVector.h"

namespace mu2e {

  class SimpleConfig;
  class TTracker;

  class TTrackerMaker{

  public:

    TTrackerMaker( SimpleConfig const& config );

    // Use compiler-generated copy c'tor, copy assignment, and d'tor

    std::unique_ptr<TTracker> getTTrackerPtr() { return std::move(_tt); }

  private:

    // Extract info from the config file.
    void parseConfig( const SimpleConfig& config );

    // Fill the details of the different straw types.
    void makeDetails();

    void makeMother();
    void makePlane( PlaneId planeId );
    void makePanel( const PanelId& panelId, Plane& plane );
    void makeLayer ( const LayerId& layId,  Panel& panel );
    void makeManifolds( const PanelId& panelId);

    void computeStrawHalfLengths();
    void computePanelBoxParams(Panel& panel, Plane& plane);
    void computeConstantPanelBoxParams();
    void computeLayerSpacingAndShift();
    void computeManifoldEdgeExcessSpace();
    void computeTrackerEnvelope();
    void computePlaneEnvelope();

    void identifyNeighbourStraws();
    void identifyDirectionalNeighbourStraws();
    StrawIndex ttStrawIndex (LayerId const &layerId, int snum);

    void makeStation( StationId stationId );

    // Do the work of constructing it.
    void buildIt();

    double choosePlaneSpacing( int iplane ) const;
    double findFirstPlaneZ0() const;
    double panelRotation(int ipanel,int iplane) const;
    double panelZSide(int ipanel, int iplane) const;

    // Some functions for the, fully detailed support structure.
    void makeSupportStructure();
    void makeStrawTubs();
    void makeThinSupportRings();

    // An ugly hack for the detailed support structure; must be called after all
    // straws have been created.
    void recomputeHalfLengths();

    // Some final self-consistency checks.
    void finalCheck();

    int    _verbosityLevel;
    int    _ttVersion;

    // Properties of the mother volume.
    double _motherRIn;               // Inner radius of the mother volume
    double _motherROut;              // Outer radius of the mother volume
    double _motherHalfLength;        // Half length, in z, of the mother
    double _motherZ0;                // Mother volume is centered on the DS axis; the z of the center is NOT
                                     // necessarily the same as the center of the instrumented region of the tracker.

    // Basic parameters needed to describe the TTracker.
    int    _numPlanes;                  // Number of planes.
    int    _panelsPerPlane;            // Number of panels in one plane.
    int    _layersPerPanel;             // Number of layers in one panel.
    int    _manifoldsPerEnd;             // Number of manifolds along one end of the wires in a layer.
    int    _strawsPerManifold;           // Number of straws connected to each manifold.
    int    _rotationPattern;             // Pattern of rotations from plane to plane.
    int    _panelZPattern;               // Pattern of rotations from plane to plane.
    int    _layerZPattern;               // If 0 then layer 0 is closest to base plane; if 1 then layer 1 is closes to base plane.
    int    _spacingPattern;              // Pattern of spacing from plane to plane.
    int    _innermostLayer;              // Which of layers 0, 1 has the more innermost wire.
    double _oddStationRotation;           // rotation of odd stations relative to even
    double _zCenter;                     // z position of the center of the tracker, in the Mu2e coord system.
    double _xCenter;                     // x position of the center of the tracker, in the Mu2e coord system.
    double _envelopeInnerRadius;         // Inner radius of inside of innermost straw.
    double _rInnermostWire;              // Radius from center to midpoint of the innermost wire, in tracker coord system.
    double _strawOuterRadius;            // Radius of each straw.
    double _strawWallThickness;          // Thickness of each straw.
    double _strawGap;                    // Gap between straws.
    double _planeSpacing;               // Z-separation between adjacent stations.
    double _planeHalfSeparation;        // Z-separation between adjacent planes

    double _planePadding;         // Small spaces around panel and plane to 
    double _panelPadding;         // allow for misalignment (and more 
    // realistically describe how both are built.

    double _innerSupportRadius;          // Inner radius of support frame.
    double _outerSupportRadius;          // Outer radius of support frame.
    double _supportHalfThickness;        // Thickness of support frame.
    double _wireRadius;                  // Wire radius
    double _manifoldYOffset;             // Offset of manifold from inner edge of support.
    double _passivationMargin;           // Deadened region near the end of the straw.
    double _wallOuterMetalThickness;     // Metal on the outer surface of the straw
    double _wallInnerMetal1Thickness;    // Metal on the inner surface of the straw
    double _wallInnerMetal2Thickness;    // Thinner metal layer between Metal1 and the gas
    double _wirePlateThickness;          // Plating on the top of the wire.
    double _virtualDetectorHalfLength;   // Half length (or thickness) of virtual detectors - needed to leave space for them!

    std::string _envelopeMaterial;        // Material for the envelope volume.
    std::string _supportMaterial;         // Material for the support volume.
    std::string _wallOuterMetalMaterial;  // Material on the outer surface of the straw
    std::string _wallInnerMetal1Material; // Material on the inner surface of the straw
    std::string _wallInnerMetal2Material; // Material between Meta1 and the gas.
    std::string _wirePlateMaterial;       // Material for plating on the wire.

    std::vector<double> _manifoldHalfLengths; // Dimensions of each manifold.
    std::vector<std::string> _strawMaterials; // Names of the materials.

    // Base rotations of a panel; does not include plane rotation.
    std::vector<double> _panelBaseRotations;
    std::vector<double> _panelZSide;
    double _planerot; // hack to make redundant information self-consistent

    std::unique_ptr<TTracker> _tt;

    // Derived parameters.

    // Compute at start and compare against final result.
    int _nStrawsToReserve;

    // Lengths of straws indexed by manifold, from innermost radius, outwards.
    std::vector<double> _strawHalfLengths;

    // Same for the active length of the straw.
    // This is only valid for SupportModel==simple
    std::vector<double> _strawActiveHalfLengths;

    // panel box half lengths
    std::vector<double> _panelBoxHalfLengths;

    // panel box offset magnitudes
    double _panelBoxXOffsetMag;
    double _panelBoxZOffsetMag;

    // distance between layers (in Z)
    double _layerHalfSpacing;
    // relative layer shift (in X)
    double _layerHalfShift;

    // Space between first/last straw and edge of manifold
    double _manifoldXEdgeExcessSpace;
    double _manifoldZEdgeExcessSpace;

    // Z Location of the first plane.
    double _z0;

    int    _numStations;                  // Number of Stations.
    int    _planesPerStation;

    // The detailed description of the complete support structure
    SupportModel _supportModel;
    double       _endRingOuterRadius;
    double       _endRingInnerRadius;
    double       _endRingHalfLength;
    double       _endRingZOffset;
    std::string  _endRingMaterial;
    bool         _hasDownRing;
    double       _downRingOuterRadius;
    double       _downRingInnerRadius;
    double       _downRingHalfLength;
    double       _downRingZOffset;
    std::string  _downRingMaterial;

    std::vector<int> _midRingSlot;
    double           _midRingHalfLength;
    double           _midRingPhi0;
    double           _midRingdPhi;
    std::string      _midRingMaterial;

    //  These two for all panels
    double         _panelPhi;
    double         _dphiRibs;

    double      _innerRingInnerRadius;
    double      _innerRingOuterRadius;
    double      _innerRingHalfLength;
    std::string _innerRingMaterial;

    double      _centerPlateHalfLength;
    std::string _centerPlateMaterial;

    double      _outerRingInnerRadius;
    double      _outerRingOuterRadius;
    std::string _outerRingMaterial;

    double      _coverHalfLength;
    std::string _coverMaterial;

    double      _electronicsG10HalfLength;
    std::string _electronicsG10Material;

    double      _electronicsCuHhalfLength;
    std::string _electronicsCuMaterial;

    double      _channelZOffset;
    double      _channelDepth;
    std::string _channelMaterial;
    double      _panelZOffset;  // used from version 5 on

    std::string _electronicsSpaceMaterial;

    std::vector<double> _beam0_phiRange;
    double _beam0_innerRadius;
    double _beam0_outerRadius;
    std::string _beam0_material;

    std::vector<double> _beam1_phiRange;
    std::vector<double> _beam1_phiSpans;
    std::vector<double> _beam1_servicePhi0s;
    std::vector<double> _beam1_servicePhiEnds;
    double _beam1_innerRadius;
    double _beam1_midRadius1;
    std::vector<double> _beam1_serviceOuterRadii;
    double _beam1_midRadius2;
    double _beam1_outerRadius;
    std::string _beam1_material;
    std::vector<std::string> _beam1_serviceMaterials;
    std::vector<double> _beam1_serviceCovRelThickness;
    std::vector<std::string> _beam1_serviceMaterialsCov;

    // electronic board aka key 
    // connected to each of the panels

    double _EBKeyHalfLength;
    double _EBKeyShieldHalfLength;
    double _EBKeyInnerRadius;
    double _EBKeyOuterRadius;
    double _EBKeyShiftFromPanelFace;
    bool   _EBKeyVisible;
    bool   _EBKeySolid;
    bool   _EBKeyShieldVisible;
    bool   _EBKeyShieldSolid;
    std::string _EBKeyMaterial;
    std::string _EBKeyShieldMaterial;
    double _EBKeyPhiRange;
    double _EBKeyPhiExtraRotation;

    std::vector<int>  _nonExistingPlanes;

  };

}  //namespace mu2e

#endif /* GeometryService_TTrackerMaker_hh */
