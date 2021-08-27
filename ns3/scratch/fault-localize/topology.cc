#include <iostream>
#include <vector>
#include <queue>
#include <set>
#include <map>
#include <random>
#include <iterator>
#include <algorithm>
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
#include "ns3/bulk-send-application.h"
#include "ns3/onoff-application.h"
#include "topology.h"

using namespace ns3;
using namespace std;

// Function to create address string from numbers
char * IpOctectsToString(int first, int second, int third, int fourth){
    char *address =  new char[30];
    char firstOctet[30], secondOctet[30], thirdOctet[30], fourthOctet[30];  
    //address = firstOctet.secondOctet.thirdOctet.fourthOctet;
    assert(first < 256 && second < 256 && third < 256 && fourth < 256);

    bzero(address,30);
    snprintf(firstOctet,10,"%d",first);
    strcat(firstOctet,".");
    snprintf(secondOctet,10,"%d",second);
    strcat(secondOctet,".");
    snprintf(thirdOctet,10,"%d",third);
    strcat(thirdOctet,".");
    snprintf(fourthOctet,10,"%d",fourth);

    strcat(thirdOctet,fourthOctet);
    strcat(secondOctet,thirdOctet);
    strcat(firstOctet,secondOctet);
    strcat(address,firstOctet);

    return address;
}

// Function to create address string from ns3::Ipv4Address
char * IpvAddressToString(Ipv4Address& ip_address){
    uint32_t address_int = ip_address.Get();
    int fourth = address_int % 256;
    address_int >>= 8;
    int third = address_int % 256;
    address_int >>= 8;
    int second = address_int % 256;
    address_int >>= 8;
    int first = address_int % 256;
    return IpOctectsToString(first, second, third, fourth);
}

void Topology::ReadFlowsFromFile(string tm_filename){
    //< read TM from the TM File
    flows.clear();
    ifstream myfile(tm_filename.c_str());
    string line;
    if (myfile.is_open()){
        string delimiter = ",", flow_type;
        while(myfile.good()){
            line.clear();
            getline(myfile, line);
            std::replace(line.begin(), line.end(), ',', ' ');
            if (line.find_first_not_of (' ') == line.npos) continue;
            stringstream ss(line);
            int src_node, dest_node, nbytes;
            string data_rate;
            ss >> src_node >> dest_node >> nbytes >> data_rate;
            uint64_t data_rate_int;
            if (!DataRate::DoParse(data_rate, &data_rate_int)){
                cout << "Could not data_rate " << data_rate << endl;
                assert(false);
            }
            Flow flow(src_node, dest_node, nbytes, data_rate);
            PII penultimate_hops = PenultimateHops(src_node, dest_node);
            flow.second_hop = penultimate_hops.first;
            flow.second_last_hop = penultimate_hops.second;
            flows.push_back(flow);
        }
        myfile.close();
    }
}

