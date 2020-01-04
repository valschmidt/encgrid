#include "gridinterp.h"

Grid_Interp::Grid_Interp()
{
  geod = Geodesy();
  grid_size = 5;
  buffer_size = 5;
  MOOS_Path = "/home/sji367/moos-ivp/moos-ivp-reed/";
  ENC_filename = "/home/sji367/moos-ivp/moos-ivp-reed/src/ENCs/US5NH02M/US5NH02M.000";
  MHW_Offset = 0;
  minX = 0;
  minY = 0;
  maxX = 0;
  maxY = 0;
  landZ = -500;
  simpleGrid = false;
}

Grid_Interp::Grid_Interp(string MOOS_path, string ENC_Filename, double Grid_size, double buffer_dist, double MHW_offset, Geodesy Geod, bool SimpleGrid)
{
  initGeodesy(Geod);
  grid_size = Grid_size;
  ENC_filename = ENC_Filename;
  MOOS_Path = MOOS_path;
  buffer_size = buffer_dist;
  MHW_Offset = MHW_offset;
  minX = 0;
  minY = 0;
  maxX = 0;
  maxY = 0;
  landZ = static_cast<int>(round(-(MHW_offset+2)*100));
  simpleGrid = SimpleGrid;
}

Grid_Interp::Grid_Interp(string MOOS_path, string ENC_Filename, double Grid_size, double buffer_dist, double MHW_offset, double lat, double lon, bool SimpleGrid)
{
  geod = Geodesy(lat, lon);
  grid_size = Grid_size;
  ENC_filename = ENC_Filename;
  MOOS_Path = MOOS_path;
  buffer_size = buffer_dist;
  MHW_Offset = MHW_offset;
  minX = 0;
  minY = 0;
  maxX = 0;
  maxY = 0;
  landZ = static_cast<int>(round(-(MHW_offset+2)*100));
  simpleGrid = SimpleGrid;
}

void Grid_Interp::buildLayers()
{
    GDALAllRegister();
    // Build the datasets
    const char *pszDriverName = "ESRI Shapefile";
    GDALDriver *poDriver;
    poDriver = GetGDALDriverManager()->GetDriverByName(pszDriverName );
    OGRFieldDefn oField_depth( "Depth", OFTReal);
    OGRFieldDefn oField_inside( "Inside_ENC", OFTReal);

    // Build the file and layer that will hold the wrecks and land area polygons
    // *************************************************************************
    string polyPath = MOOS_Path+"/polygon.shp";
    ds_poly = poDriver->Create(polyPath.c_str(), 0, 0, 0, GDT_Unknown, NULL );
    if( ds_poly == NULL )
    {
        printf( "Creation of output file failed.\n" );
        exit( 1 );
    }
    // Create the layers
    layer_poly = ds_poly->CreateLayer( "poly", NULL, wkbPolygon25D, NULL );
    if( layer_poly == NULL )
    {
        printf( "Layer creation failed.\n" );
        exit( 1 );
    }
    // Create the field
    if(layer_poly->CreateField( &oField_depth ) != OGRERR_NONE )
    {
        printf( "Creating Depth field failed.\n" );
        exit( 1 );
    }
    feat_def_poly = layer_poly->GetLayerDefn();

    // Build the datasource and layer that will hold independant points
    // *************************************************************************
    string pntPath = MOOS_Path+"/point.shp";
    ds_pnt = poDriver->Create(pntPath.c_str(), 0, 0, 0, GDT_Unknown, NULL );
    if( ds_pnt == NULL )
    {
        printf( "Creation of output file failed.\n" );
        exit( 1 );
    }
    // Create the layers
    // VES Changed to polygon later to handle buffering points.
    //layer_pnt = ds_pnt->CreateLayer( "point", NULL, wkbPoint25D, NULL );
    layer_pnt = ds_pnt->CreateLayer( "point", NULL, wkbPolygon25D, NULL );

    if( layer_pnt == NULL )
    {
        printf( "Layer creation failed.\n" );
        exit( 1 );
    }
    // Create the field
    if(layer_pnt->CreateField( &oField_depth ) != OGRERR_NONE )
    {
        printf( "Creating Depth field failed.\n" );
        exit( 1 );
    }
    feat_def_pnt = layer_pnt->GetLayerDefn();


    // Build the datasource layer that will hold the depth area polygons
    // *************************************************************************
    string depthPath = MOOS_Path+"/depth.shp";
    ds_depth = poDriver->Create(depthPath.c_str(), 0, 0, 0, GDT_Unknown, NULL );
    if( ds_depth == NULL )
    {
        printf( "Creation of output file failed.\n" );
        exit( 1 );
    }
    // Create the layer
    layer_depth = ds_depth->CreateLayer( "depth", NULL, wkbPolygon25D, NULL );
    if( layer_depth == NULL )
    {
        printf( "Layer creation failed.\n" );
        exit( 1 );
    }
    // Create the field
    if(layer_depth->CreateField( &oField_depth ) != OGRERR_NONE )
    {
        printf( "Creating Depth field failed.\n" );
        exit( 1 );
    }
    feat_def_depth = layer_depth->GetLayerDefn();


    // Build the datasource and layer that will hold the outline of the ENC
    // *************************************************************************
    string outlinePath = MOOS_Path+"/outline.shp";
    ds_outline = poDriver->Create(outlinePath.c_str(), 0, 0, 0, GDT_Unknown, NULL );
    // MODIFIED: VES 9/29/2017
    // Fixed bug in which this check checked ds_poly again.
    if( ds_outline == NULL )
    {
        printf( "Creation of output file failed.\n" );
        exit( 1 );
    }
    // Create the layers
    layer_outline = ds_outline->CreateLayer( "outline", NULL, wkbPolygon25D, NULL );
    if( layer_outline == NULL )
    {
        printf( "Layer creation failed.\n" );
        exit( 1 );
    }
    // Create the field
    if(layer_outline->CreateField( &oField_inside ) != OGRERR_NONE )
    {
        printf( "Creating Inside_ENC field failed.\n" );
        exit( 1 );
    }
    feat_def_outline = layer_outline->GetLayerDefn();
}

