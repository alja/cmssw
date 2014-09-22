#include "Fireworks/Geometry/interface/FWTGeoRecoGeometryESProducer.h"
#include "Fireworks/Geometry/interface/FWTGeoRecoGeometry.h"
#include "Fireworks/Geometry/interface/FWTGeoRecoGeometryRecord.h"

#include "DataFormats/GeometrySurface/interface/RectangularPlaneBounds.h"
#include "DataFormats/GeometrySurface/interface/TrapezoidalPlaneBounds.h"
#include "DataFormats/SiPixelDetId/interface/PXBDetId.h"
#include "DataFormats/SiPixelDetId/interface/PXFDetId.h"
#include "DataFormats/SiStripDetId/interface/TIBDetId.h"
#include "DataFormats/SiStripDetId/interface/TIDDetId.h"
#include "DataFormats/SiStripDetId/interface/TOBDetId.h"
#include "DataFormats/SiStripDetId/interface/TECDetId.h"
#include "DataFormats/MuonDetId/interface/RPCDetId.h"
#include "DataFormats/MuonDetId/interface/GEMDetId.h"
#include "DataFormats/EcalDetId/interface/EcalSubdetector.h"
#include "DataFormats/HcalDetId/interface/HcalSubdetector.h"
#include "DataFormats/EcalDetId/interface/EBDetId.h"
#include "DataFormats/EcalDetId/interface/EEDetId.h"
#include "DataFormats/HcalDetId/interface/HcalDetId.h"

#include "Geometry/CommonDetUnit/interface/GlobalTrackingGeometry.h"
#include "Geometry/CaloGeometry/interface/CaloGeometry.h"
#include "Geometry/CaloGeometry/interface/CaloCellGeometry.h"
#include "Geometry/CSCGeometry/interface/CSCGeometry.h"
#include "Geometry/DTGeometry/interface/DTGeometry.h"
#include "Geometry/CSCGeometry/interface/CSCChamber.h"
#include "Geometry/CSCGeometry/interface/CSCLayer.h"
#include "Geometry/DTGeometry/interface/DTChamber.h"
#include "Geometry/DTGeometry/interface/DTLayer.h"
#include "Geometry/RPCGeometry/interface/RPCGeometry.h"
#include "Geometry/GEMGeometry/interface/GEMEtaPartition.h"
#include "Geometry/GEMGeometry/interface/GEMGeometry.h"
#include "Geometry/Records/interface/CaloGeometryRecord.h"
#include "Geometry/TrackerGeometryBuilder/interface/TrackerGeometry.h"
#include "Geometry/TrackerGeometryBuilder/interface/RectangularPixelTopology.h"
#include "Geometry/TrackerGeometryBuilder/interface/PixelGeomDetUnit.h"
#include "Geometry/TrackerGeometryBuilder/interface/StripGeomDetUnit.h"
#include "Geometry/TrackerGeometryBuilder/interface/StripGeomDetType.h"
#include "Geometry/CommonTopologies/interface/StripTopology.h"
#include "Geometry/CommonTopologies/interface/RectangularStripTopology.h"
#include "Geometry/CommonTopologies/interface/TrapezoidalStripTopology.h"
#include "Geometry/CaloGeometry/interface/IdealZPrism.h"
#include "Geometry/CaloGeometry/interface/IdealObliquePrism.h"
#include "Geometry/CaloGeometry/interface/TruncatedPyramid.h"

#include "TGeoManager.h"
#include "TGeoArb8.h"
#include "TGeoMatrix.h"

#include "TFile.h"
#include "TTree.h"
#include "TError.h"
#include "TMath.h"

#include "TEveVector.h"
#include "TEveTrans.h"


//std::map< const CaloCellGeometry::CCGFloat*, TGeoVolume*> caloShapeMap;
namespace {
typedef std::map< const float*, TGeoVolume*> CaloVolMap;
}
FWTGeoRecoGeometryESProducer::FWTGeoRecoGeometryESProducer( const edm::ParameterSet& /*pset*/ ):
m_dummyMedium(0)
{
  setWhatProduced( this );
}

FWTGeoRecoGeometryESProducer::~FWTGeoRecoGeometryESProducer( void )
{}

namespace
{

void AddLeafNode(TGeoVolume* mother, TGeoVolume* daughter, const char* name, TGeoMatrix* mtx , unsigned int rawid)
{
   int n = mother->GetNdaughters();
   mother->AddNode(daughter, 1, mtx);
   mother->GetNode(n)->SetName(name);
   mother->GetNode(n)->SetTitle(Form("%d", rawid));
}

  /** Create TGeo transformation of GeomDet */
TGeoCombiTrans* createPlacement( const GeomDet *det )
{
   // Position of the DetUnit's center
   float posx = det->surface().position().x();
   float posy = det->surface().position().y();
   float posz = det->surface().position().z();

   TGeoTranslation trans( posx, posy, posz );

   // Add the coeff of the rotation matrix
   // with a projection on the basis vectors
   TkRotation<float> detRot = det->surface().rotation();

   TGeoRotation rotation;
   const Double_t matrix[9] = { detRot.xx(), detRot.yx(), detRot.zx(),
                                detRot.xy(), detRot.yy(), detRot.zy(),
                                detRot.xz(), detRot.yz(), detRot.zz() 
   };
   rotation.SetMatrix( matrix );
     
   return new TGeoCombiTrans( trans, rotation );
}


}
//______________________________________________________________________________


TGeoVolume* FWTGeoRecoGeometryESProducer::GetDaughter(TGeoVolume* mother, const char* prefix, ERecoDet cidx, int id)
{
   TGeoVolume* res = 0;
   if (mother->GetNdaughters()) { 
      TGeoNode* n = mother->FindNode(Form("%s_%d_1", prefix, id));
      if ( n ) res = n->GetVolume();
   }

   if (!res) {
      res = new TGeoVolumeAssembly( Form("%s_%d", prefix, id ));
      res->SetMedium(GetMedium(cidx));
      mother->AddNode(res, 1);
   }

   return res;
}

