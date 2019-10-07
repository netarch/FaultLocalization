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
#include "ns3/bulk-send-application.h"
#include "ns3/onoff-application.h"
#include "topology.h"

using namespace ns3;
using namespace std;


// Function to create address string from numbers
//
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

void Topology::ReadServerTmFromFile(string tm_filename){
    //< read TM from the TM File
    ifstream myfile(tm_filename.c_str());
    string line;
    if (myfile.is_open()){
        string delimiter = ",";
        while(myfile.good()){
            getline(myfile, line);
            std::replace(line.begin(), line.end(), ',', ' ');
            vector<double> array;
            stringstream ss(line);
            double temp;
            while (ss >> temp){
                array.push_back(temp); 
                //cout<<temp<<",";
            }
            server_TM.push_back(array);
            //cout<<endl;
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
                //cout<<"(sw1, sw2, num_tor): "<<sw1<<", "<<sw2<<", "<<num_tor<<endl;
            }
            if(line.find("->") != string::npos){ //host --> rack link
                assert(line.find(" ") == string::npos);
                int host = atoi(line.substr(0, line.find("->")).c_str());
                int rack = atoi(line.substr(line.find("->") + 2).c_str());
                if(rack >= num_tor){
                    cout<<"Graph file has out of bounds nodes, "<<host<<"->"<<rack<<", NSW: "<<num_tor<<endl;
                    exit(0);
                }
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
        pair<int, int> hostlink = host_edges[i];
        hosts_in_tor[hostlink.second].push_back(hostlink.first);
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
            int offset_host = OffsetHost(host);
            edges_list.push_back(pair<int, int>(t, offset_host));
            edges_list.push_back(pair<int, int>(offset_host, t));
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

void Topology::GetPathsHost(int src_host, int dest_host, vector<vector<int> >&result){
    int src_rack = GetHostRack(src_host);
    int dest_rack = GetHostRack(dest_host);
    GetPathsRack(src_rack, dest_rack, result);
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

int Topology::OffsetHost(int host){
    int offset= 10000;
    return host + offset;
}

int Topology::GetHostRack(int host){
    return host_to_tor[host];
}

int Topology::GetHostNumber(int rack, int ind){
    assert (ind < hosts_in_tor[rack].size());
    return hosts_in_tor[rack][ind];
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
        double max_drop_rate_failed_link = 0.010;
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
        std::cout<<"Failing_link "<<link.first<<" "<<link.second<<" "<<silent_drop_rate<<endl;
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
        for (int h=0; h<GetNumHostsInRack(i); h++){         

            int offset_host = OffsetHost(GetHostNumber(i, h));
            double silent_drop_rate1 = GetFailParam(pair<int, int>(i, offset_host), fail_param);
            double silent_drop_rate2 = GetFailParam(pair<int, int>(offset_host, i), fail_param);

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

void Topology::PrintFlowPath(int src_host, int dest_host){
    vector<vector<int> > paths;
    GetPathsHost(src_host, dest_host, paths);
    for (vector<int>& p: paths){
       cout<<"flowpath ";
       for (int node: p){
          cout<<node<<" ";
       }
       cout<<endl;
    }
    vector<vector<int> > reverse_paths;
    GetPathsHost(dest_host, src_host, reverse_paths);
    for (vector<int>& p: reverse_paths){
       cout<<"flowpath_reverse ";
       for (int node: p){
          cout<<node<<" ";
       }
       cout<<endl;
    }
}

void Topology::SnapshotFlow(int src_host, int dest_host, int bytes, ApplicationContainer& flow_app, Time start_time, Time snapshot_time){
    assert (flow_app.GetN() == 1);
    Ptr<Application> app = flow_app.Get(0);
    Ptr<TcpSocketBase> tcp_socket_base = GetSocketFromOnOffApp(app);
    cout<<"Flowid "<<OffsetHost(src_host)<<" "<<OffsetHost(dest_host)<<" "<<GetHostIpAddress(src_host)
        <<" "<<GetHostIpAddress(dest_host)<<" " <<host_to_tor[src_host]
        << " " << host_to_tor[dest_host] <<" "<<tcp_socket_base->GetLocalPort()
        <<" "<<tcp_socket_base->GetPeerPort()<<" "<<bytes<<" "
        <<start_time<<" "<<snapshot_time
        <<" "<<tcp_socket_base->GetSentPackets()<<" "
        <<tcp_socket_base->GetLostPackets()<<" "
        <<tcp_socket_base->GetRandomlyLostPackets()<<" "
        <<tcp_socket_base->GetAckedPackets()<<endl;
}

void Topology::PrintFlowInfo(int src_host, int dest_host, int bytes, ApplicationContainer& flow_app){
    assert (flow_app.GetN() == 1);
    Ptr<Application> app = flow_app.Get(0);
    Ptr<TcpSocketBase> tcp_socket_base = GetSocketFromBulkSendApp(app);
    cout<<"Flowid "<<OffsetHost(src_host)<<" "<<OffsetHost(dest_host)<<" "
        <<GetHostIpAddress(src_host)<<" "<<GetHostIpAddress(dest_host)<<" "
        <<host_to_tor[src_host] << " " << host_to_tor[dest_host] <<" "
        <<tcp_socket_base->GetLocalPort()<<" "<<tcp_socket_base->GetPeerPort()<<" "
        <<bytes<<" "<<app->GetStartTime()<<" "<<tcp_socket_base->GetFinishTime()
        <<" "<<tcp_socket_base->GetSentPackets()<<" "
        <<tcp_socket_base->GetLostPackets()<<" "
        <<tcp_socket_base->GetRandomlyLostPackets()<<" "
        <<tcp_socket_base->GetAckedPackets()<<endl;
    PrintFlowPath(src_host, dest_host);
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
        cout<<"host_ip "<<address1<<" "<<address2<<" "<<OffsetHost(host)<<" "<<rack<<endl;
        //char* address = GetHostIpAddress(host);
        //cout<<"host_ip "<<address<<" "<<host<<endl;
    }
}

Ptr<TcpSocketBase> Topology::GetSocketFromBulkSendApp(Ptr<Application> app){
    Ptr<BulkSendApplication> bulk_send_app = app->GetObject<BulkSendApplication> ();
    Ptr<Socket> socket = bulk_send_app->GetSocket();
    Ptr<TcpSocketBase> tcp_socket_base = socket->GetObject<TcpSocketBase>();
    return tcp_socket_base;
}

Ptr<TcpSocketBase> Topology::GetSocketFromOnOffApp(Ptr<Application> app){
    Ptr<OnOffApplication> on_off_app = app->GetObject<OnOffApplication> ();
    Ptr<Socket> socket = on_off_app->GetSocket();
    Ptr<TcpSocketBase> tcp_socket_base = socket->GetObject<TcpSocketBase>();
    return tcp_socket_base;
}

void Topology::AdaptNetwork(){
    //!TODO
}

void AdaptNetwork(Topology& topology){
    topology.AdaptNetwork();
}