// This function creates a grid from the soundings, land areas, rocks, wrecks, depth areas
//  and depth contours and store it as a 2D vector of ints where the depth is stored in cm.
void Grid_Interp::Run(bool csv, bool mat)
{
    clock_t start = clock();
    GDALAllRegister();
    GDALDataset* ds;

    // buildLayers is a prepatory step that creates shape files as temporary holders
    // for data of specific kinds for later processing.
    buildLayers();
    cout << "Before\n";

    ds = (GDALDataset*) GDALOpenEx( ENC_filename.c_str(), GDAL_OF_VECTOR, NULL, NULL, NULL );
    cout << "after";
    
    getENC_MinMax(ds);

    vector<int> poly_rasterdata, depth_area_rasterdata, ENC_outline_rasterdata, point_rasterdata;
    int poly_extentX, poly_extentY, depth_extentX, depth_extentY, outline_extentX, outline_extentY, point_extentX, point_extentY;

    // Add the vertices from the soundings, land areas, rocks, wrecks, and depth contours.
    // This data is going into X, Y, and depth (private variables of the grid class.

    layer2XYZ(ds->GetLayerByName("SOUNDG"), "SOUNDG");
    layer2XYZ(ds->GetLayerByName("DEPCNT"), "DEPCNT");
    layer2XYZ(ds->GetLayerByName("M_COVR"), "M_COVR");
    layer2XYZ(ds->GetLayerByName("DEPARE"), "DEPARE");
    layer2XYZ(ds->GetLayerByName("LNDARE"), "LNDARE");

    if (!simpleGrid)
    {
        layer2XYZ(ds->GetLayerByName("UWTROC"), "UWTROC");
        layer2XYZ(ds->GetLayerByName("WRECKS"), "WRECKS");
        layer2XYZ(ds->GetLayerByName("PONTON"), "PONTON");
        layer2XYZ(ds->GetLayerByName("FLODOC"), "FLODOC");
        layer2XYZ(ds->GetLayerByName("OBSTRN"), "OBSTRN");
        layer2XYZ(ds->GetLayerByName("DYKCON"), "DYKCON");
    }

//    layer2XYZ(ds->GetLayerByName("SOUNDG"), "SOUNDG");
//    layer2XYZ(ds->GetLayerByName("UWTROC"), "UWTROC");
//    layer2XYZ(ds->GetLayerByName("WRECKS"), "WRECKS");
//    layer2XYZ(ds->GetLayerByName("PONTON"), "PONTON");
//    layer2XYZ(ds->GetLayerByName("DEPCNT"), "DEPCNT");
//    layer2XYZ(ds->GetLayerByName("M_COVR"), "M_COVR");
//    layer2XYZ(ds->GetLayerByName("DEPARE"), "DEPARE");
//    layer2XYZ(ds->GetLayerByName("FLODOC"), "FLODOC");
//    layer2XYZ(ds->GetLayerByName("OBSTRN"), "OBSTRN");
//    layer2XYZ(ds->GetLayerByName("DYKCON"), "DYKCON");
//    layer2XYZ(ds->GetLayerByName("LNDARE"), "LNDARE");

    // Save the rasters
    GDALClose(ds_depth);
    GDALClose(ds_poly);
    GDALClose(ds_outline);
    GDALClose(ds_pnt);

    // Rasterize the polygon, point, outline and depth area layers
    rasterizeSHP("polygon.tiff","polygon.shp", "Depth");
    rasterizeSHP("depth_area.tiff","depth.shp", "Depth");
    rasterizeSHP("outline.tiff", "outline.shp", "Inside_ENC");
    rasterizeSHP("point.tiff","point.shp", "Depth");
    cout << "rasterize" << endl;

    // Store the rasterized data as 1D vectors
    getRasterData("polygon.tiff", poly_extentX, poly_extentY, poly_rasterdata);
    getRasterData("depth_area.tiff", depth_extentX, depth_extentY, depth_area_rasterdata);
    getRasterData("outline.tiff", outline_extentX, outline_extentY, ENC_outline_rasterdata);
    getRasterData("point.tiff", point_extentX, point_extentY, point_rasterdata);

    // Grid the data
    GDALGridLinearOptions options;
    options.dfRadius = 0;
    options.dfNoDataValue = -1000;

    // An attempt to capture the chart scale.
    OGRLayer* DID_layer = ds->GetLayerByName("DSID");
    int chartScale;
    chartScale = ds->GetLayerByName("DSID")->GetFeature(0)->GetFieldAsInteger("DSPM_CSCL");
    cout << "Chart Scale: " << chartScale << endl;

    // If grid_size was not set explicitly (-1), then set it to 0.25 mm at chart scale.
    // otherwise use the specified value.
    if (grid_size == -1) {
        grid_size = float(chartScale)/1000.0 * 0.25;
    }

    int x_res = int(round((maxX - minX)/grid_size));
    int y_res = int(round((maxY - minY)/grid_size));

    griddedData.resize(y_res*x_res);
    vector<float> new_rast_data(y_res*x_res,-10.0);

    //new_rast_data.resize(y_res*x_res);
    //new_rast_data.

    Map.clear();
    Map.resize(x_res, vector<int> (y_res,0));
    //Map.resize(y_res,x_res);

    /*
    OGRSpatialReference UTM = geod.getUTM();
    // Build vector of layers
    vector<OGRLayer*> layers;
    layers.push_back(land);

    // Set spatial Reference
    char    *spat_ref = NULL;
    UTM.exportToWkt(&spat_ref);

    // Set the rasterize options
    char** opt = nullptr;
    opt = CSLSetNameValue(opt, "ALL_TOUCHED", "TRUE");

    // Build the geotransformation matrix of the target raster
    vector<double> geoTransform = {minX, grid_size, 0, maxY, 0, -grid_size};
    if (GDALRasterizeLayersBuf(Map.data(), x_res, y_res, GDT_Int32, 0, 0, 1, (OGRLayerH*)&layers[0], spat_ref, geoTransform.data(), NULL, NULL, landZ, opt, NULL, NULL ) == CE_Failure)
    */

    printf("Starting to grid data\n\tData points: %d, Grid: %dx%d %d\n", static_cast<int>(X.size()), y_res,x_res, depth.size());

    // Grid the data
    if (GDALGridCreate(GGA_Linear, &options, static_cast<int>(X.size()), X.data(), Y.data(), depth.data(), minX, maxX, minY, maxY, x_res, y_res, GDT_Int32, griddedData.data(), NULL, NULL) == CE_Failure)
    {
        cout << "Gridding Failed" << endl;
        exit(1);
    }

    cout << "Gridded" << endl;

    // Here we want to write out the gridded data before updating it in a final step. At this point the grid
    // is upside down. This loop swaps it.
    vector<int> gridded_data_temp(y_res*x_res);
    int rasterIndex=0;
    for (int i =0; i<griddedData.size(); i++)
    {
        rasterIndex2gridIndex(i,rasterIndex, x_res, y_res);
        //row_major2grid(i, x, y, x_res);
        gridded_data_temp[rasterIndex] = griddedData[i];
    }

    // Write out the raw gridded data before applying masks (in updateMap below) to fix depth area anomalies.
    fs::path ENC_filename_path = fs::path(ENC_filename);
    string file_basename = ENC_filename_path.stem().string() + "_raw.tiff";
    ENC_filename_path = fs::path(file_basename);
    fs::path tiffPath = fs::path(MOOS_Path);
    tiffPath /= ENC_filename_path.filename();
    writeRasterData(tiffPath.string(), x_res, y_res, gridded_data_temp);

    // Update map by masking against depth areas to fix depth area anomalies.
    // This step masks the gridded data, taking the more conservative of the individual polygons
    // points, depth areas and enc outline data. This ensures that the final grid does not loose
    // fidelity of navigational hazards in the buffering of adjacent ones.
    updateMap(poly_rasterdata, depth_area_rasterdata, ENC_outline_rasterdata, point_rasterdata, new_rast_data, x_res, y_res);

    if (csv)
        write2csv(poly_rasterdata, depth_area_rasterdata, point_rasterdata, x_res, y_res, mat);

    // MODIFIED: VES 9/29/2017
    // Output tiff filename is now based on the name of the ENC, with .000 replaced with .tiff
    ENC_filename_path = fs::path(ENC_filename);
    ENC_filename_path.replace_extension(".tiff");
    tiffPath = fs::path(MOOS_Path);
    tiffPath /= ENC_filename_path.filename();

    writeRasterData(tiffPath.string(), x_res, y_res, new_rast_data);
    clock_t end = clock();
    double total_time = double(end - start) / CLOCKS_PER_SEC;
    cout << "Elapsed Time: " << total_time << " seconds." << endl;
}