TGeoVolume* FWTGeoRecoGeometryESProducer::GetDaughter(TGeoVolume* mother, const char* prefix, ERecoDet cidx)
{
   TGeoVolume* res = 0;
   if (mother->GetNdaughters()) { 
      TGeoNode* n = mother->FindNode(Form("%s_1",prefix));
      if ( n ) res = n->GetVolume();
   }

   if (!res) {
      //      printf("GetDau... new holder %s for mother %s \n", mother->GetName(), prefix);
      res = new TGeoVolumeAssembly(prefix);
      res->SetMedium(GetMedium(cidx));
      mother->AddNode(res, 1);
   }

   return res;
}

TGeoVolume* FWTGeoRecoGeometryESProducer::GetTopHolder(const char* prefix, ERecoDet cidx)
{
   //   printf("GetTopHolder res = %s \n", prefix);
   TGeoVolume* res =  GetDaughter(gGeoManager->GetTopVolume(), prefix, cidx);
   return res;
}


TGeoMedium*
FWTGeoRecoGeometryESProducer::GetMedium(ERecoDet det)
{
   std::map<ERecoDet, TGeoMedium*>::iterator it = m_recoMedium.find(det);
   if (it != m_recoMedium.end())
      return it->second;

   std::string name;
   int color;
   // see TGeoManager::DefaultColors()
   switch (det)
   {
      case kSiPixel:
         name = "SiPixel";
         color = 4;//kGreen-10
         break;
      case kSiStrip:
         name = "SiStrip";
         color = 29; // kOrange+9
         break;
      case kMuonDT:
         name = "MuonDT";
         color = 13; // kBlue -10
         break;
      case kMuonGEM:
         name = "MuonGEM";
         color = 16; //kYellow+1
         break;
      case kMuonRPC:
         name = "MuonRPC";
         color = 10; //kRed-10
         break;
      case kMuonCSC:
         name = "MuonCSC"; // kMagenta
         color = 9;
         break;
      case kECal:
         name = "ECal";
         color = 6;
         break;
      case kHCal:
         name = "HCal";    
         color = 10; // kRed-10
         break;
      case kHGCE:
         name = "HGCEE";
         color = 10;
         break;
      case kHGCH:
         name = "HGCEH";
         color = 6; // kBlue -7
         break;
      default:
         printf("invalid medium id \n");
         return m_dummyMedium;
   }

   TGeoMaterial* mat = new TGeoMaterial(name.c_str(), 0, 0, 0);
   mat->SetZ(color);
   m_recoMedium[det] = new TGeoMedium(name.c_str(), 0, mat);
   mat->SetFillStyle(3000); // tansparency 3000-3100
   mat->SetDensity(1); // disable override of transparency in TGeoManager::DefaultColors()

   return m_recoMedium[det];
}


//______________________________________________________________________________



boost::shared_ptr<FWTGeoRecoGeometry> 
FWTGeoRecoGeometryESProducer::produce( const FWTGeoRecoGeometryRecord& record )
{
   using namespace edm;

   m_fwGeometry = boost::shared_ptr<FWTGeoRecoGeometry>( new FWTGeoRecoGeometry );
   record.getRecord<GlobalTrackingGeometryRecord>().get( m_geomRecord );
  
   DetId detId( DetId::Tracker, 0 );
   m_trackerGeom = (const TrackerGeometry*) m_geomRecord->slaveGeometry( detId );
  
   record.getRecord<CaloGeometryRecord>().get( m_caloGeom );

   TGeoManager* geom = new TGeoManager( "cmsGeo", "CMS Detector" );
   if( 0 == gGeoIdentity )
   {
      gGeoIdentity = new TGeoIdentity( "Identity" );
   }

   m_fwGeometry->manager( geom );
  
   // Default material is Vacuum
   TGeoMaterial *vacuum = new TGeoMaterial( "Vacuum", 0 ,0 ,0 );
   m_dummyMedium = new TGeoMedium( "reco", 0, vacuum);


   TGeoVolume *top = geom->MakeBox( "CMS", m_dummyMedium, 270., 270., 120. );
  

   if( 0 == top )
   {
      return boost::shared_ptr<FWTGeoRecoGeometry>();
   }
   geom->SetTopVolume( top );
   // ROOT chokes unless colors are assigned
   top->SetVisibility( kFALSE );
   top->SetLineColor( kBlue );
   
   addPixelBarrelGeometry();

   addPixelForwardGeometry();
   addTIBGeometry();
   addTIDGeometry();
   addTOBGeometry();
   addTECGeometry();
   addDTGeometry();
   addCSCGeometry();
   addRPCGeometry();
   addGEMGeometry();

 
   if (0) {
   addEcalCaloGeometry();
   
   addHcalCaloGeometryBarrel();
   addHcalCaloGeometryEndcap();
   }

   geom->CloseGeometry();
   geom->DefaultColors();
   geom->CloseGeometry();

  printf("AMT tracker topologies %lu volumes %d nodes %d \n",  m_topology.size(),
          gGeoManager->GetListOfShapes()->GetLast(), gGeoManager->GetNNodes());

   for (std::map<DMtopo, DMids>::iterator t =  m_topology.begin(); t != m_topology.end(); ++t)
       printf("num raws = %lu \n", t->second.size());

   return m_fwGeometry;
}

