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
char * IpOctectsToString(int first, int second, int third, int fourth);

class Topology{
    //char filename [] = "statistics/File-From-Graph.xml";// filename for Flow Monitor xml output file
    //string topology_filename = "topology/ns3_deg4_sw8_svr8_os1_i1.edgelist";
  public:
    int num_tor = 0; //number of switches in the network
    int total_host = 0;	// number of hosts in the network	
    vector<vector<double> > server_TM;

    void Toplogy() {}
    void ReadServerTmFromFile(string tm_filename);
    void ReadTopologyFromFile(string topology_filename);
    void ChooseFailedLinks(int nfails);

    char* GetHostIpAddress(int host);
    int GetHostRack(int host);
    int GetNumHostsInRack(int rack);
    int GetHostIndexInRack(int host);

    void ConnectSwitchesAndSwitches(PointToPointHelper &p2p, NodeContainer &tors, double fail_param);
    void ConnectSwitchesAndHosts(PointToPointHelper &p2p, NodeContainer &tors,
                                 NodeContainer *rack_hosts, double fail_param);

    void PrintFlowPath(int src_host, int dest_host);
    void PrintFlowInfo(int src_host, int dest_host, int bytes, ApplicationContainer& flow_app);
    void PrintIpAddresses();
    void SnapshotFlow(int src_host, int dest_host, int bytes, ApplicationContainer& flow_app,
                      Time start_time, Time snapshot_time);

    void AdaptNetwork();
private:
    //The hosts are offset in the paths
    vector<vector<int> > GetPaths(int src_host, int dest_host);
    pair<char*, char*> GetLinkBaseIpAddress(int sw, int h);
    pair<char*, char*> GetHostBaseIpAddress(int sw, int h);
    int GetFirstOctetOfTor(int tor);
    int GetSecondOctetOfTor(int tor);
    int OffsetHost(int host);
    int GetHostNumber(int rack, int ind);
    pair<int, int> GetRackHostsLimit(int rack); //rack contains hosts from [l,r) ===> return (l, r)

    double GetDropRateLinkUniform(double min_drop_rate, double max_drop_rate);
    double GetDropRateFailedLink();
    double GetFailParam(pair<int, int> link, double fail_param);

    void ComputeAllPairShortestPathlens();
    Ptr<TcpSocketBase> GetSocketFromBulkSendApp(Ptr<Application> app);
    Ptr<TcpSocketBase> GetSocketFromOnOffApp(Ptr<Application> app);

    vector<vector<int> > network_links;
    vector<vector<int> > hosts_in_tor;
    map<int, int> host_to_tor;
    set<pair<int, int> > failed_links;
  	Ipv4AddressHelper address;
    vector<vector<int> > shortest_pathlens;
};


void AdaptNetwork(Topology& topology);

#endif