void Topology::ReadTopologyFromFile(string topology_filename){
    //< read graph from the graphFile
    ifstream myfile(topology_filename.c_str());
    string line;
    vector<pair<int, int> > network_edges, host_edges;
    if (myfile.is_open()){
        while(myfile.good()){
            getline(myfile, line);
            if (line.find("->") == string::npos && line.find(" ") == string::npos) break;
            if(line.find(" ") != string::npos){ //switch --> switch link
                assert(line.find("->") == string::npos);
                int sw1 = atoi(line.substr(0, line.find(" ")).c_str());
                int sw2 = atoi(line.substr(line.find(" ") + 1).c_str());
                num_tor = max(max(num_tor, sw1+1), sw2+1); 
                network_edges.push_back(pair<int, int>(sw1, sw2));
                nodes.insert(sw1);
                nodes.insert(sw2);
                //cout<<"(sw1, sw2, num_tor): "<<sw1<<", "<<sw2<<", "<<num_tor<<endl;
            }
            if(line.find("->") != string::npos){ //host --> rack link
                assert(line.find(" ") == string::npos);
                int host = atoi(line.substr(0, line.find("->")).c_str());
                int rack = atoi(line.substr(line.find("->") + 2).c_str());
                if(rack >= num_tor or !IsNodeHost(host)){
                    cout<<"Graph file has out of bounds nodes, "<<host<<"->"<<rack<<", NSW: "<<num_tor<<endl;
                    exit(0);
                }
                nodes.insert(host);
                nodes.insert(rack);
                host_edges.push_back(pair<int, int>(host, rack));
                host_to_tor[host] = rack;
                total_host++;
                //cout<<"(host, rack, num_tor): "<<host<<", "<<rack<<", "<<num_tor<<endl;
            }
        }
        myfile.close();
    }
    cout<<"num_tor: "<<num_tor<<endl;
    cout<<"total_host: "<<total_host<<endl;
    cout<<"num_network_edges: "<<network_edges.size()<<endl;
    cout<<"num_host_edges: "<<host_edges.size()<<endl;
    network_links.resize(num_tor);
    hosts_in_tor.resize(num_tor);
    for(int i=0; i<network_edges.size(); i++){
        pair<int, int> link = network_edges[i];
        network_links[link.first].push_back(link.second);
        network_links[link.second].push_back(link.first);
    }
    for(int i=0; i<host_edges.size(); i++){
        hosts_in_tor[host_edges[i].second].push_back(host_edges[i].first);
    }
    //sanity check
    for(int i=0; i<num_tor; i++){ 
        sort(hosts_in_tor[i].begin(), hosts_in_tor[i].end());
        for(int t=1; t<hosts_in_tor[i].size(); t++){
            if(hosts_in_tor[i][t] != hosts_in_tor[i][t-1]+1){
                cout<<"Hosts in Rack "<<i<<" are not contiguous"<<endl;
                exit(0);
            }
        }
    }
    ComputeAllPairShortestPathlens();
}

void Topology::ChooseFailedDevice(int nfails, double frac_links_failed){
    vector<int> switches;
    for (int n: nodes) if (!IsNodeHost(n)) switches.push_back(n);

    failed_switches.clear();
    for (int i=0; i<nfails; i++){
        int ind = rand()%switches.size();
        while(failed_switches.find(switches[ind])!=failed_switches.end()){
            ind = rand()%failed_switches.size();
        }
        failed_switches.insert(switches[ind]);
    }

    failed_links.clear();
    for (int fsw: failed_switches){
        std::cout<<"Failing_link "<<fsw<<" "<<fsw<<" "<<frac_links_failed<<endl;
        for (int nbr: network_links[fsw]){
            if (drand48() < frac_links_failed) failed_links.insert(pair<int, int>(fsw, nbr));
            if (drand48() < frac_links_failed) failed_links.insert(pair<int, int>(nbr, fsw));
        }
        for(int host: hosts_in_tor[fsw]){
            if (drand48() < frac_links_failed) failed_links.insert(pair<int, int>(fsw, host));
            if (drand48() < frac_links_failed) failed_links.insert(pair<int, int>(host, fsw));
        }
    }
}

void Topology::ChooseFailedLinks(int nfails){
    vector<pair<int, int> > edges_list;
    for(int s=0; s<network_links.size(); s++){
        for(int ind=0; ind<network_links[s].size(); ind++){
            edges_list.push_back(pair<int, int>(s, network_links[s][ind]));
        }
    }
    for(int t=0; t<num_tor; t++){ 
        for(int h=0; h<hosts_in_tor[t].size(); h++){
            int host = hosts_in_tor[t][h];
            edges_list.push_back(pair<int, int>(t, host));
            edges_list.push_back(pair<int, int>(host, t));
        }
    }
    failed_links.clear();
    for (int i=0; i<nfails; i++){
        int ind = rand()%edges_list.size();
        while(failed_links.find(edges_list[ind])!=failed_links.end()){
            ind = rand()%edges_list.size();
        }
        failed_links.insert(edges_list[ind]);
    }
}