vector<vector<int> > Grid_Interp::transposeMap()
{
    vector <vector<int> > t_map (Map[0].size(), vector<int> (Map.size(),0));

    for (int i =0; i<Map.size(); i++)
    {
        for (int j=0; j<Map[0].size(); j++)
        {
            t_map[j][i] = Map[i][j];
        }
    }
    return t_map;
}

// Build the 2D map from the 4 1D vectors
void Grid_Interp::updateMap(vector<int> &poly_data, vector<int> &depth_data, vector<int> &outline_data, vector<int> &point_data, vector<float> &new_rast_data, int x_res, int y_res)
{
    int x, y;
    int rasterIndex = 0;

    // Make sure that the size of all of the 1D vectors are the same
    if ((poly_data.size() == depth_data.size()) && (poly_data.size() == griddedData.size()) &&
            (poly_data.size() == outline_data.size())&&((simpleGrid)||(poly_data.size() == point_data.size())))
    {
        for (int i =0; i<griddedData.size(); i++)
        {
            rasterIndex2gridIndex(i,rasterIndex, x_res, y_res);
            row_major2grid(i, x, y, x_res);

            // Make sure that the datapoint is inside of the ENC
            if (outline_data[rasterIndex] == 1)
            {
                // Check to see if the index is on land.
                if (poly_data[rasterIndex] == landZ)
                    Map[y][x] =poly_data[rasterIndex];
                else
                {
                    // If it is not land make sure that the recorded depth is not below the minimum depth given by the depth area
                    if((depth_data[rasterIndex] != -1000) && (depth_data[rasterIndex] > griddedData[i]))
                        Map[y][x] = depth_data[rasterIndex];
                    else
                        Map[y][x] = griddedData[i];

                    // If it not on land then we must still check to see if the depth value is less than the one given by
                    //  the previous statement (for example if it is a wreck or dock)
                    if((poly_data[rasterIndex] != -1000) && (poly_data[rasterIndex] < Map[y][x]))
                    {
                        Map[y][x] = poly_data[rasterIndex];
                    }
                    if (!simpleGrid)
                    {
                       // The last check is to see if the point data depth value is less than the one given by
                       //  the previous statements (ie pontoon or rock)
                       if((point_data[rasterIndex] != -1000) && (point_data[rasterIndex] < Map[y][x]))
                        {
                            Map[y][x] = point_data[rasterIndex];
                        }
                    }
                }
            }
            else
            {
                Map[y][x] = -1000;
            }
            /*
            // Store the Map data so that it can be converted to a raster
            if (Map[y][x] <= landZ)
                new_rast_data[rasterIndex] = NAN;
            else
            */
            new_rast_data[rasterIndex] = 0.01*(1.0*Map[y][x]);
                //new_rast_data[rasterIndex] = Map[y][x];//0.01*(1.0*Map[y][x]);
        }

    }
    else
    {
        printf("Poly: %d, Depth: %d, Grid:%d, ENC Outline:%d\n", (int) poly_data.size(), (int) depth_data.size(), (int) griddedData.size(), (int) outline_data.size());
        cout << "The vectors are the wrong size. Exiting...\n";
        exit(1);
    }
}

