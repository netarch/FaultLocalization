#ifndef FAULT_LOCALIZE_TOPOLOGY
#define FAULT_LOCALIZE_TOPOLOGY

#include <iostream>
#include <vector>
#include <queue>
#include <set>
#include <map>
#include <climits>
#include <assert.h>
#include "ns3/flow-monitor-module.h"
#include "ns3/bridge-helper.h"
#include "ns3/bridge-net-device.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/point-to-point-net-device.h"

using namespace ns3;
using namespace std;


// Function to create address string from numbers
//
char * ipOctectsToString(int first, int second, int third, int fourth);

class Topology{
    vector<vector<int> > networkLinks;
    vector<vector<int> > hostsInTor;
    map<int, int> hostToTor;
    set<pair<int, int> > failedLinks;
  	Ipv4AddressHelper address;
    vector<vector<int> > shortest_pathlens;
    //char filename [] = "statistics/File-From-Graph.xml";// filename for Flow Monitor xml output file
    //string topology_filename = "topology/ns3_deg4_sw8_svr8_os1_i1.edgelist";
  public:
    int num_tor = 0; //number of switches in the network
    int total_host = 0;	// number of hosts in the network	
    vector<vector<double> > serverTM;

    void Toplogy() {}

    void readServerTmFromFile(string tmfile);

    void readTopologyFromFile(string topofile);

    void chooseFailedLinks(int nfails);

    double getFailparam(pair<int, int> link);

    //Only the network links in the path
    vector<vector<int> > getPaths(int srchost, int desthost);

    pair<char*, char*> getLinkBaseIpAddress(int sw, int h);

    char* getHostIpAddress(int host);

    pair<char*, char*> getHostBaseIpAddress(int sw, int h);

    pair<int, int> getRackHostsLimit(int rack); //rack contains hosts from [l,r) ===> return (l, r)

    int getHostRack(int host);
    int GetHostNumber(int rack, int ind);
    int OffsetHost(int host);
    int getNumHostsInRack(int rack);
    int getHostIndexInRack(int host);
    int getFirstOctetOfTor(int tor);
    int getSecondOctetOfTor(int tor);

    double get_drop_rate_link_uniform(double min_drop_rate, double max_drop_rate);
    double get_drop_rate_failed_link();
    double GetFailParam(pair<int, int> link, double failparam);
    void connect_switches_and_switches(PointToPointHelper &p2p, Ptr<RateErrorModel> rem, NodeContainer &tors, double failparam);

    void connect_switches_and_hosts(PointToPointHelper &p2p, NodeContainer &tors, NodeContainer *rackhosts, double failparam);

    void compute_all_pair_shortest_pathlens();

    void print_flow_path(int srchost, int desthost);

    void print_flow_info(int srchost, int desthost, int bytes, ApplicationContainer& flowApp);

    void print_ip_addresses();

    void snapshot_flow(int srchost, int desthost, int bytes, ApplicationContainer& flowApp, Time startTime, Time snapshotTime);

    void adapt_network();
private:
    Ptr<TcpSocketBase> getSocketFromBulkSendApp(Ptr<Application> app);
    Ptr<TcpSocketBase> getSocketFromOnOffApp(Ptr<Application> app);
};


void adapt_network(Topology& topology);

#endif