pair<int, int> Topology::PenultimateHops(int src_node, int dest_node){
    assert (src_node != dest_node);
    int second_hop, second_last_hop;
    if (IsNodeHost(src_node)) second_hop = GetHostRack(src_node);
    if (IsNodeHost(dest_node)) second_last_hop = GetHostRack(dest_node);
    
    if (!IsNodeHost(src_node) or !IsNodeHost(dest_node)){
        int src_rack = (IsNodeHost(src_node)? GetHostRack(src_node) : src_node);
        int dest_rack = (IsNodeHost(dest_node)? GetHostRack(dest_node) : dest_node);
        vector<vector<int> > paths;
        GetPathsRack(src_rack, dest_rack, paths);
        // otherwise second hop is not well defined
        if (!(paths.size() == 1 and paths[0].size() >= 2))
          cout << "Penultimate hops " <<  src_node << " " << dest_node <<  " " << paths.size() << " " << paths[0].size() << endl;
        assert(paths.size() == 1 and paths[0].size() >= 2);
        if (!IsNodeHost(src_node)) second_hop = paths[0][1];
        if (!IsNodeHost(dest_node)) second_last_hop = paths[0][paths[0].size()-2];
    }
    return PII(second_hop, second_last_hop);
}

void Topology::GetPathsRack(int src_rack, int dest_rack, vector<vector<int> > &result){
    queue<vector<int> > shortest_paths_till_now;
    vector<int> path_till_now;
    path_till_now.push_back(src_rack);
    shortest_paths_till_now.push(path_till_now);
    while(!shortest_paths_till_now.empty()){
        path_till_now = shortest_paths_till_now.front();
        shortest_paths_till_now.pop();
        int last_vertex = path_till_now.back();
        if (last_vertex == dest_rack){
            //found a shortest path
            result.push_back(path_till_now);
        }
        else{
            //extend path along neighbours which are on shortest paths
            for(int nbr: network_links[last_vertex]){
                if(shortest_pathlens[last_vertex][dest_rack] == 1 + shortest_pathlens[nbr][dest_rack]){
                    vector<int> extended_path(path_till_now);
                    extended_path.push_back(nbr);
                    shortest_paths_till_now.push(extended_path);
                }
            }
        }
    }
}

char* Topology::GetLinkIpAddressFirst(int sw, int h){
    return IpOctectsToString(10, (GetFirstOctetOfTor(sw) | 128), GetSecondOctetOfTor(sw), 2 + GetNumHostsInRack(sw) + 2*h); 
}

char* Topology::GetLinkIpAddressSecond(int sw, int h){
    return IpOctectsToString(10, (GetFirstOctetOfTor(sw) | 128), GetSecondOctetOfTor(sw), 2 + GetNumHostsInRack(sw) + 2*h + 1); 
}

pair<char*, char*> Topology::GetLinkBaseIpAddress(int sw, int h){
    char* subnet = IpOctectsToString(10, (GetFirstOctetOfTor(sw) | 128), GetSecondOctetOfTor(sw), 0);
    char* base = IpOctectsToString(0, 0, 0, 2 + GetNumHostsInRack(sw) + 2*h); 
    return pair<char*, char*>(subnet, base);
}

char* Topology::GetHostIpAddress(int host){
    int rack = GetHostRack(host);
    int ind = GetHostIndexInRack(host);
    return IpOctectsToString(10, GetFirstOctetOfTor(rack), GetSecondOctetOfTor(rack), ind*2 + 2);
}

char* Topology::GetHostRackIpAddressRackIface(int host){
    int rack = GetHostRack(host);
    int ind = GetHostIndexInRack(host);
    return IpOctectsToString(10, GetFirstOctetOfTor(rack), GetSecondOctetOfTor(rack), ind*2 + 1);
}

pair<char*, char*> Topology::GetHostBaseIpAddress(int sw, int h){
    char* subnet = IpOctectsToString(10, GetFirstOctetOfTor(sw), GetSecondOctetOfTor(sw), 0);
    char* base = IpOctectsToString(0, 0, 0, (h*2) + 1); 
    return pair<char*, char*>(subnet, base);
}

pair<int, int> Topology::GetRackHostsLimit(int rack){ //rack contains hosts from [l,r) ===> return (l, r)
    pair<int, int> ret(0, 0);
    if(hosts_in_tor[rack].size() > 0){
        ret.first = hosts_in_tor[rack][0];
        ret.second = hosts_in_tor[rack].back()+1;
    }
    return ret;
}