// Save the 1D vectors (polygon, depth area, pre-processed interpolated grid) and the 2D
//  post-processed interpolated grid as csv files
void Grid_Interp::write2csv(vector<int> &poly_data, vector<int> &depth_data, vector<int> &point_data, int x_res, int y_res, bool mat)
{
    ofstream combined, grid, poly, DA, point;
    int x,y;
    int prev_x = 0;
    int rasterIndex = 0;

    // FIX: Fix this path concatination with Boost:filesystem paths.
    string postcsv= MOOS_Path+"post.csv";
    string precsv= MOOS_Path+"pre.csv";
    string polycsv= MOOS_Path+"poly.csv";
    string DAcsv= MOOS_Path+"DA.csv";
    string pntcsv = MOOS_Path+"point.csv";


    // File to write data to csv format
    combined.open(postcsv.c_str());
    grid.open(precsv.c_str());
    poly.open(polycsv.c_str());
    DA.open(DAcsv.c_str());
    point.open(pntcsv.c_str());

    cout << "Writing data to CSV files." << endl;

    // Make sure that the size of all of the 1D vectors are the same
    if ((poly_data.size() == depth_data.size()) && (poly_data.size() == griddedData.size()))
    {
        for (int i =0; i<griddedData.size(); i++)
        {
            rasterIndex2gridIndex(i,rasterIndex, x_res, y_res);
            row_major2grid(i, x, y, x_res);

            if (x!=prev_x)
            {
                grid << "\n";
                poly << "\n";
                DA << "\n";
                combined << "\n";
                point << "\n";
            }
            // Store the data in a csv
            combined << Map[y][x]<< ",";
            grid << griddedData[i] << ",";
            poly << poly_data[rasterIndex] << ",";
            DA << depth_data[rasterIndex] << ",";
            point << point_data[rasterIndex] << ",";

            /*

            if (griddedData[i] == -1000)
                griddedData[i] = NAN;
            else
                grid << griddedData[i] << ",";

            if (poly_data[rasterIndex] == -1000)
                poly_data[rasterIndex] = NAN;
            else
                poly << poly_data[rasterIndex] << ",";

            if (depth_data[rasterIndex] == -1000)
                depth_data[rasterIndex] = NAN;
            else
                DA << depth_data[rasterIndex] << ",";

            if (point_data[rasterIndex] == -1000)
                point_data[rasterIndex] = NAN;
            else
                point << point_data[rasterIndex] << ",";
            */
            prev_x =x;
        }
        combined.close();
        grid.close();
        poly.close();
        DA.close();
        point.close();

        // Write these files to .mat
        if (mat)
        {
            cout << "Writing data to .mat files." << endl;
            writeMat("post");
            writeMat("pre");
            writeMat("poly");
            writeMat("DA");
            //writeMat("point");
        }
    }
    else
    {
        printf("Poly: %d, Depth: %d, Grid:%d\n", (int) poly_data.size(), (int) depth_data.size(), (int) griddedData.size());
        cout << "The vectors are the wrong size. Exiting...\n";
        exit(1);
    }
}

void Grid_Interp::writeMat(string filename)
{
    string toMat = "python " + MOOS_Path+"scripts/csv2mat.py " + MOOS_Path+filename+".csv " + MOOS_Path+filename+".mat";
    cout << toMat << endl;
    system(toMat.c_str());
}

// Get the minimum and maximum x/y values (in local UTM) of the ENC
void Grid_Interp::getENC_MinMax(GDALDataset* ds)
{
    OGRLayer* layer;
    OGRFeature* feat;
    OGRLinearRing* ring;
    OGRGeometry* geom;
    OGRPolygon* coverage;

    vector<double> x,y;
    double UTM_x, UTM_y;
    double remainder;

    layer = ds->GetLayerByName("M_COVR");
    layer->ResetReading();
    feat = layer->GetNextFeature();

    // Cycle through the points and store all x and y values of the layer
    for (int i=0; i<layer->GetFeatureCount(); i++)
    {
        if (feat->GetFieldAsDouble("CATCOV") == 1)
        {
            geom = feat->GetGeometryRef();
            coverage = ( OGRPolygon * )geom;
            ring = coverage->getExteriorRing();
            for (int j =0;  j<ring->getNumPoints(); j++)
            {
                geod.LatLong2LocalUTM(ring->getY(j), ring->getX(j), UTM_x, UTM_y);
                x.push_back(UTM_x);
                y.push_back(UTM_y);
            }
        }
        feat = layer->GetNextFeature();
    }

    // Find the min and max of the X and Y values
    minX = *min_element(x.begin(), x.end());
    maxX = *max_element(x.begin(), x.end());
    minY = *min_element(y.begin(), y.end());
    maxY = *max_element(y.begin(), y.end());

    // Make sure each one of min/max values are divisable by grid_size
    remainder = fmod(minX, grid_size);
    if (remainder != 0)
        minX -= remainder+grid_size;

    remainder = fmod(minY, grid_size);
    if (remainder != 0)
        minY -= remainder+grid_size;

    remainder = fmod(maxX, grid_size);
    if (remainder != 0)
        maxX += grid_size - remainder;

    remainder = fmod(maxY, grid_size);
    if (remainder != 0)
        maxY += grid_size - remainder;

}

// This function takes in an OGRLayer and sorts out which function it should
//  use based upon the geometry type of the feature for interpolation
void Grid_Interp::layer2XYZ(OGRLayer* layer, string layerName)
{
  OGRFeature* feat;
  OGRGeometry* geom;
  string geom_name;

  if (layer != NULL)
    {
      layer->ResetReading();
      while( (feat = layer->GetNextFeature()) != NULL )
        {
          geom = feat->GetGeometryRef();
          geom_name = geom->getGeometryName();
          if (geom_name == "MULTIPOINT")
            multipointFeat(feat, geom);
          else if (geom_name == "POLYGON")
            polygonFeat(feat, geom, layerName);
          else if (geom_name == "LINESTRING")
            lineFeat(feat, geom, layerName);
          else if (geom_name == "POINT")
            pointFeat(feat, geom, layerName);
        }
    }
  else
    cout << layerName << " is not in the ENC!" << endl;
}