/** Create TGeo shape for GeomDet */
TGeoShape* 
FWTGeoRecoGeometryESProducer::createShape( const GeomDet *det )
{
  TGeoShape* shape = 0;

  // Trapezoidal
  const Bounds *b = &((det->surface ()).bounds ());
  const TrapezoidalPlaneBounds *b2 = dynamic_cast<const TrapezoidalPlaneBounds *> (b);
  if( b2 )
  {
      std::array< const float, 4 > const & par = b2->parameters ();
    
    // These parameters are half-lengths, as in CMSIM/GEANT3
    float hBottomEdge = par [0];
    float hTopEdge    = par [1];
    float thickness   = par [2];
    float apothem     = par [3];

    std::stringstream s;
    s << "T_"
      << hBottomEdge << "_"
      << hTopEdge << "_"
      << thickness << "_"
      << apothem;
    std::string name = s.str();

    // Do not create identical shape,
    // if one already exists
    shape = m_nameToShape[name];
    if( 0 == shape )
    {
      shape = new TGeoTrap(
	name.c_str(),
	thickness,  //dz
	0, 	    //theta
	0, 	    //phi
	apothem,    //dy1
	hBottomEdge,//dx1
	hTopEdge,   //dx2
	0, 	    //alpha1
	apothem,    //dy2
	hBottomEdge,//dx3
	hTopEdge,   //dx4
	0);         //alpha2

      m_nameToShape[name] = shape;
    }
  }
  if( dynamic_cast<const RectangularPlaneBounds *> (b) != 0 )
  {
    // Rectangular
    float length = det->surface().bounds().length();
    float width = det->surface().bounds ().width();
    float thickness = det->surface().bounds().thickness();

    std::stringstream s;
    s << "R_"
      << width << "_"
      << length << "_"
      << thickness;
    std::string name = s.str();

    // Do not create identical shape,
    // if one already exists
    shape = m_nameToShape[name];
    if( 0 == shape )
    {
      shape = new TGeoBBox( name.c_str(), width / 2., length / 2., thickness / 2. ); // dx, dy, dz

      m_nameToShape[name] = shape;
    }
  }
  
  return shape;
}

/** Create TGeo volume for GeomDet */
TGeoVolume* 
FWTGeoRecoGeometryESProducer::createVolume( const std::string& name, const GeomDet *det, ERecoDet mid )
{
   TGeoShape* solid = createShape( det );

   std::map<TGeoShape*, TGeoVolume*>::iterator vIt = m_shapeToVolume.find(solid);
   if (vIt != m_shapeToVolume.end()) return  vIt->second;
   

   TGeoVolume* volume = new TGeoVolume( name.c_str(),solid, GetMedium(mid));

   m_shapeToVolume[solid] = volume;

   return volume;
}



///////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////


//______________________________________________________________________________
void
FWTGeoRecoGeometryESProducer::addPixelTopology(DetId& detid)
{

  const PixelGeomDetUnit* det = dynamic_cast<const PixelGeomDetUnit*>(  m_trackerGeom->idToDetUnit( detid )); 
  if( det )							
  {      							
    const PixelTopology* topo = &det->specificTopology();
    DMtopo par;
    par.push_back(topo->nrows());
    par.push_back(topo->ncolumns());
    m_topology[par].push_back(detid.rawId());
  }
}

//______________________________________________________________________________
void
FWTGeoRecoGeometryESProducer::addStripTopology(DetId& detid)
{
    const StripGeomDetUnit* det = dynamic_cast<const StripGeomDetUnit*>( m_trackerGeom->idToDetUnit( detid )); 
    if( det )                                                     
    {     
        DMtopo par;                                                        
        const StripTopology* topo = dynamic_cast<const StripTopology*>( &det->specificTopology() );
        par.push_back(0);
        par.push_back(topo->nstrips());
        par.push_back(topo->stripLength());
        if( const RadialStripTopology* rtop = dynamic_cast<const RadialStripTopology*>( &(det->specificType().specificTopology()) ) )
        {									
            par[0] = 1;
            par.push_back(rtop->yAxisOrientation());
            par.push_back(rtop->originToIntersection());
            par.push_back(rtop->phiOfOneEdge());
            par.push_back(rtop->angularWidth());
        }                                                                   
        else if( dynamic_cast<const RectangularStripTopology*>( &(det->specificType().specificTopology()) ) )     
        {     
            par[0] = 2;                                                              
            par.push_back(topo->pitch());
        }									
        else if( dynamic_cast<const TrapezoidalStripTopology*>( &(det->specificType().specificTopology()) ) )     
        {     
            par[0] = 3;                                                              
            par.push_back(topo->pitch());	
        }

        m_topology[par].push_back(detid.rawId());							
    }
}

//______________________________________________________________________________
void
FWTGeoRecoGeometryESProducer::addPixelBarrelGeometry()
{
   TGeoVolume* tv =  GetTopHolder("SiPixel", kSiPixel);
   TGeoVolume *assembly = GetDaughter(tv, "PXB", kSiPixel);

   for( TrackerGeometry::DetContainer::const_iterator it = m_trackerGeom->detsPXB().begin(),
           end = m_trackerGeom->detsPXB().end();
        it != end; ++it)
   {
       DetId detid = ( *it )->geographicalId();
       unsigned int rawid = detid.rawId();

       PXBDetId xx(rawid);
       std::string name = Form("PXB Ly:%d, Md:%d Ld:%d ", xx.layer(), xx.module(), xx.layer());
       TGeoVolume* child = createVolume( name, *it, kSiPixel );

       TGeoVolume* holder  = GetDaughter(assembly, "Layer", kSiPixel, xx.layer());
       holder = GetDaughter(holder, "Module", kSiPixel, xx.module());
                                 
       AddLeafNode(holder, child, name.c_str(), createPlacement( *it ), rawid);

       addPixelTopology(detid);
   }
}
//______________________________________________________________________________


void
FWTGeoRecoGeometryESProducer::addPixelForwardGeometry()
{
   TGeoVolume* tv =  GetTopHolder("SiPixel", kSiPixel);
   TGeoVolume *assembly = GetDaughter(tv, "PXF", kSiPixel);

   for( TrackerGeometry::DetContainer::const_iterator it = m_trackerGeom->detsPXF().begin(),
           end = m_trackerGeom->detsPXF().end();
        it != end; ++it )
   {
      PXFDetId detid = ( *it )->geographicalId();
      std::stringstream s;
      s << detid;
      std::string name = s.str();

      TGeoVolume* child = createVolume( name, *it, kSiPixel );


      TGeoVolume* holder  = GetDaughter(assembly, "Side", kSiPixel, detid.side());
      holder = GetDaughter(holder, "Disk", kSiPixel, detid.disk());
      holder = GetDaughter(holder, "Blade", kSiPixel, detid.blade());
      holder = GetDaughter(holder, "Panel", kSiPixel, detid.panel());
   
      AddLeafNode(holder, child, name.c_str(),  createPlacement( *it ), detid.rawId());

      addPixelTopology(detid);
   }
}

