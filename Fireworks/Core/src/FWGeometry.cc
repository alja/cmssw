#include "TFile.h"
#include "TTree.h"
#include "TEveGeoNode.h"
#include "TPRegexp.h"
#include "TSystem.h"
#include "TGeoArb8.h"
#include "TObjArray.h"

#include "TGeoManager.h"
#include "TGeoVolume.h"
#include "TGeoNode.h"
#include "TGeoMatrix.h"

#include "Fireworks/Core/interface/FWGeometry.h"
#include "Fireworks/Core/interface/fwLog.h"
#include "DataFormats/DetId/interface/DetId.h"

#include <iostream>
#include <cassert>
#include <sstream>
#include <stdexcept>
#include <algorithm>

#include "boost/lexical_cast.hpp"


FWGeometry::FWGeometry( void )
{}

FWGeometry::~FWGeometry( void )
{}

TFile*
FWGeometry::findFile( const char* fileName )
{
   std::string searchPath = ".";

   if (gSystem->Getenv( "CMSSW_SEARCH_PATH" ))
   {    
       TString paths = gSystem->Getenv( "CMSSW_SEARCH_PATH" );
       TObjArray* tokens = paths.Tokenize( ":" );
       for( int i = 0; i < tokens->GetEntries(); ++i )
       {
           TObjString* path = (TObjString*)tokens->At( i );
           searchPath += ":";
           searchPath += path->GetString();
           searchPath += "/Fireworks/Geometry/data/";
       }
   }

   TString fn = fileName;
   const char* fp = gSystem->FindFile(searchPath.c_str(), fn, kFileExists);
   return fp ? TFile::Open( fp) : 0;
}

//______________________________________________________________________________

void FWGeometry::recursiveImportVolume(const TGeoVolume* mother)
{
   using boost::lexical_cast;
   using boost::bad_lexical_cast;
   int n = mother->GetNdaughters();
   for (int i = 0; i < n; ++i)
   {
      TGeoNode* node = mother->GetNode(i);
      std::string title = node->GetTitle();
      if (!title.empty()) {
         try {
            unsigned int rawid = lexical_cast<unsigned int>(title);
            m_idToInfo.push_back(GeomDetInfo(rawid, node));
            // printf("importr %u == %s\n", rawid, node->GetTitle());
         }
         catch (bad_lexical_cast &e) {
            fwLog(fwlog::kError) << "import [" << title << "] " << e.what() <<std::endl;
            // assert(false);
         }
      }
      recursiveImportVolume(node->GetVolume());
   }

}

namespace {
   struct less_than_key
   {
      inline bool operator() (const FWGeometry::GeomDetInfo& struct1, const FWGeometry::GeomDetInfo& struct2)
      {
         return (struct1.id < struct2.id);
      }
   };
}

void
FWGeometry::loadMap( const char* fileName )
{  
   TFile* file = findFile( fileName );
   if( ! file )
   {
      throw std::runtime_error( "ERROR: failed to find geometry file. Initialization failed." );
      return;
   }

   m_geoManager = (TGeoManager*) file->Get("cmsGeo;1");
   recursiveImportVolume(m_geoManager->GetTopVolume());



   std::sort(m_idToInfo.begin(), m_idToInfo.end(), less_than_key());

  
   file->Close();
}


void
FWGeometry::initMap( const FWRecoGeom::InfoMap& map )
{
}
//______________________________________________________________________________



const TGeoMatrix*
FWGeometry::getMatrix( unsigned int id ) const
{
   assert(false);   return 0;
}

//______________________________________________________________________________


std::vector<unsigned int>
FWGeometry::getMatchedIds( Detector det, SubDetector subdet ) const
{
   std::vector<unsigned int> ids;
   unsigned int mask = ( det << 4 ) | ( subdet );
   for( IdToInfoItr it = m_idToInfo.begin(), itEnd = m_idToInfo.end();
	it != itEnd; ++it )
   {
      if( FWGeometry::match_id( *it, mask ))
	 ids.push_back(( *it ).id );
   }
   
   return ids;
}

//______________________________________________________________________________