// This function converts the mulitpoints into Local UTM, and
//  stores the vertices for interpolation
void Grid_Interp::multipointFeat(OGRFeature* feat, OGRGeometry* geom)
{
    OGRGeometry* poPointGeometry;
    OGRMultiPoint *poMultipoint;
    OGRPoint * poPoint;

    double lat,lon, x, y, z;

    // Cycle through all points in the multipoint and store the xyz location of each point
    geom = feat->GetGeometryRef();
    poMultipoint = ( OGRMultiPoint * )geom;
    for(int iPnt = 0; iPnt < poMultipoint->getNumGeometries(); iPnt++ )
      {
        // Make the point
        poPointGeometry = poMultipoint ->getGeometryRef(iPnt);
        poPoint = ( OGRPoint * )poPointGeometry;
        // Get the location in the grid coordinate system
        lon = poPoint->getX();
        lat = poPoint->getY();
        geod.LatLong2LocalUTM(lat,lon, x,y);

        // Get depth in cm
        z= poPoint->getZ()*100;
        if (!z)
            return;
        else
            depth.push_back(z);

        // FIX: IS THIS INTENDED TO PUSH X AND Y EVEN IF THERE IS NO DEPTH???
        X.push_back(x);
        Y.push_back(y);
      }
}

// This function converts the lines into Local UTM, segements the lines, and
//  stores the vertices for interpolation
void Grid_Interp::lineFeat(OGRFeature* feat, OGRGeometry* geom, string layerName)
{
    OGRLineString *UTM_line;
    OGRGeometry *geom_UTM, *buff_geom;
    //OGRPoint pt;
    OGRFeature *new_feat;
    OGRPolygon *Buff_line;

    double x,y,z;
    bool WL_flag;
    int WL;

    string Z;

    geom_UTM = geod.LatLong2UTM(geom);
    UTM_line = ( OGRLineString * )geom_UTM;
    buff_geom = UTM_line->Buffer(buffer_size);
    Buff_line = (OGRPolygon *) buff_geom;
    Buff_line->segmentize(grid_size);

    if (layerName=="DEPCNT")
    {
        z = feat->GetFieldAsDouble("VALDCO")*100;
        UTM_line->segmentize(grid_size);
        // Get the location in the grid coordinate system
        for (int j=0; j<UTM_line->getNumPoints(); j++)
        {
            x = UTM_line->getX(j)-geod.getXOrigin();// convert to local UTM
            y = UTM_line->getY(j)-geod.getYOrigin();// convert to local UTM

            // Store the XYZ of the vertices
            X.push_back(x);
            Y.push_back(y);
            depth.push_back(z); // in cm
        }
        return; // do not store the line as a polygon
    }
    else if ((layerName =="PONTON")||(layerName =="FLODOC")||(layerName =="DYKCON")||(layerName == "LNDARE"))
    {
        z = landZ;
    }
    else if ((layerName == "OBSTRN"))
    {
        Z = feat->GetFieldAsString("VALSOU");
        WL_flag = Z.empty();
        if (WL_flag)
        {
            WL = feat->GetFieldAsDouble("WATLEV");
            z = calcDepth(WL)*100;
            if (z==-10000)
                return;
        }
        else
            z = feat->GetFieldAsDouble("VALSOU")*100;
    }
    /*
        UTM_line->segmentize(grid_size/5);
        // Place all lines into the point shapefile
        for (int j=0; j<UTM_line->getNumPoints(); j++)
        {
            x = UTM_line->getX(j);// convert to local UTM
            y = UTM_line->getY(j);// convert to local UTM
            pt = OGRPoint(x,y);
            new_feat =  OGRFeature::CreateFeature(feat_def_pnt);
            new_feat->SetField("Depth", z);
            new_feat->SetGeometry(&pt);
            // Build the new feature
            if( layer_pnt->CreateFeature( new_feat ) != OGRERR_NONE )
            {
                printf( "Failed to create feature in polygon shapefile.\n" );
                exit( 1 );
            }
            OGRFeature::DestroyFeature(new_feat);
        }
        if (WL_flag)
            return;
    }
    */
    else
    {
        cout << "Unknown Line Layer: " << layerName << endl;
        return;
    }
    // If it is a valid layer that we know how to deal with then store the buffered line
    //  (which is now a polygon)
    new_feat =  OGRFeature::CreateFeature(feat_def_poly);
    new_feat->SetField("Depth", z);
    new_feat->SetGeometry(Buff_line);
    // Build the new feature
    if( layer_poly->CreateFeature( new_feat ) != OGRERR_NONE )
    {
        printf( "Failed to create feature in polygon shapefile.\n" );
        exit( 1 );
    }
    OGRFeature::DestroyFeature(new_feat);

    /*
    UTM_line->segmentize(grid_size);
    // Get the location in the grid coordinate system
    for (int j=0; j<UTM_line->getNumPoints(); j++)
    {
        x = UTM_line->getX(j)-geod.getXOrigin();// convert to local UTM
        y = UTM_line->getY(j)-geod.getYOrigin();// convert to local UTM

        // Store the XYZ of the vertices
        X.push_back(x);
        Y.push_back(y);
        depth.push_back(z); // in cm
    }
    */
}

