#ifndef GEOMETRY_FWTGEO_RECO_GEOMETRY_H
# define GEOMETRY_FWTGEO_RECO_GEOMETRY_H

# include <cassert>
# include <map>
# include <string>
# include <vector>

# include "DataFormats/GeometryVector/interface/GlobalPoint.h"

class TGeoManager;

class FWTGeoRecoGeometry
{
public:
  FWTGeoRecoGeometry( void );
  virtual ~FWTGeoRecoGeometry( void );

  TGeoManager* manager( void ) const
    {
      return m_manager;
    }
  void manager( TGeoManager* geom )
    {
      m_manager = geom;
    }

private:
  TGeoManager* m_manager;
};

#endif // GEOMETRY_FWTGEO_RECO_GEOMETRY_H
