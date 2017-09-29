#include "geodesy.h"

void Geodesy::Initialise(double Lat_Origin, double Lon_Origin)
{
  LatOrigin =Lat_Origin;
  LonOrigin=Lon_Origin;
  UTM_Zone=getUTMZone(Lon_Origin);
  
  UTM.SetWellKnownGeogCS("WGS84");
  UTM.SetUTM(UTM_Zone, signbit(-Lat_Origin));
  
  LatLong.SetWellKnownGeogCS("WGS84");
  LatLong2UTM(LatOrigin, LonOrigin, x_origin, y_origin);
}

void Geodesy::LatLong2UTM(double Lat, double Lon, double &x, double &y)
{
  OGRCoordinateTransformation* coordTrans = OGRCreateCoordinateTransformation(&LatLong, &UTM);
  bool reprojected = coordTrans->Transform(1, &Lon, &Lat);
  x=Lon;
  y=Lat;
}

void Geodesy::UTM2LatLong(double x, double y, double &Lat, double &Lon)
{
    OGRCoordinateTransformation* coordTrans = OGRCreateCoordinateTransformation(&UTM, &LatLong);
    bool reprojected = coordTrans->Transform(1, &x, &y);
    Lon = x;
    Lat = y;
}

// Convert a geometry to the other coordinate system
OGRGeometry* Geodesy::LatLong2UTM(OGRGeometry *LatLong_geom)
{
  if (LatLong_geom->transformTo(&UTM) == OGRERR_NONE)
    return LatLong_geom;
  else{
    cout << "Failed to convert geometry to UTM" << endl;
    exit(0);
  }
}

OGRGeometry* Geodesy::UTM2LatLong(OGRGeometry *UTM_geom)
{
  if (UTM_geom->transformTo(&LatLong)== OGRERR_NONE)
    return UTM_geom;
  else{
    cout << "Failed to convert geometry to LatLong" << endl;
    exit(0);
  }
}

// Convert a polygon to the other coordinate system
OGRPolygon* Geodesy::LatLong2UTM(OGRPolygon *LatLong_poly)
{
  if (LatLong_poly->transformTo(&UTM)== OGRERR_NONE)
    return LatLong_poly;
  else{
    cout << "Failed to convert polygon to UTM" << endl;
    exit(0);
  }
}

OGRPolygon* Geodesy::UTM2LatLong(OGRPolygon *UTM_poly)
{
  if (UTM_poly->transformTo(&LatLong)== OGRERR_NONE)
    return UTM_poly;
  else{
    cout << "Failed to convert polygon to LatLong" << endl;
    exit(0);
  }
}

OGRLinearRing* Geodesy::LatLong2UTM(OGRLinearRing *LatLong_ring)
{
    if (LatLong_ring->transformTo(&UTM)== OGRERR_NONE)
      return LatLong_ring;
    else{
      cout << "Failed to convert ring to UTM" << endl;
      exit(0);
    }
}

OGRLinearRing* Geodesy::UTM2LatLong(OGRLinearRing *UTM_ring)
{
    if (UTM_ring->transformTo(&LatLong)== OGRERR_NONE)
      return UTM_ring;
    else{
      cout << "Failed to convert polygon to LatLong" << endl;
      exit(0);
    }
}

// Convert a point to the other coordinate system
OGRPoint* Geodesy::LatLong2UTM(OGRPoint *LatLong_pnt)
{
  if (LatLong_pnt->transformTo(&UTM) == OGRERR_NONE)
    return LatLong_pnt;
  else{
    cout << "Failed to convert point to UTM" << endl;
    exit(0);
  }
}

OGRPoint* Geodesy::UTM2LatLong(OGRPoint *UTM_pnt)
{
  if (UTM_pnt->transformTo(&LatLong)== OGRERR_NONE)
    return UTM_pnt;
  else{
    cout << "Failed to convert point to LatLong" << endl;
    exit(0);
  }
}

// Convert a line to the other coordinate system
OGRLineString* Geodesy::LatLong2UTM(OGRLineString *LatLong_line)
{
  if (LatLong_line->transformTo(&UTM)== OGRERR_NONE)
    return LatLong_line;
  else{
    cout << "Failed to convert Line to UTM" << endl;
    exit(0);
  }
}

OGRLineString* Geodesy::UTM2LatLong(OGRLineString *UTM_line)
{
  if (UTM_line->transformTo(&LatLong)== OGRERR_NONE)
    return UTM_line;
  else{
    cout << "Failed to convert Line to LatLong" << endl;
    exit(0);
  }
}