void Grid_Interp::pointFeat(OGRFeature* feat, OGRGeometry* geom, string layerName)
{
    OGRPoint *poPoint, pt;
    int WL;
    double lat,lon, x,y, z;
    OGRFeature *new_feat;




    poPoint = poPoint = ( OGRPoint * )geom;
    lon = poPoint->getX();
    lat = poPoint->getY();

    // VES
    // VES BUFFERING POINT FEATURES
    OGRGeometry *geom_UTM, *buff_geom;
    OGRLineString *UTM_line;
    OGRPolygon *Buff_line;
    geom_UTM = geod.LatLong2UTM(geom);
    UTM_line = ( OGRLineString * )geom_UTM;
    // Buffer at 8X the grid size, which should be 2mm at chart scale when gridded at 0.25 mm.
    buff_geom = UTM_line->Buffer(grid_size * 8.0);
    Buff_line = (OGRPolygon *) buff_geom;
    Buff_line->segmentize(grid_size);

    bool WL_flag = false;
    string Z;


    if (layerName == "LNDARE")
        z = landZ;

    else if ((layerName == "UWTROC")||(layerName == "WRECKS")||(layerName == "OBSTRN"))
    {
        Z = feat->GetFieldAsString("VALSOU");
        WL_flag = Z.empty();
        if (WL_flag)
        {
            WL = feat->GetFieldAsDouble("WATLEV");
            z = calcDepth(WL)*100;
            if (z==-10000)
                return;
        }
        else
            z = feat->GetFieldAsDouble("VALSOU")*100;

        // VES Commented this out...
        // Put into point layer
        //geod.LatLong2UTM(lat, lon, x,y);
        //pt = OGRPoint(x,y);
        //new_feat =  OGRFeature::CreateFeature(feat_def_pnt);
        //new_feat->SetField("Depth", z);
        //new_feat->SetGeometry(&pt);

        // VES Added this to insert buffered point as polygon.
        new_feat =  OGRFeature::CreateFeature(feat_def_pnt);
        new_feat->SetField("Depth", z);
        new_feat->SetGeometry(Buff_line);

        // Build the new feature
        if( layer_pnt->CreateFeature( new_feat ) != OGRERR_NONE )
        {
            printf( "Failed to create feature in polygon shapefile.\n" );
            exit( 1 );
        }
        OGRFeature::DestroyFeature(new_feat);

        // Only use the feature to interpolate if the depth is known
        if (WL_flag)
            return;
    }
    else
    {
        cout << "Unknown point layer: " << layerName << endl;
        return;
    }


    // Get the location in the grid coordinate system
    geod.LatLong2LocalUTM(lat, lon, x,y);

    // Store the XYZ of the vertices
    X.push_back(x);
    Y.push_back(y);
    depth.push_back(z);
}

// Store the polygon as shapefiles for rasterization and store the vertices for interpolation
void Grid_Interp::polygonFeat(OGRFeature* feat, OGRGeometry* geom, string layerName)
{
    OGRPolygon *UTM_poly, *poly;
    OGRGeometry *geom_UTM, *buff_geom;
    OGRFeature *new_feat;

    double z;
    string Z;
    int WL;
    poly = ( OGRPolygon * )geom;

    geom_UTM = geod.LatLong2UTM(geom);
    UTM_poly = ( OGRPolygon * )geom_UTM;

    // Build the layer for the area covered by the ENC
    if (layerName == "M_COVR")
    {
        if (feat->GetFieldAsDouble("CATCOV") == 1)
        {
            UTM_poly = ( OGRPolygon * )geom_UTM;
            UTM_poly->segmentize(grid_size);
            new_feat =  OGRFeature::CreateFeature(feat_def_outline);
            new_feat->SetField("Inside_ENC", 1);
            new_feat->SetGeometry(UTM_poly);

            // Build the new feature
            if( layer_outline->CreateFeature( new_feat ) != OGRERR_NONE )
            {
                printf( "Failed to create feature in polygon shapefile.\n" );
                exit( 1 );
            }
            OGRFeature::DestroyFeature(new_feat);
        }
    }

    buff_geom = geom_UTM->Buffer(buffer_size);

    UTM_poly = ( OGRPolygon * )buff_geom;
    UTM_poly->segmentize(grid_size);

    if (layerName == "LNDARE")
    {
        z = landZ;
        new_feat =  OGRFeature::CreateFeature(feat_def_poly);
        new_feat->SetField("Depth", z);
        new_feat->SetGeometry(UTM_poly);

        // Build the new feature
        if( layer_poly->CreateFeature( new_feat ) != OGRERR_NONE )
        {
            printf( "Failed to create feature in polygon shapefile.\n" );
            exit( 1 );
        }
        // To limit the amount of points stored, only store the vertices of the
        //  buffered polygon.
        store_vertices(UTM_poly,z);
        OGRFeature::DestroyFeature(new_feat);
    }
    else if ((layerName == "WRECKS")||(layerName == "OBSTRN"))
    {
        Z = feat->GetFieldAsString("VALSOU");
        if (Z.empty())
        {
            WL = feat->GetFieldAsDouble("WATLEV");
            z = calcDepth(WL)*100;
            if (z==-10000)
                return;
        }
        else
            z = feat->GetFieldAsDouble("VALSOU");

        new_feat =  OGRFeature::CreateFeature(feat_def_poly);
        new_feat->SetField("Depth", z);
        new_feat->SetGeometry(UTM_poly);

        // Build the new feature
        if( layer_poly->CreateFeature( new_feat ) != OGRERR_NONE )
        {
            printf( "Failed to create feature in polygon shapefile.\n" );
            exit( 1 );
        }
        OGRFeature::DestroyFeature(new_feat);
    }
    else if (layerName == "DEPARE")
    {
        z = feat->GetFieldAsDouble("DRVAL1")*100; // Minimum depth in range
        if (!z)
            return;

        new_feat =  OGRFeature::CreateFeature(feat_def_depth);
        new_feat->SetField("Depth", z);
        new_feat->SetGeometry(UTM_poly);

        // Build the new feature
        if( layer_depth->CreateFeature( new_feat ) != OGRERR_NONE )
        {
            printf( "Failed to create feature in depth shapefile.\n" );
            exit( 1 );
        }
        OGRFeature::DestroyFeature(new_feat);
    }
    else if ((layerName == "PONTON")||(layerName =="FLODOC")||(layerName == "DYKCON"))
    {
        z = landZ;
        new_feat =  OGRFeature::CreateFeature(feat_def_poly);
        new_feat->SetField("Depth", z);
        new_feat->SetGeometry(UTM_poly);

        // Build the new feature
        if( layer_poly->CreateFeature( new_feat ) != OGRERR_NONE )
        {
            printf( "Failed to create feature in polygon shapefile.\n" );
            exit( 1 );
        }
        OGRFeature::DestroyFeature(new_feat);
    }
    else
    {
        if (layerName != "M_COVR")
        {
            cout << "Unknown Polygon layer: " << layerName << endl;
            return;
        }
    }
}