char* Topology::GetFlowSrcIpAddress(Flow &flow){
    if (IsNodeHost(flow.src_node)) return GetHostIpAddress(flow.src_node);
    // Switch node, need to chose the right interface: 2 cases

    // Case1: next hop on the path is a host
    if (IsNodeHost(flow.second_hop)) return GetHostRackIpAddressRackIface(flow.second_hop);

    // Case2: next hop on the path is another switch
    if (flow.src_node < flow.second_hop){
        for (int h=0;h<network_links[flow.src_node].size();h++){
            if (network_links[flow.src_node][h] == flow.second_hop){
                return GetLinkIpAddressFirst(flow.src_node, h);
            }
        }
        assert(false);
    }
    else{
        for (int h=0;h<network_links[flow.second_hop].size();h++){
            if (network_links[flow.second_hop][h] == flow.src_node){
                return GetLinkIpAddressSecond(flow.second_hop, h);
            }
        }
        assert(false);
    }
}

char* Topology::GetFlowDestIpAddress(Flow &flow){
    if (IsNodeHost(flow.dest_node)) return GetHostIpAddress(flow.dest_node);
    // Switch node, need to chose the right interface: 2 cases

    // Case1: previous hop on the path is a host
    if (IsNodeHost(flow.second_last_hop)) return GetHostRackIpAddressRackIface(flow.second_last_hop);

    // Case2: previous hop on the path is another switch
    if (flow.dest_node < flow.second_last_hop){
        for (int h=0;h<network_links[flow.dest_node].size();h++){
            if (network_links[flow.dest_node][h] == flow.second_last_hop){
                return GetLinkIpAddressFirst(flow.dest_node, h);
            }
        }
        assert(false);
    }
    else{
        for (int h=0;h<network_links[flow.second_last_hop].size();h++){
            if (network_links[flow.second_last_hop][h] == flow.dest_node){
                return GetLinkIpAddressSecond(flow.second_last_hop, h);
            }
        }
        assert(false);
    }
}

char* Topology::GetSwitchIpAddress(int sw){
    assert (network_links[sw].size() > 0);
    // return ip address of any interface
    int nbr = network_links[sw][0];
    if (sw < nbr){
        // link ip determined by sw
        return GetLinkIpAddressFirst(sw, 0);
    }
    else{
        // link ip determined by nbr
        for (int ind=0; ind<network_links[nbr].size(); ind++){
            if (network_links[nbr][ind] == sw){
                return GetLinkIpAddressSecond(nbr, ind);
            }
        }
        assert(false);
    }
}

bool Topology::IsNodeHost(int node){
    return (node >= HOST_OFFSET);
}

bool Topology::IsNodeRack(int node){
    return (node < HOST_OFFSET) and (hosts_in_tor.size()<node and hosts_in_tor[node].size()>0);
}

int Topology::GetHostRack(int host){
    assert (IsNodeHost(host));
    return host_to_tor[host];
}

int Topology::GetNumHostsInRack(int rack){
    return hosts_in_tor[rack].size();
}

int Topology::GetHostIndexInRack(int host){
    return host - GetRackHostsLimit(GetHostRack(host)).first;
}

int Topology::GetFirstOctetOfTor(int tor){
    return tor/256;
}

int Topology::GetSecondOctetOfTor(int tor){
    return tor%256;
}

double Topology::GetDropRateLinkUniform(double min_drop_rate, double max_drop_rate){
    return min_drop_rate + drand48() * (max_drop_rate - min_drop_rate);
}

double Topology::GetDropRateFailedLink(){
    //if (drand48() <= 0.5){
        double min_drop_rate_failed_link = 0.001;
        double max_drop_rate_failed_link = 0.01;
       return GetDropRateLinkUniform(min_drop_rate_failed_link, max_drop_rate_failed_link);
    //}
    //else{
    //    double min_drop_rate_failed_link = 0.05;
    //    double max_drop_rate_failed_link = 0.1;
    //    return GetDropRateLinkUniform(min_drop_rate_failed_link, max_drop_rate_failed_link);
    //}
}


