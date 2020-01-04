#include <vector>
#include <iostream>
#include <string>
#include <cmath> // For sqrt and pow
#include <stdlib.h> // for atoi and atof (string to double/int)
#include <algorithm> // for max_element and sort
#include "gdal_frmts.h" // for GDAL/OGR
#include <ogrsf_frmts.h> // OGRLayer
#include <ogr_geometry.h> // OGRPoint, OGRGeometry etc
#include <ogr_feature.h> // OGRFeature
#include "geodesy.h"
#include "gdal_alg.h"
#include <math.h>

#include "gdal_alg_priv.h"
#include "gdal_priv.h"
#include "ogr_api.h"
#include "ogr_geometry.h"
#include "ogr_spatialref.h"

#include <stdlib.h>
#include <fstream>
#include <ctime>

#include <boost/filesystem.hpp>

#ifdef OGR_ENABLED
#include "ogrsf_frmts.h"
#endif

using namespace std;
namespace fs = boost::filesystem;

#ifndef GRIDINTERP_H
#define GRIDINTERP_H

class Grid_Interp
{
public:
    Grid_Interp();
    Grid_Interp(string MOOS_path, string ENC_Filename, double Grid_size, double buffer_dist, double MHW_offset, Geodesy Geod, bool simpleGrid);
    Grid_Interp(string MOOS_path, string ENC_Filename, double Grid_size, double buffer_dist, double MHW_offset, double lat, double lon, bool simpleGrid);
    ~Grid_Interp() {}

    // Initialize Geodesy
    void initGeodesy(Geodesy Geod) { geod = Geodesy(Geod.getLatOrigin(), Geod.getLonOrigin()); }

    void Run(bool csv, bool mat);
    void Run(bool output) {Run(output, output); }
    void write2csv(vector<int> &poly_data, vector<int> &depth_data, vector<int> &point_data, int x_res, int y_res, bool mat);
    void writeMat(string filename);
    vector <vector<int> > transposeMap();

    void buildLayers();

    // Get the minimum and maximum x/y values (in local UTM) of the ENC
    void getENC_MinMax(GDALDataset* ds);

    void rasterizeSHP(string outfilename, string infilename, string attribute);
    void store_vertices(OGRPolygon *UTM_Poly, double z);
    void raster2XYZ(vector<int>&RasterData, int nXSize);
    void getRasterData(string filename, int &nXSize, int &nYSize, vector<int>&RasterData);
    void writeRasterData(string filename, int nXSize, int nYSize, vector<float> &RasterData);
    void writeRasterData(string filename, int nXSize, int nYSize, vector<int>&RasterData);

    void layer2XYZ(OGRLayer* layer, string layerName);
    void multipointFeat(OGRFeature* feat, OGRGeometry* geom);
    void pointFeat(OGRFeature* feat, OGRGeometry* geom, string layerName);
    void polygonFeat(OGRFeature* feat, OGRGeometry* geom, string layerName);
    void lineFeat(OGRFeature* feat, OGRGeometry* geom, string layerName);

    // Fills out the 2D interpolated map from the 1D interpolated data and the Depth areas
    void updateMap(vector<int> &poly_data, vector<int> &depth_data, vector<int> &outline_data, vector<int> &point_data, vector<float> &new_rast_data, int x_res, int y_res);

    // Returns the interpolated map
    vector<vector<int> > getGriddedMap() {return Map; }

    // Functions for converting between the grid, local UTM, and Lat/Long
    void UTM2grid(double x, double y, int &gridX, int &gridY) { xy2grid(x+geod.getXOrigin(),y+geod.getYOrigin(), gridX, gridY);}
    void xy2grid(double x, double y, int &gridX, int &gridY);
    void grid2xy(int gridX, int gridY, double &x, double &y);
    void LatLong2Grid(double lat, double lon, int &gridX, int &gridY);
    void Grid2LatLong(int gridX, int gridY, double &lat, double &lon);

    // Conversions between 1D and 2D vectors
    void row_major_to_2D(int index, double &x, double &y, int numCols);
    void row_major2grid(int index, int &gridX, int &gridY, int numCols);
    void rasterIndex2gridIndex(int rasterIndex, int &gridIndex, int x_res, int y_res);

    // Returns each member of the envelope of the Map
    double getMinX() {return minX; }
    double getMaxX() {return maxX; }
    double getMinY() {return minY; }
    double getMaxY() {return maxY; }

    double getGridSize() {return grid_size; }

    // Convert the WL to a depth
    double calcDepth(int WL);


private:
    Geodesy geod;
    string ENC_filename, MOOS_Path;
    vector<double> X,Y,depth;
    vector<int> griddedData;
    vector <vector<int> > Map, t_map;
    double minX, minY, maxX, maxY;
    double grid_size;
    double MHW_Offset, landZ;
    double buffer_size;
    GDALDataset *ds_poly, *ds_pnt, *ds_depth, *ds_outline;
    OGRLayer  *layer_poly, *layer_pnt, *layer_depth, *layer_outline;
    OGRFeatureDefn *feat_def_poly, *feat_def_pnt, *feat_def_depth, *feat_def_outline;
    bool simpleGrid;
};

#endif // GRIDINTERP_H