// This function stores the vertices of a polygon (that is in UTM) for interpolation
void Grid_Interp::store_vertices(OGRPolygon *UTM_Poly, double z)
{
    OGRLinearRing *ring;

    ring = UTM_Poly->getExteriorRing();

    for (int j = 0 ; j< ring->getNumPoints(); j++)
    {
        X.push_back(ring->getX(j)-geod.getXOrigin());// convert to local UTM
        Y.push_back(ring->getY(j)-geod.getYOrigin());// convert to local UTM
        depth.push_back(z);
    }

}

void Grid_Interp::getRasterData(string filename, int &nXSize, int &nYSize, vector<int>&RasterData)
{
    GDALDataset  *poDataset;
    GDALRasterBand *poRasterBand;

    // MODIFIED: Removed fixed paths. VES 9/28/2017
    //string full_Filename = MOOS_Path+"src/ENCs/Grid/" +filename;
    //FIX: THis needs to be done with boost:filesystem paths so handle / gracefully in all cases.
    string full_Filename = MOOS_Path + filename;


    GDALAllRegister();
    poDataset = (GDALDataset *) GDALOpen( full_Filename.c_str(), GA_ReadOnly );

    poRasterBand = poDataset -> GetRasterBand(1);

    nXSize = poRasterBand->GetXSize(); // width
    nYSize = poRasterBand->GetYSize(); // height

    // Resize the vector to fit the raster dataset
    RasterData.resize(nXSize*nYSize);

    // Read the raster into a 1D row-major vector where it is organized in left to right,top to bottom pixel order
    if( poRasterBand->RasterIO( GF_Read, 0, 0, nXSize, nYSize, RasterData.data(), nXSize, nYSize, GDT_Int32, 0, 0 ) != CE_None )
    {
        printf( "Failed to access raster data.\n" );
        GDALClose(poDataset);
        exit( 1 );
    }

    GDALClose(poDataset);
}

void Grid_Interp::writeRasterData(string filename, int nXSize, int nYSize, vector<float>&RasterData)
{
    GDALDataset *poDataset;
    GDALRasterBand *poBand;
    OGRSpatialReference oSRS;
    GDALDriver *poDriver;
    char *pszSRS_WKT = NULL;
    char **papszOptions = NULL;
    const char *pszFormat = "GTiff";
    int i,j;

    // MODIFIED: Removed fixed paths. VES 9/28/2017
    //string full_Filename = MOOS_Path+"src/ENCs/Grid/" +filename;
    string full_Filename = filename;

    // Open file for writing
    poDriver = GetGDALDriverManager()->GetDriverByName(pszFormat);
    poDataset = poDriver->Create( full_Filename.c_str(), nXSize, nYSize, 1, GDT_Float32, papszOptions );

    // Set geo transform
    double adfGeoTransform[6] = {minX+geod.getXOrigin(), grid_size, 0, maxY+geod.getYOrigin(), 0, -grid_size};
    poDataset->SetGeoTransform( adfGeoTransform );

    // Set Coordinate system
    oSRS = geod.getUTM();
    oSRS.exportToWkt( &pszSRS_WKT );
    poDataset->SetProjection( pszSRS_WKT );
    CPLFree( pszSRS_WKT );

    // Write the file
    poBand = poDataset->GetRasterBand(1);
    // MODIFIED: VES 9/28/2017
    // Divided by 100 to get back to meters before writing tiff file. SI units are good.
    //for (i=0; i<=RasterData.size();i++){
    //        RasterData[i] = RasterData[i] / 100.0;
    //}

    poBand->RasterIO( GF_Write, 0, 0, nXSize, nYSize,
                      RasterData.data(), nXSize, nYSize, GDT_Float32, 0, 0 );

    // Once we're done, close properly the dataset
    GDALClose( (GDALDatasetH) poDataset );
}

void Grid_Interp::writeRasterData(string filename, int nXSize, int nYSize, vector<int>&RasterData)
{
    GDALDataset *poDataset;
    GDALRasterBand *poBand;
    OGRSpatialReference oSRS;
    GDALDriver *poDriver;
    char *pszSRS_WKT = NULL;
    char **papszOptions = NULL;
    const char *pszFormat = "GTiff";
    int i,j;

    // MODIFIED: Removed fixed paths. VES 9/28/2017
    //string full_Filename = MOOS_Path+"src/ENCs/Grid/" +filename;
    string full_Filename = filename;

    // Open file for writing
    poDriver = GetGDALDriverManager()->GetDriverByName(pszFormat);
    poDataset = poDriver->Create( full_Filename.c_str(), nXSize, nYSize, 1, GDT_Float32, papszOptions );

    // Set geo transform
    double adfGeoTransform[6] = {minX+geod.getXOrigin(), grid_size, 0, maxY+geod.getYOrigin(), 0, -grid_size};
    poDataset->SetGeoTransform( adfGeoTransform );

    // Set Coordinate system
    oSRS = geod.getUTM();
    oSRS.exportToWkt( &pszSRS_WKT );
    poDataset->SetProjection( pszSRS_WKT );
    CPLFree( pszSRS_WKT );

    // Write the file
    poBand = poDataset->GetRasterBand(1);
    // MODIFIED: VES 9/28/2017
    // Divided by 100 to get back to meters before writing tiff file. SI units are good.
    //for (i=0; i<=RasterData.size();i++){
    //        RasterData[i] = RasterData[i] / 100.0;
    //}

    poBand->RasterIO( GF_Write, 0, 0, nXSize, nYSize,
                      RasterData.data(), nXSize, nYSize, GDT_Int32, 0, 0 );

    // Once we're done, close properly the dataset
    GDALClose( (GDALDatasetH) poDataset );
}

