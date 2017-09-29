#include <iostream>
#include <algorithm>
#include <string>
#include <vector>
#include <boost/filesystem.hpp>

// Sam's work (mostly)
//#include <astar.h>
#include "geodesy.h"
#include "gridinterp.h"
#include <ctime>

using namespace std;
namespace fs = boost::filesystem;


/* Create a class for parsing command-line input parameters */
/* Carped from stackoverflow */
/*class InputParser{
    public:
        InputParser (int &argc, char **argv){
            for (int i=1; i < argc; ++i)
                this->tokens.push_back(string(argv[i]));
        }
        /// @author iain
        const string& getCmdOption(const string &option) const{
            vector<string>::const_iterator itr;
            itr =  find(this->tokens.begin(), this->tokens.end(), option);
            if (itr != this->tokens.end() && ++itr != this->tokens.end()){
                return *itr;
            }
            static const string empty_string("");
            return empty_string;
        }
        /// @author iain
        bool cmdOptionExists(const string &option) const{
            return find(this->tokens.begin(), this->tokens.end(), option)
                   != this->tokens.end();
        }
    private:
        vector <string> tokens;
};
*/

int main(int argc, char **argv){

    int opt,f,b,r;
    fs::path ENCfilename = "";
    int buffer_dist = 0;
    int grid_size = 5;

    while ((opt = getopt(argc,argv,"f:b:r:")) != EOF)
    {
           switch(opt)
           {
               case 'f':
            {
               f = 1;
               ENCfilename = fs::path(optarg);
               cout <<"ENCFilename: "<< ENCfilename.string() <<endl;
               break;
           }
               case 'b':
           {
                b = 1;
                buffer_dist = atoi(optarg);
                cout <<"Buffer     : "<< buffer_dist <<endl;
                break;
           }
               case 'r':
            {
               r = 1;
               grid_size = atoi(optarg);
               cout << "Resolution : "<< grid_size <<endl ;
               break;
           }
               case 'h':
           {
               fprintf(stderr,"   USAGE: encgrid -f ENC_FILENAME -b buffer -r resolution\n");
           }
               default: cout<<endl; abort();
           }
     }


    bool flag = 0;
    clock_t start;
    double lat, lon;
    double t=0;
    bool build_layer = false;

    double lat1, lat2, lat3, lat4;
    double lon1, lon2, lon3, lon4;

    // This is the position used to determine the UTM zone. Sam has this hard coded into Grid_Interp.
    // FIX: Calculate the mean lat/lon in the data to determine the UTM zone on the fly within Grid_Interp.
    double LatOrigin = 43.0;
    double LongOrigin = -70.0;

    // This is mean high water which is used to estimate depths of features whose features are not specified.
    // I'm not sure how to guess MHW programmatically.
    // FIX: Guess MHW programmatically.
    double MHW = 0.0;

    // Build the gridded ENC
    // This formulation creates a directory into which all temporaray and final output files are put.
    fs::path outputGriddir = fs::path(".");
    cout << ENCfilename.string() << endl;
    outputGriddir /= fs::path(ENCfilename.filename().stem());
    cout << outputGriddir.string() << endl;

    // Create the output directory.
    if(boost::filesystem::create_directory(outputGriddir)) {
           std::cout << "Success" << "\n";
    }

    Grid_Interp grid = Grid_Interp(outputGriddir.string() + '/',
                                   ENCfilename.string(),
                                   grid_size,
                                   buffer_dist,
                                   MHW,
                                   LatOrigin,
                                   LongOrigin);

    // I don't think csv or .mat options are available.
    grid.Run(false,true);//(true, true); // Boleans are t/f build a .csv or .mat files for each raster





    return 0;
}