double Topology::GetFailParam(pair<int, int> link, double fail_param){
    //random drop rate b/w 5*10^-5 and 10^-4
    double min_drop_rate_correct_link = 0.0000;
    double max_drop_rate_correct_link = 0.0001;
    double silent_drop_rate;
    if(failed_links.find(link) != failed_links.end()){
        //fail_param is appropriately set, so use that
        if (fail_param > max_drop_rate_correct_link) silent_drop_rate = fail_param;
        else silent_drop_rate = GetDropRateFailedLink();
        if (failed_switches.find(link.first) == failed_switches.end() and
            failed_switches.find(link.second) == failed_switches.end()){
            std::cout<<"Failing_link "<<link.first<<" "<<link.second<<" "<<silent_drop_rate<<endl;
        }
        else
            std::cout<<"Failing_device_link "<<link.first<<" "<<link.second<<" "<<silent_drop_rate<<endl;
    }
    else{
        silent_drop_rate = GetDropRateLinkUniform(min_drop_rate_correct_link, max_drop_rate_correct_link);
    }
    return silent_drop_rate;
}

void Topology::ConnectSwitchesAndSwitches(PointToPointHelper &p2p, NodeContainer &tors, double fail_param){
    vector<NetDeviceContainer> ss[num_tor];     
    vector<Ipv4InterfaceContainer> ip_container_ss[num_tor];
    for(int i=0; i<num_tor; i++){
        ss[i].resize(network_links[i].size());
        ip_container_ss[i].resize(network_links[i].size());
    }
    for (int i=0;i<num_tor;i++){
        for (int h=0;h<network_links[i].size();h++){
            int nbr = network_links[i][h];
            if (nbr < i) continue;
            double silent_drop_rate1 = GetFailParam(pair<int, int>(i, nbr), fail_param);
            double silent_drop_rate2 = GetFailParam(pair<int, int>(nbr, i), fail_param);
            ss[i][h] = p2p.Install(tors.Get(i), tors.Get(nbr));
            Ptr<NetDevice> device;
            Ptr<PointToPointNetDevice> p2p_device;
            //i --> nbr
            if (silent_drop_rate1 > 0.0) {
                Ptr<RateErrorModel> rem1 = CreateObjectWithAttributes<RateErrorModel> (
                                    "ErrorRate", DoubleValue (silent_drop_rate1),
                                    "ErrorUnit", EnumValue (RateErrorModel::ERROR_UNIT_PACKET));
                device =  ss[i][h].Get(1);
                p2p_device = device->GetObject<PointToPointNetDevice> ();;
                p2p_device->SetAttribute ("ReceiveErrorModel", PointerValue (rem1));
            }
            //nbr --> i
            if (silent_drop_rate2 > 0.0) {
                Ptr<RateErrorModel> rem2 = CreateObjectWithAttributes<RateErrorModel> (
                                    "ErrorRate", DoubleValue (silent_drop_rate2),
                                    "ErrorUnit", EnumValue (RateErrorModel::ERROR_UNIT_PACKET));
                device =  ss[i][h].Get(0);
                p2p_device = device->GetObject<PointToPointNetDevice> ();;
                p2p_device->SetAttribute ("ReceiveErrorModel", PointerValue (rem2));
            }

            //Assign subnet
            pair<char* , char*> subnet_base = GetLinkBaseIpAddress(i, h);
            char *subnet = subnet_base.first;
            char *base =subnet_base.second;
            //cout<<"Network Link; "<<i<<" <--> "<<nbr<< " Nodecontainer.size: "<<ss[i][h].GetN() << endl;
            //cout<<subnet<<" , "<<base<<endl;
            address.SetBase (subnet, "255.255.255.0",base);
            ip_container_ss[i][h] = address.Assign(ss[i][h]);
        }           
    }
}

