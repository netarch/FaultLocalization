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

struct Flow{
    int src_node, dest_node, nbytes;
    string data_rate;
    Flow (int src_node_, int dest_node_, int nbytes_, string data_rate_):
          src_node(src_node_), dest_node(dest_node_), nbytes(nbytes_), data_rate(data_rate_) {}
    int second_hop, second_last_hop;
};

const int HOST_OFFSET = 10000;
typedef pair<int, int> PII;

// Function to create address string from numbers
char * ipoctectstostring(int first, int second, int third, int fourth);
// Function to create address string from ns3::Ipv4Address
char * IpvAddressToString(Ipv4Address& ip_address);

class Topology{
    //char filename [] = "statistics/File-From-Graph.xml";// filename for Flow Monitor xml output file
    //string topology_filename = "topology/ns3_deg4_sw8_svr8_os1_i1.edgelist";
  public:
    int num_tor = 0; //number of switches in the network
    int total_host = 0;	// number of hosts in the network	
    vector<Flow> flows;
    set<int> nodes; //list of nodes (switches and hosts) in the network

    void Toplogy() {}
    void ReadFlowsFromFile(string tm_filename);
    void ReadTopologyFromFile(string topology_filename);
    void ChooseFailedLinks(int nfails);

    char* GetNodeIpAddress(int node);
    char* GetSwitchIpAddress(int sw);
    char* GetHostIpAddress(int host);

    bool IsNodeHost(int node);
    int GetHostRack(int host);
    int GetNumHostsInRack(int rack);
    int GetHostIndexInRack(int host);

    void ConnectSwitchesAndSwitches(PointToPointHelper &p2p, NodeContainer &tors, double fail_param);
    void ConnectSwitchesAndHosts(PointToPointHelper &p2p, NodeContainer &tors,
                                 NodeContainer *rack_hosts, double fail_param);

    void PrintAllPairShortestPaths();
    void PrintIpAddresses();
    void SnapshotFlow(Flow flow, ApplicationContainer& flow_app,
                      Time start_time, Time snapshot_time);

    void AdaptNetwork();

    const int start_port = 9; //port for background app
    map<PII, int> endpoint_first_unused_port;
    map<int, int> node_first_unused_port;
    // server nodes are offset
    int GetFirstUnusedPort(int src_node, int dest_node);
    int GetStartPort() { return start_port;}
    int GetFirstUnusedPort(int node) { return node_first_unused_port[node]; }

private:
    //The hosts are offset in the paths
    void GetPathsHost(int src_host, int dest_host, vector<vector<int> >& result);
    void GetPathsRack(int src_host, int dest_host, vector<vector<int> >& result);
    char* GetLinkIpAddressFirst(int sw, int h);
    char* GetLinkIpAddressSecond(int sw, int h);
    pair<char*, char*> GetLinkBaseIpAddress(int sw, int h);
    pair<char*, char*> GetHostBaseIpAddress(int sw, int h);
    int GetFirstOctetOfTor(int tor);
    int GetSecondOctetOfTor(int tor);
    pair<int, int> GetRackHostsLimit(int rack); //rack contains hosts from [l,r) ===> return (l, r)

    double GetDropRateLinkUniform(double min_drop_rate, double max_drop_rate);
    double GetDropRateFailedLink();
    double GetFailParam(pair<int, int> link, double fail_param);

    void ComputeAllPairShortestPathlens();
    Ptr<TcpSocketBase> GetSocketFromOnOffApp(Ptr<Application> app);

    vector<vector<int> > network_links;
    vector<vector<int> > hosts_in_tor;
    map<int, int> host_to_tor;
    set<pair<int, int> > failed_links;
  	Ipv4AddressHelper address;
    vector<vector<int> > shortest_pathlens;
    pair<int, int> PenultimateHops(int src_node, int dest_node);
};

#endif
