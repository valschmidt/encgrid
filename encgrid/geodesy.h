/*
 * geodesy.h
 *
 *  Created on: Mar 31, 2017
 *      Author: Sam Reed
 */

#include "ogr_spatialref.h"
#include "gdal_frmts.h" // for GDAL/OGR
#include <ogrsf_frmts.h> // OGRLayer
#include <ogr_geometry.h> // OGRPoint, OGRGeometry etc
#include <ogr_feature.h> // OGRFeature
#include <string>
#include <iostream>
#include <cmath>

using namespace std;

#ifndef GEODESY_H_
#define GEODESY_H_


class Geodesy
{
public:
	// Constructor
        Geodesy() {Initialise(0, 0); UTM_Zone=0; }
        Geodesy(double LatOrigin, double LonOrigin) {Initialise(LatOrigin, LonOrigin); }
        ~Geodesy() {}

        double getLatOrigin() { return LatOrigin; }
        double getLonOrigin() { return LonOrigin; }

	void Initialise(double Lat_Origin, double Lon_Origin);

	void LatLong2UTM(double Lat, double Lon, double &x, double &y);
	void UTM2LatLong(double x, double y, double &Lat, double &Lon);

        OGRGeometry* LatLong2UTM(OGRGeometry *LatLong_geom);
        OGRGeometry* UTM2LatLong(OGRGeometry *UTM_geom);

	OGRPolygon* LatLong2UTM(OGRPolygon *LatLong_poly);
	OGRPolygon* UTM2LatLong(OGRPolygon *UTM_poly);

        OGRLinearRing* LatLong2UTM(OGRLinearRing *LatLong_ring);
        OGRLinearRing* UTM2LatLong(OGRLinearRing *UTM_ring);

	OGRPoint* LatLong2UTM(OGRPoint *LatLong_pnt);
	OGRPoint* UTM2LatLong(OGRPoint *UTM_pnt);

	OGRLineString* LatLong2UTM(OGRLineString *LatLong_line);
	OGRLineString* UTM2LatLong(OGRLineString *UTM_line);

        void LatLong2LocalUTM(double Lat, double Lon, double &x, double &y) {LatLong2UTM(Lat, Lon, x, y); x-=x_origin; y-=y_origin; }
        void LocalUTM2LatLong(double x, double y, double &Lat, double &Lon) {x+=x_origin; y+=y_origin; UTM2LatLong(x, y, Lat, Lon); }
	
        int getUTMZone(double longitude) {return 1+(longitude+180.0)/6.0; }

	OGRSpatialReference getUTM() {return UTM; }
	OGRSpatialReference getLatLong() {return LatLong; }

        double getXOrigin() {return x_origin;}
        double getYOrigin() {return y_origin;}

private:
	double LatOrigin, LonOrigin;
	double x_origin, y_origin;
	int UTM_Zone;

	OGRSpatialReference UTM, LatLong;
        //OGRCoordinateTransformation *coordTrans2LatLong, *coordTrans2UTM;
};

#endif /* GEODESY_H_ */