void Topology::ConnectSwitchesAndHosts(PointToPointHelper &p2p, NodeContainer &tors, NodeContainer *rack_hosts, double fail_param){
    vector<NetDeviceContainer> sh[num_tor];     
    vector<Ipv4InterfaceContainer> ip_container_sh[num_tor];
    for(int i=0; i<num_tor; i++){
        sh[i].resize(GetNumHostsInRack(i));
        ip_container_sh[i].resize(GetNumHostsInRack(i));
    }
    for (int i=0;i<num_tor;i++){
        for (int h=0; h<hosts_in_tor[i].size(); h++){
            int host = hosts_in_tor[i][h];
            assert (GetHostIndexInRack(host) == h);
            double silent_drop_rate1 = GetFailParam(pair<int, int>(i, host), fail_param);
            double silent_drop_rate2 = GetFailParam(pair<int, int>(host, i), fail_param);

            Ptr<NetDevice> device;
            Ptr<PointToPointNetDevice> p2p_device;
            sh[i][h] = p2p.Install(tors.Get(i), rack_hosts[i].Get(h));
            //i --> host
            if (silent_drop_rate1 > 0.0) {
                Ptr<RateErrorModel> rem1 = CreateObjectWithAttributes<RateErrorModel> (
                                    "ErrorRate", DoubleValue (silent_drop_rate1),
                                    "ErrorUnit", EnumValue (RateErrorModel::ERROR_UNIT_PACKET));
                device =  sh[i][h].Get(1);
                p2p_device = device->GetObject<PointToPointNetDevice> ();;
                p2p_device->SetAttribute ("ReceiveErrorModel", PointerValue (rem1));
            }
            //host --> i
            if (silent_drop_rate2 > 0.0) {
                Ptr<RateErrorModel> rem2 = CreateObjectWithAttributes<RateErrorModel> (
                                    "ErrorRate", DoubleValue (silent_drop_rate2),
                                    "ErrorUnit", EnumValue (RateErrorModel::ERROR_UNIT_PACKET));
                device =  sh[i][h].Get(0);
                p2p_device = device->GetObject<PointToPointNetDevice> ();;
                p2p_device->SetAttribute ("ReceiveErrorModel", PointerValue (rem2));
            }

            //Assign subnet
            pair<char* , char*> subnet_base = GetHostBaseIpAddress(i, h);
            char *subnet = subnet_base.first;
            char *base =subnet_base.second;
            address.SetBase (subnet, "255.255.255.0", base);
            ip_container_sh[i][h] = address.Assign(sh[i][h]);
        }
    }
}

// Between all racks
void Topology::ComputeAllPairShortestPathlens(){
    shortest_pathlens.resize(num_tor);
    for(int i = 0; i < num_tor; i++){
        shortest_pathlens[i] = vector<int>(num_tor, INT_MAX/2);
        shortest_pathlens[i][i] = 0;
        for (int nbr: network_links[i]){
            shortest_pathlens[i][nbr] = 1;
        }
    }

    //Floyd Warshall
    for(int k = 0; k < num_tor; k++){
        for(int i = 0; i < num_tor; i++){
            for(int j = 0; j < num_tor; j++){
                if(shortest_pathlens[i][j] > shortest_pathlens[i][k] + shortest_pathlens[k][j])
                    shortest_pathlens[i][j] = shortest_pathlens[i][k] + shortest_pathlens[k][j];
            }
        }
    }
    cout<<"Finished floyd warshall"<<endl;
}

void Topology::PrintAllPairShortestPaths(){
    shortest_pathlens.resize(num_tor);
    for(int i = 0; i<num_tor; i++){
        for (int j=0; j<num_tor; j++){
            if (hosts_in_tor[i].size() > 0 and hosts_in_tor[j].size() > 0 and i!=j){
                vector<vector<int> > paths;
                GetPathsRack(i, j, paths);
                for (vector<int>& p: paths){
                   cout<<"flowpath ";
                   for (int node: p){
                      cout<<node<<" ";
                   }
                   cout<<endl;
                }
            }
        }
    }
}