//______________________________________________________________________________


void
FWTGeoRecoGeometryESProducer::addTIBGeometry()
{
   TGeoVolume* tv =  GetTopHolder( "SiStrip", kSiStrip);
   TGeoVolume *assembly = GetDaughter(tv,"TIB", kSiStrip);

   for( TrackerGeometry::DetContainer::const_iterator it = m_trackerGeom->detsTIB().begin(),
           end = m_trackerGeom->detsTIB().end();
        it != end; ++it )
   {
      TIBDetId detid(( *it )->geographicalId());
      std::stringstream s;
      s << detid;
      std::string name = s.str();

      TGeoVolume* child = createVolume( name, *it, kSiStrip );

      TGeoVolume* holder  = GetDaughter(assembly, "Module", kSiStrip, detid.module());
      holder = GetDaughter(holder, "Order", kSiStrip, detid.order());
      holder = GetDaughter(holder, "Side", kSiStrip, detid.side());
      AddLeafNode(holder, child, name.c_str(),  createPlacement( *it ), detid.rawId());

      addStripTopology(detid);
   }
}

//______________________________________________________________________________


void
FWTGeoRecoGeometryESProducer::addTIDGeometry()
{
   TGeoVolume* tv =  GetTopHolder( "SiStrip", kSiStrip);
   TGeoVolume *assembly = GetDaughter( tv, "TID", kSiStrip);

   for( TrackerGeometry::DetContainer::const_iterator it = m_trackerGeom->detsTID().begin(),
           end = m_trackerGeom->detsTID().end();
        it != end; ++it)
   {
      TIDDetId detid = ( *it )->geographicalId();
      std::stringstream s;
      s << detid;
      std::string name = s.str();

      TGeoVolume* child = createVolume( name, *it, kSiStrip );
      TGeoVolume* holder  = GetDaughter(assembly, "Side", kSiStrip, detid.side());
      holder = GetDaughter(holder, "Wheel", kSiStrip, detid.wheel());
      holder = GetDaughter(holder, "Ring", kSiStrip, detid.ring());
      AddLeafNode(holder, child, name.c_str(),  createPlacement( *it ), detid.rawId());

      addStripTopology(detid);
   }
}

//______________________________________________________________________________

void
FWTGeoRecoGeometryESProducer::addTOBGeometry()
{
   TGeoVolume* tv =  GetTopHolder( "SiStrip", kSiStrip);
   TGeoVolume *assembly = GetDaughter(tv, "TOB",  kSiStrip);

   for( TrackerGeometry::DetContainer::const_iterator it = m_trackerGeom->detsTOB().begin(),
           end = m_trackerGeom->detsTOB().end();
        it != end; ++it )
   {
      TOBDetId detid(( *it )->geographicalId());
      std::stringstream s;
      s << detid;
      std::string name = s.str();

      TGeoVolume* child = createVolume( name, *it, kSiStrip );
      TGeoVolume* holder  = GetDaughter(assembly, "Rod", kSiStrip, detid.rodNumber());
      holder = GetDaughter(holder, "Side", kSiStrip, detid.side());
      holder = GetDaughter(holder, "Module", kSiStrip, detid.moduleNumber());
      AddLeafNode(holder, child, name.c_str(),  createPlacement( *it ), detid.rawId());

      addStripTopology(detid);
   }

}
//______________________________________________________________________________


void
FWTGeoRecoGeometryESProducer::addTECGeometry()
{
   TGeoVolume* tv =  GetTopHolder( "SiStrip", kSiStrip);
   TGeoVolume *assembly = GetDaughter(tv, "TEC", kSiStrip);

   for( TrackerGeometry::DetContainer::const_iterator it = m_trackerGeom->detsTEC().begin(),
           end = m_trackerGeom->detsTEC().end();
        it != end; ++it )
   {
      TECDetId detid = ( *it )->geographicalId();

      std::stringstream s;
      s << detid;
      std::string name = s.str();

      TGeoVolume* child = createVolume( name, *it, kSiStrip );

      TGeoVolume* holder  = GetDaughter(assembly, "Order", kSiStrip, detid.order());
      holder = GetDaughter(holder, "Ring", kSiStrip, detid.ring());
      holder = GetDaughter(holder, "Module", kSiStrip, detid.module());
      AddLeafNode(holder, child, name.c_str(),  createPlacement( *it ), detid.rawId());

      addStripTopology(detid);
   }
}

//==============================================================================
//==============================================================================
//=================================== MUON =====================================
//==============================================================================



