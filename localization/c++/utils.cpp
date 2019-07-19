#include "utils.h"
#include <string>
#include <sstream>

LogFileData* GetDataFromLogfile(string filename){ 
    
    LogFileData* data = new LogFileData();
    ifstream input( "filename.ext" );
    string line;
    istringstream line_stream (line);
    string op;
    line_string >> op;
    while (getline(infile, line)){
        if (op == "Failing_link"){
            int src, dest;
            float failparam;
            line_stream >> src >> dest >> failparam;
            data->failed_links[PII(src, dest)] = failparam;
            if (VERBOSE){
                cout<< "Failed link "<<src<<" "<<dest<<" "<<failparam<<endl; 
            }
        }
        else if (op == "Flowid="){
            
        }
        else if (op == "Snapshot"){

        }
        else if (op == "flowpath_reverse"){

        }
        else if (op == "flowpath"){
            
        }
        else if (op == "link_statistics"){

        }
    }
    


}