void Topology::SnapshotFlow(Flow flow, ApplicationContainer& flow_app, Time start_time, Time snapshot_time){
    assert (flow_app.GetN() == 1);
    Ptr<Application> app = flow_app.Get(0);
    Ptr<TcpSocketBase> tcp_socket_base = GetSocketFromOnOffApp(app);
    cout<<"Flowid "<<flow.src_node<<" "<<flow.dest_node<<" "<<GetFlowSrcIpAddress(flow)
        <<" "<<GetFlowDestIpAddress(flow)<<" " << flow.second_hop
        << " " << flow.second_last_hop <<" "<<tcp_socket_base->GetLocalPort()
        <<" "<<tcp_socket_base->GetPeerPort()<<" "<<flow.nbytes<<" "
        <<start_time<<" "<<snapshot_time
        <<" "<<tcp_socket_base->GetSentPackets()<<" "
        <<tcp_socket_base->GetLostPackets()<<" "
        <<tcp_socket_base->GetRandomlyLostPackets()<<" "
        <<tcp_socket_base->GetAckedPackets()<<endl;
}

void Topology::PrintIpAddresses(){
    for (int i=0;i<num_tor;i++){
        for (int h=0;h<network_links[i].size();h++){
            int nbr = network_links[i][h];
            if (nbr < i) continue;
            pair<char* , char*> subnet_base = GetLinkBaseIpAddress(i, h);
            char *subnet = subnet_base.first;
            char *base =subnet_base.second;
            address.SetBase (subnet, "255.255.255.0",base);
            Ipv4Address address1 = address.NewAddress(); //avoid postfix, infix issues by avoiding inline call
            Ipv4Address address2 = address.NewAddress();
            cout<<"link_ip "<<address1<<" "<<address2<<" "<<i<<" "<<nbr<<endl;
        }       
    }
    for (map<int, int>::iterator it = host_to_tor.begin(); it != host_to_tor.end(); it++){
        int host = it->first;
        int rack = it->second;
        int h = GetHostIndexInRack(host);
        pair<char* , char*> subnet_base = GetHostBaseIpAddress(rack, h);
        char *subnet = subnet_base.first;
        char *base =subnet_base.second;
        address.SetBase (subnet, "255.255.255.0",base);
        Ipv4Address address1 = address.NewAddress();
        Ipv4Address address2 = address.NewAddress();
        cout<<"host_ip "<<address1<<" "<<address2<<" "<<host<<" "<<rack<<endl;
    }
}

Ptr<TcpSocketBase> Topology::GetSocketFromOnOffApp(Ptr<Application> app){
    Ptr<OnOffApplication> on_off_app = app->GetObject<OnOffApplication> ();
    Ptr<Socket> socket = on_off_app->GetSocket();
    Ptr<TcpSocketBase> tcp_socket_base = socket->GetObject<TcpSocketBase>();
    return tcp_socket_base;
}

int Topology::GetFirstUnusedPort(int src_node, int dest_node){
    PII endpoints(src_node, dest_node);
    if (endpoint_first_unused_port.find(endpoints) == endpoint_first_unused_port.end()){
        endpoint_first_unused_port[endpoints] = start_port;
    }
    int port = endpoint_first_unused_port[endpoints];
    node_first_unused_port[src_node] = max(node_first_unused_port[src_node], port+1);
    node_first_unused_port[dest_node] = max(node_first_unused_port[dest_node], port+1);
    endpoint_first_unused_port[endpoints] = port+1;
    //cout << src_node << " " << dest_node << " " << port << endl;
    return port;
}

void Topology::SetupEcmpFailure(NodeContainer &tors){
    vector<int> switches;
    failed_switches.clear();
    for (int n: nodes) if (!IsNodeHost(n) and !IsNodeRack(n)) switches.push_back(n);
    failed_switches.insert(switches[rand()%switches.size()]);
    for (int fsw: failed_switches){
        cout << "Failing ecmp on " << fsw << endl;
        Ptr<Node> node = tors.Get(fsw);
        Ptr<Ipv4> ipv4 = node->GetObject<Ipv4> ();
        Ptr<Ipv4GlobalRouting> routing_proto = StaticCast<Ipv4GlobalRouting, Ipv4RoutingProtocol>(ipv4->GetRoutingProtocol());
        routing_proto->SetEcmpFailure();
    }
}

void Topology::AdaptNetwork(){
    //!TODO
}