void
FWTGeoRecoGeometryESProducer::addDTGeometry(  )
{
   TGeoVolume* tv =  GetTopHolder("Muon", kMuonRPC);
   TGeoVolume *assemblyTop = GetDaughter(tv, "DT", kMuonDT);

   //
   // DTChambers geometry
   //
   {
      TGeoVolume *assembly = GetDaughter(assemblyTop, "DTChamber", kMuonDT);
      auto const & dtChamberGeom = m_geomRecord->slaveGeometry( DTChamberId())->dets();
      for( auto it = dtChamberGeom.begin(),
              end = dtChamberGeom.end(); 
           it != end; ++it )
      {
         if( auto chamber = dynamic_cast< const DTChamber *>(*it))
         {      
            DTChamberId detid = chamber->geographicalId();
            std::stringstream s;
            s << detid;
            std::string name = s.str();
      
            TGeoVolume* child = createVolume( name, chamber, kMuonDT );
            TGeoVolume* holder  = GetDaughter(assembly, "Wheel", kMuonDT, detid.wheel());
            holder = GetDaughter(holder, "Station", kMuonDT, detid.station());
            holder = GetDaughter(holder, "Sector", kMuonDT, detid.sector());
   
            AddLeafNode(holder, child, name.c_str(),  createPlacement( chamber), detid.rawId());
         }
      }
   }

   // Fill in DTSuperLayer parameters
   {
      TGeoVolume *assembly = GetDaughter(assemblyTop, "DTSuperLayer", kMuonDT);
      auto const & dtSuperLayerGeom = m_geomRecord->slaveGeometry( DTSuperLayerId())->dets();
      for( auto it = dtSuperLayerGeom.begin(),
              end = dtSuperLayerGeom.end(); 
           it != end; ++it )
      {
         if( auto * superlayer = dynamic_cast<const DTSuperLayer*>(*it))
         {
            DTSuperLayerId detid( DetId(superlayer->geographicalId()));
            std::stringstream s;
            s << detid;
            std::string name = s.str();
      
            TGeoVolume* child = createVolume( name, superlayer, kMuonDT );

            TGeoVolume* holder  = GetDaughter(assembly, "Wheel", kMuonDT, detid.wheel());
            holder = GetDaughter(holder, "Station", kMuonDT, detid.station());
            holder = GetDaughter(holder, "Sector", kMuonDT, detid.sector());
            holder = GetDaughter(holder, "SuperLayer", kMuonDT, detid.superlayer());
            AddLeafNode(holder, child, name.c_str(),  createPlacement( superlayer), detid.rawId());
         }
      }
   }
   // Fill in DTLayer parameters
   {
      TGeoVolume *assembly = GetDaughter(assemblyTop, "DTLayer", kMuonDT);
      auto const & dtLayerGeom = m_geomRecord->slaveGeometry( DTLayerId())->dets();
      for( auto it = dtLayerGeom.begin(),
              end = dtLayerGeom.end(); 
           it != end; ++it )
      {
         if(auto layer = dynamic_cast<const DTLayer*>(*it))
         {

            DTLayerId detid( DetId(layer->geographicalId()));

            std::stringstream s;
            s << detid;
            std::string name = s.str();
      
            TGeoVolume* child = createVolume( name, layer, kMuonDT );

            TGeoVolume* holder  = GetDaughter(assembly, "Wheel", kMuonDT, detid.wheel());
            holder = GetDaughter(holder, "Station", kMuonDT, detid.station());
            holder = GetDaughter(holder, "Sector", kMuonDT, detid.sector());
            holder = GetDaughter(holder, "SuperLayer", kMuonDT, detid.superlayer());
            holder = GetDaughter(holder, "Layer", kMuonDT, detid.layer());
            AddLeafNode(holder, child, name.c_str(),  createPlacement( layer), detid.rawId());

            const DTTopology& topo = layer->specificTopology();
            const BoundPlane& surf = layer->surface();
            DMtopo par;
            // Topology W/H/L:
            par.push_back(topo.cellWidth());
            par.push_back(topo.cellHeight());
            par.push_back(topo.cellLenght());
            par.push_back(topo.firstChannel());
            par.push_back(topo.lastChannel());
            par.push_back(topo.channels());


            // Bounds W/H/L:
            par.push_back(surf.bounds().width());
            par.push_back(surf.bounds().thickness());
            par.push_back(surf.bounds().length());

            m_topology[par].push_back(detid.rawId());
         }
      } 
   }
}

//______________________________________________________________________________

void
FWTGeoRecoGeometryESProducer::addCSCGeometry()
{   
   TGeoVolume* tv =  GetTopHolder("Muon", kMuonRPC);
   TGeoVolume *assembly = GetDaughter(tv, "CSC", kMuonCSC);

   DetId detId( DetId::Muon, 2 ); 
   const CSCGeometry* cscGeometry = (const CSCGeometry*) m_geomRecord->slaveGeometry( detId );
   for( auto it = cscGeometry->chambers().begin(),
           end = cscGeometry->chambers().end(); 
        it != end; ++it )
   {
      const CSCChamber *chamber = *it;
    
      if( chamber )
      {
         unsigned int rawid = (*it)->geographicalId();
         CSCDetId detId(rawid);
         std::stringstream s;
         s << "CSCChamber" << detId;
         std::string name = s.str();

         TGeoVolume* child = createVolume( name, chamber, kMuonCSC );
         TGeoVolume* holder  = GetDaughter(assembly, "Endcap", kMuonCSC, detId.endcap());
         holder = GetDaughter(holder, "Station", kMuonCSC, detId.station());
         holder = GetDaughter(holder, "Ring", kMuonCSC, detId.ring());
         holder = GetDaughter(holder, "Chamber", kMuonCSC , detId.chamber());
         AddLeafNode(holder, child, name.c_str(),  createPlacement(*it), detId.rawId());

         for( std::vector< const CSCLayer* >::const_iterator lit = chamber->layers().begin(),
                 lend = chamber->layers().end(); 
              lit != lend; ++lit )
         {
            const CSCLayer* layer = *lit;
            if( layer )
            {
               name += "layer ";
               name += detId.layer();
               AddLeafNode(holder, createVolume( name, layer, kMuonCSC ), name.c_str(), 
                           createPlacement(*it), layer->geographicalId());
                    
               DMtopo par;
               const CSCStripTopology* stripTopology = layer->geometry()->topology();
               par.push_back( stripTopology->yAxisOrientation());
               par.push_back(stripTopology->centreToIntersection());
               par.push_back(stripTopology->yCentreOfStripPlane());
               par.push_back( stripTopology->phiOfOneEdge());
               par.push_back(stripTopology->stripOffset());
               par.push_back(stripTopology->angularWidth());

               const CSCWireTopology* wireTopology = layer->geometry()->wireTopology();
               par.push_back(wireTopology->wireSpacing());
               par.push_back(wireTopology->wireAngle());

               m_topology[par].push_back(rawid);
            }
         }
      }
   }
}