// Cycle through the raster data and store the xyz coordinates to be used in the interpolation
void Grid_Interp::raster2XYZ(vector<int>& RasterData, int nXSize)
{
    double x,y;

    // Store the Polygon data before gridding
    for (int i=0; i<RasterData.size(); i++)
    {
        // Convert the coordinates from 1D to 2D then grid to local_UTM
        row_major_to_2D(i, x, y, nXSize);
        X.push_back(x);
        Y.push_back(y);
        depth.push_back(RasterData[i]);
    }
}

// Rasterize the shapefile using the commandline function gdal_rasterize
void Grid_Interp::rasterizeSHP(string outfilename, string infilename, string attribute)
{
    // Strings to hold the data for the input/output filenames for gdal_rasterize
    // MODIFIED: Removed fixed paths. VES 9/28/2017
    // TO FIX: Put these in a temp directory and clean them up when done.
    //string full_inFilename = MOOS_Path+"src/ENCs/Grid/" +infilename;
    //string full_outFilename = MOOS_Path+"src/ENCs/Grid/" +outfilename;
    string full_inFilename = MOOS_Path + infilename;
    string full_outFilename = MOOS_Path + outfilename;
    string filenames = full_inFilename + " " + full_outFilename;

    // String to hold the data for the georeferenced extents for gdal_rasterize
    string georef_extent = "-te "+to_string(minX+geod.getXOrigin())+ " "+to_string(minY+geod.getYOrigin())+ " "
            +to_string(maxX+geod.getXOrigin())+ " "+to_string(maxY+geod.getYOrigin())+ " ";

    // String for all the options for gdal_rasterize
    // FIX: EPSG is hard coded here. This must be determined from the geom variable.
    string options = "-a_nodata -1000 -at -a " + attribute + " -tr " + to_string(grid_size)+ " " +
            to_string(grid_size)+ " -a_srs EPSG:2219 -ot Int32 " + georef_extent + filenames;

    string rasterize = "gdal_rasterize " + options;

    cout << "Rasterizing " << outfilename << endl;
    cout << rasterize << endl;

    system(rasterize.c_str());
}

// Converts depths to
double Grid_Interp::calcDepth(int WL)
{
    double WL_depth = 0;
    double feet2meters = 0.3048;

    if (WL == 2) // Always Dry
    {
        // At least 2 feet above MHW. Being shoal biased, we will take the
        //   object's "charted" depth as 2 feet above MHW
        WL_depth = -(1.0*feet2meters+MHW_Offset);
    }

    else if (WL == 3) // Always underwater
    {
        // At least 1 foot below MLLW. Being shoal biased, we will take the
        //   object's "charted" depth as 1 foot below MLLW
        WL_depth = 1.0*feet2meters;
    }

    else if (WL == 4) // Covers and Uncovers
    {
        // The range for these attributes 1 foot below MLLW to 1 foot above MHW
        //   Therefore, we will be shoal biased and take 1 foot above MHW as the
        //   object's "charted" depth.
        WL_depth = -(1.0*feet2meters+MHW_Offset);
    }

    else if (WL == 5) // Awash
    {
        // The range for these attributes 1 foot below MLLW to 1 foot above
        //   MLLW. Therefore, we will be shoal biased and take 1 foot above MLLW
        //   as the object's "charted" depth.
        WL_depth = -1.0*feet2meters;
    }

    else
    {
        // All other Water levels (1, 6, and 7) don't have a quantitative
        //   descriptions. Therefore we will set it to -100 and ignore it
        //   in the next step.
        WL_depth = -100;
    }

    return WL_depth;
}

void Grid_Interp::xy2grid(double x, double y, int &gridX, int &gridY)
{
  // Converts the local UTM to the grid coordinates.
  gridX = int(round((x-minX-grid_size/2.0)/grid_size));
  gridY = int(round((y-minY-grid_size/2.0)/grid_size));
}

void Grid_Interp::grid2xy(int gridX, int gridY, double &x, double &y)
{
  // Converts the grid coordinates to the local UTM.
  x = int(round(gridX*grid_size+ minX+ grid_size/2.0));
  y = int(round(gridY*grid_size+ minY+ grid_size/2.0));
}

void Grid_Interp::LatLong2Grid(double lat, double lon, int &gridX, int &gridY)
{
    double x,y;
    geod.LatLong2LocalUTM(lat,lon, x,y);
    xy2grid(x,y, gridX, gridY);
}


void Grid_Interp::Grid2LatLong(int gridX, int gridY, double &lat, double &lon)
{
    double x,y;
    grid2xy(gridX, gridY, x,y);
    geod.LocalUTM2LatLong(x,y, lat,lon);
}

void Grid_Interp::row_major_to_2D(int index, double &x, double &y, int numCols)
{
    int gridX, gridY;
    gridY = index%numCols;
    gridX = (index-gridY)/numCols;
    grid2xy(gridX, gridY, x,y);
}

void Grid_Interp::row_major2grid(int index, int &gridX, int &gridY, int numCols)
{
    gridY = index%numCols;
    gridX = (index-gridY)/numCols;
}

void Grid_Interp::rasterIndex2gridIndex(int rasterIndex, int &gridIndex, int x_res, int y_res)
{
    int x,y;

    row_major2grid(rasterIndex, x,y, x_res);

    // Raster indexing is top to bottom and the grid indexing is bottom to top
    gridIndex = (y_res-1 - x)*x_res +y;
}