TGeoShape*
FWGeometry::getShape( unsigned int id ) const
{
   IdToInfoItr it = FWGeometry::find( id );
   if( it == m_idToInfo.end())
   {
      fwLog( fwlog::kWarning ) << "no reco geoemtry found for id " <<  id << std::endl;
      return 0;
   }
   else 
   {
      return getShape( *it );
   }
}



TGeoShape*
FWGeometry::getShape( const GeomDetInfo& info ) const 
{
   return info.node->GetVolume()->GetShape();
}

//______________________________________________________________________________



TEveGeoShape*
FWGeometry::getEveShape( unsigned int id  ) const
{
   IdToInfoItr it = FWGeometry::find( id );
   if( it == m_idToInfo.end())
   {
      fwLog( fwlog::kWarning ) << "no reco geoemtry found for id " <<  id << std::endl;
      return 0;
   }
   else
   {
      const GeomDetInfo& info = *it;
      TEveGeoShape* shape = new TEveGeoShape(TString::Format("RecoGeom Id=%u", id));
      TGeoShape* geoShape = getShape( info );
      shape->SetShape( geoShape );
      // Set transformation matrix from a column-major array
      shape->SetTransMatrix( *info.node->GetMatrix() );
      return shape;
   }
}

//______________________________________________________________________________


const float*
FWGeometry::getCorners( unsigned int id ) const
{
   // reco geometry points
   IdToInfoItr it = FWGeometry::find( id );
   if( it == m_idToInfo.end())
   {
      fwLog( fwlog::kWarning ) << "no reco geometry found for id " <<  id << std::endl;
      return 0;
   }
   else
   {
      return ( *it ).points;
   }
}
//______________________________________________________________________________


const float*
FWGeometry::getParameters( unsigned int id ) const
{
   // reco geometry parameters
   IdToInfoItr it = FWGeometry::find( id );
   if( it == m_idToInfo.end())
   {
      fwLog( fwlog::kWarning ) << "no reco geometry found for id " <<  id << std::endl;
      return 0;
   }
   else
   {
      return ( *it ).parameters;
   }
}

//______________________________________________________________________________


void
FWGeometry::localToGlobal( unsigned int id, const float* local, float* global, bool translatep ) const
{
   IdToInfoItr it = FWGeometry::find( id );
   if( it == m_idToInfo.end())
   {
      fwLog( fwlog::kWarning ) << "no reco geometry found for id " <<  id << std::endl;
   }
   else
   {
      localToGlobal( *it, local, global, translatep );
   }
}

void
FWGeometry::localToGlobal( unsigned int id, const float* local1, float* global1,
                           const float* local2, float* global2, bool translatep ) const
{
   IdToInfoItr it = FWGeometry::find( id );
   if( it == m_idToInfo.end())
   {
      fwLog( fwlog::kWarning ) << "no reco geometry found for id " <<  id << std::endl;
   }
   else
   {
      localToGlobal( *it, local1, global1, translatep );
      localToGlobal( *it, local2, global2, translatep );
   }
}


void
FWGeometry::localToGlobal( const GeomDetInfo& info, const float* local, float* global, bool translatep ) const
{
   const Double_t* translation = info.node->GetMatrix()->GetTranslation();
   const Double_t* rotation    = info.node->GetMatrix()->GetRotationMatrix();

   for( int i = 0; i < 3; ++i )
   {
      global[i]  = translatep ? translation[i] : 0;
      global[i] +=   local[0] * rotation[3 * i]
		   + local[1] * rotation[3 * i + 1]
		   + local[2] * rotation[3 * i + 2];
   }
}

//______________________________________________________________________________

FWGeometry::IdToInfoItr
FWGeometry::find( unsigned int id ) const
{
  FWGeometry::IdToInfoItr begin = m_idToInfo.begin();
  FWGeometry::IdToInfoItr end = m_idToInfo.end();
  return std::lower_bound( begin, end, id );
}

//______________________________________________________________________________

bool FWGeometry::VersionInfo::haveExtraDet(const char* det) const
{
   
   return (extraDetectors && extraDetectors->FindObject(det)) ? true : false;
}