//______________________________________________________________________________

void
FWTGeoRecoGeometryESProducer::addGEMGeometry()
{ 
    try {
        DetId detId( DetId::Muon, MuonSubdetId::GEM );
        const GEMGeometry* gemGeom = (const GEMGeometry*) m_geomRecord->slaveGeometry( detId );

        TGeoVolume* tv =  GetTopHolder("Muon", kMuonRPC);
        TGeoVolume *assembly = GetDaughter(tv, "GEM", kMuonGEM);

        for( auto it = gemGeom->etaPartitions().begin(),
                 end = gemGeom->etaPartitions().end(); 
             it != end; ++it )
        {
            const GEMEtaPartition* roll = (*it);
            if( roll )
            {
                GEMDetId detid = roll->geographicalId();
                std::stringstream s;
                s << detid;
                std::string name = s.str();
      
                TGeoVolume* child = createVolume( name, roll, kMuonGEM );

                TGeoVolume* holder  = GetDaughter(assembly, "ROLL Region", kMuonGEM , detid.region());
                holder = GetDaughter(holder, "Ring", kMuonGEM , detid.ring());
                holder = GetDaughter(holder, "Station", kMuonGEM , detid.station()); 
                holder = GetDaughter(holder, "Layer", kMuonGEM , detid.layer()); 
                holder = GetDaughter(holder, "Chamber", kMuonGEM , detid.chamber()); 

                AddLeafNode(holder, child, name.c_str(),  createPlacement(*it), detid.rawId());

                DMtopo par;
                const StripTopology& topo = roll->specificTopology();
                par.push_back(topo.nstrips());
                par.push_back(topo.stripLength());
                par.push_back(topo.pitch());

                float height = topo.stripLength()/2;
                LocalPoint  lTop( 0., height, 0.);
                LocalPoint  lBottom( 0., -height, 0.);
                par.push_back(roll->localPitch(lTop));
                par.push_back(roll->localPitch(lBottom));
                par.push_back(roll->npads());
            }
        }
    }
    catch ( cms::Exception& exception ) {
        edm::LogWarning("FWRecoGeometryProducerException")
           << "addGEMGeometry() Exception caught while building GEM geometry: ";// << exception.what()
            << std::endl; 
    }

}

//______________________________________________________________________________


void
FWTGeoRecoGeometryESProducer::addRPCGeometry( )
{
   TGeoVolume* tv =  GetTopHolder("Muon", kMuonRPC);
   TGeoVolume *assembly = GetDaughter(tv, "RPC", kMuonRPC);

   DetId detId( DetId::Muon, MuonSubdetId::RPC );
   const RPCGeometry* rpcGeom = (const RPCGeometry*) m_geomRecord->slaveGeometry( detId );
   for( auto it = rpcGeom->rolls().begin(),
           end = rpcGeom->rolls().end(); 
        it != end; ++it )
   {
      RPCRoll const* roll = (*it);
      if( roll )
      {
         RPCDetId detid = roll->geographicalId();
         std::stringstream s;
         s << detid;
         std::string name = s.str();
      
         TGeoVolume* child = createVolume( name, roll, kMuonRPC );

         TGeoVolume* holder  = GetDaughter(assembly, "ROLL Region", kMuonRPC, detid.region());
         holder = GetDaughter(holder, "Ring", kMuonRPC, detid.ring());
         holder = GetDaughter(holder, "Station", kMuonRPC, detid.station()); 
         holder = GetDaughter(holder, "Sector", kMuonRPC, detid.sector()); 
         holder = GetDaughter(holder, "Layer", kMuonRPC, detid.layer()); 
         holder = GetDaughter(holder, "Subsector", kMuonRPC, detid.subsector()); 
 
         AddLeafNode(holder, child, name.c_str(),  createPlacement(*it), detid.rawId());

         DMtopo par;
         const StripTopology& topo = roll->specificTopology();
         par.push_back(topo.nstrips());
         par.push_back(topo.stripLength());
         par.push_back(topo.pitch());
         m_topology[par].push_back(detid.rawId());
      }
   };
}


namespace {

double etatotheta(double eta)
{
   using namespace TMath;

   if (eta < 0)
      return 3.14 - 2*ATan(Exp(- Abs(eta)));
   else
      return 2*ATan(Exp(- Abs(eta)));
}
}

//==============================================================================
//================================= CALO =======================================
//==============================================================================


