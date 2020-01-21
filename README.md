# encgrid
A tool to convert an Electronic Nautical Chart into a raster depth map based on Sam Reed's Master's Thesis. 

To make encgrid:
git clone https://github.com/valschmidt/encgrid.git
  cd encgrid
  mkdir build
  cd build
  cmake ../
  make
  
To run encgrid:
  ./encgrid -h
    USAGE: encgrid -f ENC_FILENAME -b buffer -r resolution
    
To download ENCs on which to run encgrid, goto. 

Example:

  ../build/encgrid/encgrid -f ENC-ROOT/US5NH02M/US5NH02M.000 -b1 -r5
  ENCFilename: ENC-ROOT/US5NH02M/US5NH02M.000
  Buffer     : 1
  Resolution : 5
  ENC-ROOT/US5NH02M/US5NH02M.000
  ./US5NH02M
  Success
  Before
  afterFLODOC is not in the ENC!
  Rasterizing polygon.tiff
  gdal_rasterize -a_nodata -1000 -at -a Depth -tr 5.000000 5.000000 -a_srs EPSG:2219 -ot Int32 -te 356176.352170 4757784.925223 372756.352170 4783834.925223 ./US5NH02M/polygon.shp ./US5NH02M/polygon.tiff
  0...10...20...30...40...50...60...70...80...90...100 - done.
  Rasterizing depth_area.tiff
  gdal_rasterize -a_nodata -1000 -at -a Depth -tr 5.000000 5.000000 -a_srs EPSG:2219 -ot Int32 -te 356176.352170 4757784.925223 372756.352170 4783834.925223 ./US5NH02M/depth.shp ./US5NH02M/depth_area.tiff
  0...10...20...30...40...50...60...70...80...90...100 - done.
  Rasterizing outline.tiff
  gdal_rasterize -a_nodata -1000 -at -a Inside_ENC -tr 5.000000 5.000000 -a_srs EPSG:2219 -ot Int32 -te 356176.352170 4757784.925223 372756.352170 4783834.925223 ./US5NH02M/outline.shp ./US5NH02M/outline.tiff
  0...10...20...30...40...50...60...70...80...90...100 - done.
  Rasterizing point.tiff
  gdal_rasterize -a_nodata -1000 -at -a Depth -tr 5.000000 5.000000 -a_srs EPSG:2219 -ot Int32 -te 356176.352170 4757784.925223 372756.352170 4783834.925223 ./US5NH02M/point.shp ./US5NH02M/point.tiff
  0...10...20...30...40...50...60...70...80...90...100 - done.
  rasterize
  Chart Scale: 20000
  Starting to grid data
	Data points: 419435, Grid: 5210x3316 419435
  Gridded
  Elapsed Time: 7.44964 seconds.