void
FWTGeoRecoGeometryESProducer::addHcalCaloGeometryBarrel( void )
{
   TGeoVolume* tv =  GetTopHolder("HCal", kHCal); 
   TGeoVolume *assembly = GetDaughter(tv, "HCalBarrel", kHCal);

   std::vector<DetId> vid = m_caloGeom->getValidDetIds(DetId::Hcal, HcalSubdetector::HcalBarrel);

   CaloVolMap caloShapeMapP;
   CaloVolMap caloShapeMapN;
   for( std::vector<DetId>::const_iterator it = vid.begin(), end = vid.end(); it != end; ++it)
   {
      HcalDetId detid = HcalDetId(it->rawId());

      const CaloCellGeometry* cellb= m_caloGeom->getGeometry(*it);

      const IdealObliquePrism* cell = dynamic_cast<const IdealObliquePrism*> (cellb);
   
      if (!cell) { printf ("HB not olique !!!\n"); continue; }
  
      TGeoVolume* volume = 0;
      CaloVolMap& caloShapeMap = (cell->etaPos() > 0) ? caloShapeMapP : caloShapeMapN;
      CaloVolMap::iterator volIt =  caloShapeMap.find(cell->param());
      if  (volIt == caloShapeMap.end()) 
      {
         // printf("FIREWORKS NEW SHAPE BEGIN eta = %f etaPos = %f, phiPos %f >>>>>> \n", cell->eta(), cell->etaPos(), cell->phiPos());
         IdealObliquePrism::Pt3DVec lc(8);
         IdealObliquePrism::Pt3D ref;
         IdealObliquePrism::localCorners( lc, cell->param(), ref);
         HepGeom::Vector3D<float> lCenter;
         for( int c = 0; c < 8; ++c)
            lCenter += lc[c];
         lCenter *= 0.125;

         static const int arr[] = { 1, 0, 3, 2,  5, 4, 7, 6 };
         double points[16];
         for (int c = 0; c < 8; ++c) {
            if (cell->etaPos() > 0 )
               points[ c*2 + 0 ] = -(lc[arr[c]].z() - lCenter.z());
            else
               points[ c*2 + 0 ] = (lc[arr[c]].z() - lCenter.z()); 

            points[ c*2 + 1 ] =  (lc[arr[c]].y() - lCenter.y());
            // printf("AMT xy[%d] <=>[%d] = (%.4f, %.4f) \n", arr[c], c, points[c*2],  points[c*2+1]);
         }

         float dz = (lc[4].x() -lc[0].x()) * 0.5;
         TGeoShape* solid = new TGeoArb8(dz, &points[0]);
         volume = new TGeoVolume("hcal oblique prism", solid, GetMedium(kHCal));
         caloShapeMap[cell->param()] = volume;
      }
      else {

         volume = volIt->second;

      }      

      HepGeom::Vector3D<float> gCenter;
      CaloCellGeometry::CornersVec const & gc = cell->getCorners();
      for (int c = 0; c < 8; ++c)
         gCenter += HepGeom::Vector3D<float>(gc[c].x(), gc[c].y(), gc[c].z());
      gCenter *= 0.125;

      TGeoTranslation gtr(gCenter.x(), gCenter.y(), gCenter.z());
      TGeoRotation rot; 
      rot.RotateY(90);
     
      TGeoRotation rotPhi;
      rotPhi.SetAngles(0, -cell->phiPos()*TMath::RadToDeg(), 0);
      rot.MultiplyBy(&rotPhi);    

      TGeoVolume* holder  = GetDaughter(assembly, "side", kHCal, detid.zside());
      holder = GetDaughter(holder, "ieta", kHCal, detid.ieta());
      std::stringstream nname;
      nname << detid;
      AddLeafNode(holder, volume, nname.str().c_str(), new TGeoCombiTrans(gtr, rot), detid.rawId());
   }


   //  printf("HB map size P = %lu , N = %lu", caloShapeMapP.size(),caloShapeMapN.size() );

}
//______________________________________________________________________________

void
FWTGeoRecoGeometryESProducer::addHcalCaloGeometryEndcap( void )
{

   CaloVolMap caloShapeMapP;
   CaloVolMap caloShapeMapN;

   TGeoVolume* tv =  GetTopHolder("HCal", kHCal); 
   TGeoVolume *assembly = GetDaughter(tv, "HCalEndcap", kHCal);

   std::vector<DetId> vid = m_caloGeom->getValidDetIds(DetId::Hcal, HcalSubdetector::HcalEndcap);

   for( std::vector<DetId>::const_iterator it = vid.begin(), end = vid.end(); it != end; ++it)
   {
      HcalDetId detid = HcalDetId(it->rawId());
      const IdealObliquePrism* cell = dynamic_cast<const IdealObliquePrism*> ( m_caloGeom->getGeometry(*it));
   
      if (!cell) { printf ("EC not olique \n"); continue; }

      TGeoVolume* volume = 0;
      CaloVolMap& caloShapeMap = (cell->etaPos() > 0) ? caloShapeMapP : caloShapeMapN;
      CaloVolMap::iterator volIt =  caloShapeMap.find(cell->param());
      if  ( volIt == caloShapeMap.end()) 
      {
         IdealObliquePrism::Pt3DVec lc(8);
         IdealObliquePrism::Pt3D ref;
         IdealObliquePrism::localCorners( lc, cell->param(), ref);
         HepGeom::Vector3D<float> lCenter; 
         for( int c = 0; c < 8; ++c)
            lCenter += lc[c];
         lCenter *= 0.125;

         //for( int c = 0; c < 8; ++c)
         //   printf("lc.push_back(TEveVector(%.4f, %.4f, %.4f));\n", lc[c].x(), lc[c].y(), lc[c].z() );


         static const int arrP[] = { 3, 2, 1, 0, 7, 6, 5, 4 };
         static const int arrN[] = {  7, 6, 5, 4 ,3, 2, 1, 0};
         const int* arr = (detid.ieta() > 0) ?  &arrP[0] : &arrN[0];

         double points[16];
         for (int c = 0; c < 8; ++c) {
            points[ c*2 + 0 ] = lc[arr[c]].x() - lCenter.x(); 
            points[ c*2 + 1 ] = lc[arr[c]].y() - lCenter.y();
         }

         float dz = (lc[4].z() -lc[0].z()) * 0.5;
         TGeoShape* solid = new TGeoArb8(dz, &points[0]);
         volume = new TGeoVolume("ecal oblique prism", solid, GetMedium(kHCal));
         caloShapeMap[cell->param()] = volume;
      }
      else {

         volume = volIt->second;

      }      

      HepGeom::Vector3D<float> gCenter;
      CaloCellGeometry::CornersVec const & gc = cell->getCorners();
      for (int c = 0; c < 8; ++c) {
         gCenter += HepGeom::Vector3D<float>(gc[c].x(), gc[c].y(), gc[c].z());
         //  printf("gc.push_back(TEveVector(%.4f, %.4f, %.4f));\n", gc[c].x(), gc[c].y(),gc[c].z() );
      }
      gCenter *= 0.125;

      TGeoTranslation gtr(gCenter.x(), gCenter.y(), gCenter.z());
      TGeoRotation rot;
      rot.SetAngles(cell->phiPos()*TMath::RadToDeg(), 0, 0);

      TGeoVolume* holder  = GetDaughter(assembly, "side", kHCal, detid.zside());
      holder = GetDaughter(holder, "ieta", kHCal, detid.ieta());
      std::stringstream nname;
      nname << detid;
      AddLeafNode(holder, volume, nname.str().c_str(), new TGeoCombiTrans(gtr, rot), detid.rawId());
   }

   //   printf("HE map size P = %lu , N = %lu", caloShapeMapP.size(),caloShapeMapN.size() );
}


//______________________________________________________________________________

TGeoHMatrix* getEcalTrans(CaloCellGeometry::CornersVec const & gc)
{

   TEveVector gCenter;
   for (int i = 0; i < 8; ++i)
      gCenter += TEveVector(gc[i].x(), gc[i].y(), gc[i].z());
   gCenter *= 0.125;

  TEveVector tgCenter; // top center 4 corners
   for (int i = 4; i < 8; ++i)
      tgCenter += TEveVector(gc[i].x(), gc[i].y(), gc[i].z());
   tgCenter *= 0.25;


   TEveVector axis = tgCenter - gCenter;
   axis.Normalize();

   TEveTrans tr;
   TVector3 v1t;
   tr.GetBaseVec(1, v1t);


   TEveVector v1(v1t.x(), v1t.y(), v1t.z());
   double dot13 = axis.Dot(v1);
   TEveVector gd = axis;
   gd*= dot13;
   v1 -= gd;
   v1.Normalize();
   TEveVector v2;
   TMath::Cross(v1.Arr(), axis.Arr(), v2.Arr());
   TMath::Cross(axis.Arr(), v1.Arr(), v2.Arr());
   v2.Normalize();

   tr.SetBaseVec(1, v1.fX, v1.fY, v1.fZ);
   tr.SetBaseVec(2, v2.fX, v2.fY, v2.fZ);
   tr.SetBaseVec(3, axis.fX, axis.fY, axis.fZ);
   tr.Move3PF(gCenter.fX, gCenter.fY, gCenter.fZ);

   TGeoHMatrix* out = new TGeoHMatrix();
   tr.SetGeoHMatrix(*out);
   return out;
}

TGeoShape* makeEcalShape(const TruncatedPyramid* cell)
{
   // printf("BEGIN EE SHAPE --------------------------------\n");
   // std::cout << detid << std::endl;
   const HepGeom::Transform3D idtr;
   TruncatedPyramid::Pt3DVec co(8);
   TruncatedPyramid::Pt3D ref;
   TruncatedPyramid::localCorners( co, cell->param(), ref);
   //for( int c = 0; c < 8; ++c)
   //   printf("lc.push_back(TEveVector(%.4f, %.4f, %.4f));\n", co[c].x(),co[c].y(),co[c].z() );

   double points[16];
   for( int c = 0; c < 8; ++c )
   {
      points[c*2  ] = co[c].x();
      points[c*2+1] = co[c].y();
   }
   TGeoShape* solid = new TGeoArb8(cell->param()[0], points);
   return solid;
}

//______________________________________________________________________________



void
FWTGeoRecoGeometryESProducer::addEcalCaloGeometry( void )
{

   TGeoVolume* tv =  GetTopHolder("ECal", kECal);
   CaloVolMap caloShapeMap;

   {
      TGeoVolume *assembly = GetDaughter(tv, "ECalBarrel", kECal);

      std::vector<DetId> vid = m_caloGeom->getValidDetIds(DetId::Ecal, EcalSubdetector::EcalBarrel);
      for( std::vector<DetId>::const_iterator it = vid.begin(), end = vid.end(); it != end; ++it)
      {
         EBDetId detid(*it);
         const TruncatedPyramid* cell = dynamic_cast<const TruncatedPyramid*> ( m_caloGeom->getGeometry( *it ));
         if (!cell) { printf("ecalBarrel cell not a TruncatedPyramid !!\n"); return; }

         TGeoVolume* volume = 0;
         CaloVolMap::iterator volIt =  caloShapeMap.find(cell->param());
         if  ( volIt == caloShapeMap.end()) 
         {           
            volume = new TGeoVolume( "EE TruncatedPyramid" , makeEcalShape(cell), GetMedium(kECal));
            caloShapeMap[cell->param()] = volume;
         }
         else {
            volume = volIt->second;
         }
         TGeoHMatrix* mtx= getEcalTrans(cell->getCorners());
         TGeoVolume* holder = GetDaughter(assembly, "side", kECal, detid.zside());
         holder = GetDaughter(holder, "ieta", kECal, detid.ieta());
         std::stringstream nname;
         nname << detid;
         AddLeafNode(holder, volume, nname.str().c_str(), mtx, detid.rawId());
      }
   }
   

   {
      TGeoVolume *assembly = GetDaughter(tv, "ECalEndcap", kECal);

      std::vector<DetId> vid = m_caloGeom->getValidDetIds(DetId::Ecal, EcalSubdetector::EcalEndcap);
      for( std::vector<DetId>::const_iterator it = vid.begin(), end = vid.end(); it != end; ++it)
      {
         EEDetId detid(*it);
         const TruncatedPyramid* cell = dynamic_cast<const TruncatedPyramid*> (m_caloGeom->getGeometry( *it ));
         if (!cell) { printf("ecalEndcap cell not a TruncatedPyramid !!\n"); continue;}

         TGeoVolume* volume = 0;
         CaloVolMap::iterator volIt =  caloShapeMap.find(cell->param());
         if  ( volIt == caloShapeMap.end()) 
         {
            
            volume = new TGeoVolume( "EE TruncatedPyramid" , makeEcalShape(cell), GetMedium(kECal));
            caloShapeMap[cell->param()] = volume;
         }
         else {
            volume = volIt->second;
         }
         TGeoHMatrix* mtx= getEcalTrans(cell->getCorners());
         TGeoVolume* holder = GetDaughter(assembly, "side", kECal, detid.zside());
         holder = GetDaughter(holder, "ix", kECal, detid.ix());
         std::stringstream nname;
         nname << detid;
         AddLeafNode(holder, volume, nname.str().c_str(), mtx, detid.rawId());
      }
   }
}

