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
char * ipOctectsToString(int first, int second, int third, int fourth){
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

void Topology::readServerTmFromFile(string tmfile){
    //< read TM from the TM File
    ifstream myfile(tmfile.c_str());
    string line;
    if (myfile.is_open()){
        string delimiter = ",";
        while(myfile.good()){
            getline(myfile, line);
            std::replace(line.begin(), line.end(), ',', ' ');  // replace ',' by ' '
            vector<double> array;
            stringstream ss(line);
            double temp;
            while (ss >> temp){
                array.push_back(temp); 
                //cout<<temp<<",";
            }
            serverTM.push_back(array);
            //cout<<endl;
        }
        myfile.close();
    }
}

void Topology::readTopologyFromFile(string topofile){
    //< read graph from the graphFile
    ifstream myfile(topofile.c_str());
    string line;
    vector<pair<int, int> > networkEdges, hostEdges;
    if (myfile.is_open()){
        while(myfile.good()){
            getline(myfile, line);
            if (line.find("->") == string::npos && line.find(" ") == string::npos) break;
            if(line.find(" ") != string::npos){ //num_tor --> rack link
                assert(line.find("->") == string::npos);
                int sw1 = atoi(line.substr(0, line.find(" ")).c_str());
                int sw2 = atoi(line.substr(line.find(" ") + 1).c_str());
                num_tor = max(max(num_tor, sw1+1), sw2+1); 
                networkEdges.push_back(pair<int, int>(sw1, sw2));
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
                hostEdges.push_back(pair<int, int>(host, rack));
                hostToTor[host] = rack;
                total_host++;
                //cout<<"(host, rack, num_tor): "<<host<<", "<<rack<<", "<<num_tor<<endl;
            }
        }
        myfile.close();
    }
    cout<<"num_tor: "<<num_tor<<endl;
    cout<<"total_host: "<<total_host<<endl;
    cout<<"num_network_edges: "<<networkEdges.size()<<endl;
    cout<<"num_host_edges: "<<hostEdges.size()<<endl;
    networkLinks.resize(num_tor);
    hostsInTor.resize(num_tor);
    for(int i=0; i<networkEdges.size(); i++){
        pair<int, int> link = networkEdges[i];
        networkLinks[link.first].push_back(link.second);
        networkLinks[link.second].push_back(link.first);
    }
    for(int i=0; i<hostEdges.size(); i++){
        pair<int, int> hostlink = hostEdges[i];
        hostsInTor[hostlink.second].push_back(hostlink.first);
    }
    //sanity check
    for(int i=0; i<num_tor; i++){ 
        sort(hostsInTor[i].begin(), hostsInTor[i].end());
        for(int t=1; t<hostsInTor[i].size(); t++){
            if(hostsInTor[i][t] != hostsInTor[i][t-1]+1){
                cout<<"Hosts in Rack "<<i<<" are not contiguous"<<endl;
                exit(0);
            }
        }
    }
    compute_all_pair_shortest_pathlens();
}

void Topology::chooseFailedLinks(int nfails){
    vector<pair<int, int> > edges_list;
    for(int s=0; s<networkLinks.size(); s++){
        for(int ind=0; ind<networkLinks[s].size(); ind++){
            edges_list.push_back(pair<int, int>(s, networkLinks[s][ind]));
        }
    }
    for(int t=0; t<num_tor; t++){ 
        for(int h=0; h<hostsInTor[t].size(); h++){
            int host = hostsInTor[t][h];
            int offset_host = OffsetHost(host);
            edges_list.push_back(pair<int, int>(t, offset_host));
            edges_list.push_back(pair<int, int>(offset_host, t));
        }
    }
    failedLinks.clear();
    for (int i=0; i<nfails; i++){
        int ind = rand()%edges_list.size();
        while(failedLinks.find(edges_list[ind])!=failedLinks.end()){
            ind = rand()%edges_list.size();
        }
        failedLinks.insert(edges_list[ind]);
    }
}

vector<vector<int> > Topology::getPaths(int srchost, int desthost){
    char* srchost_ip = getHostIpAddress(srchost);
    char* desthost_ip = getHostIpAddress(desthost);
    int srcrack = getHostRack(srchost);
    int destrack = getHostRack(desthost);

    vector<vector<int> > shortest_paths;
    queue<vector<int> > shortest_paths_till_now;
    vector<int> path_till_now;
    path_till_now.push_back(OffsetHost(srchost));
    path_till_now.push_back(srcrack);
    shortest_paths_till_now.push(path_till_now);
    while(!shortest_paths_till_now.empty()){
        path_till_now = shortest_paths_till_now.front();
        shortest_paths_till_now.pop();
        int last_vertex = path_till_now.back();
        if (last_vertex == destrack){
            //found a shortest path
            path_till_now.push_back(OffsetHost(desthost));
            shortest_paths.push_back(path_till_now);
        }
        else{
            //extend path along neighbours which are on shortest paths
            for(int nbr: networkLinks[last_vertex]){
                if(shortest_pathlens[last_vertex][destrack] == 1 + shortest_pathlens[nbr][destrack]){
                    vector<int> extended_path(path_till_now);
                    extended_path.push_back(nbr);
                    shortest_paths_till_now.push(extended_path);

                }
            }
        }
    }
    return shortest_paths;
}

pair<char*, char*> Topology::getLinkBaseIpAddress(int sw, int h){
    char* subnet = ipOctectsToString(10, (getFirstOctetOfTor(sw) | 128), getSecondOctetOfTor(sw), 0);
    char* base = ipOctectsToString(0, 0, 0, 2 + getNumHostsInRack(sw) + 2*h); 
    return pair<char*, char*>(subnet, base);
}

char* Topology::getHostIpAddress(int host){
    int rack = getHostRack(host);
    int ind = getHostIndexInRack(host);
    return ipOctectsToString(10, getFirstOctetOfTor(rack), getSecondOctetOfTor(rack), ind*2 + 2);
}

pair<char*, char*> Topology::getHostBaseIpAddress(int sw, int h){
    char* subnet = ipOctectsToString(10, getFirstOctetOfTor(sw), getSecondOctetOfTor(sw), 0);
    char* base = ipOctectsToString(0, 0, 0, (h*2) + 1); 
    return pair<char*, char*>(subnet, base);
}

pair<int, int> Topology::getRackHostsLimit(int rack){ //rack contains hosts from [l,r) ===> return (l, r)
    pair<int, int> ret(0, 0);
    if(hostsInTor[rack].size() > 0){
        ret.first = hostsInTor[rack][0];
        ret.second = hostsInTor[rack].back()+1;
    }
    return ret;
}

int Topology::OffsetHost(int host){
    int offset= 10000;
    return host + offset;
}

int Topology::getHostRack(int host){
    return hostToTor[host];
}

int Topology::GetHostNumber(int rack, int ind){
    assert (ind < hostsInTor[rack].size());
    return hostsInTor[rack][ind];
}

int Topology::getNumHostsInRack(int rack){
    return hostsInTor[rack].size();
}

int Topology::getHostIndexInRack(int host){
    return host - getRackHostsLimit(getHostRack(host)).first;
}

int Topology::getFirstOctetOfTor(int tor){
    return tor/256;
}

int Topology::getSecondOctetOfTor(int tor){
    return tor%256;
}

double Topology::get_drop_rate_link_uniform(double min_drop_rate, double max_drop_rate){
    return min_drop_rate + drand48() * (max_drop_rate - min_drop_rate);
}

double Topology::get_drop_rate_failed_link(){
    //if (drand48() <= 0.5){
        double min_drop_rate_failed_link = 0.001;
        double max_drop_rate_failed_link = 0.005;
        return get_drop_rate_link_uniform(min_drop_rate_failed_link, max_drop_rate_failed_link);
    //}
    //else{
    //    double min_drop_rate_failed_link = 0.05;
    //    double max_drop_rate_failed_link = 0.1;
    //    return get_drop_rate_link_uniform(min_drop_rate_failed_link, max_drop_rate_failed_link);
    //}
}


double Topology::GetFailParam(pair<int, int> link, double failparam){
    //random drop rate b/w 5*10^-5 and 10^-4
    double min_drop_rate_correct_link = 0.00005;
    double max_drop_rate_correct_link = 0.0001;
    double silent_drop_rate = get_drop_rate_link_uniform(min_drop_rate_correct_link, max_drop_rate_correct_link);
    if(failedLinks.find(link) != failedLinks.end()){
        //failparam is appropriately set, so use that
        if (failparam > max_drop_rate_correct_link) silent_drop_rate = failparam;
        else silent_drop_rate = get_drop_rate_failed_link();
        std::cout<<"Failing_link "<<link.first<<" "<<link.second<<" "<<silent_drop_rate<<endl;
    }
    return silent_drop_rate;
}

void Topology::connect_switches_and_switches(PointToPointHelper &p2p, Ptr<RateErrorModel> rem, NodeContainer &tors, double failparam){
    vector<NetDeviceContainer> ss[num_tor];     
    vector<Ipv4InterfaceContainer> ipSsContainer[num_tor];
    for(int i=0; i<num_tor; i++){
        ss[i].resize(networkLinks[i].size());
        ipSsContainer[i].resize(networkLinks[i].size());
    }
    for (int i=0;i<num_tor;i++){
        for (int h=0;h<networkLinks[i].size();h++){
            int nbr = networkLinks[i][h];
            if (nbr < i) continue;
            double silent_drop_rate1 = GetFailParam(pair<int, int>(i, nbr), failparam);
            double silent_drop_rate2 = GetFailParam(pair<int, int>(nbr, i), failparam);
            ss[i][h] = p2p.Install(tors.Get(i), tors.Get(nbr));
            Ptr<NetDevice> device;
            Ptr<PointToPointNetDevice> p2pDevice;
            //i --> nbr
            if (silent_drop_rate1 > 0.0) {
                Ptr<RateErrorModel> rem1 = CreateObjectWithAttributes<RateErrorModel> (
                                    "ErrorRate", DoubleValue (silent_drop_rate1),
                                    "ErrorUnit", EnumValue (RateErrorModel::ERROR_UNIT_PACKET));
                device =  ss[i][h].Get(1);
                p2pDevice = device->GetObject<PointToPointNetDevice> ();;
                p2pDevice->SetAttribute ("ReceiveErrorModel", PointerValue (rem1));
            }
            //nbr --> i
            if (silent_drop_rate2 > 0.0) {
                Ptr<RateErrorModel> rem2 = CreateObjectWithAttributes<RateErrorModel> (
                                    "ErrorRate", DoubleValue (silent_drop_rate2),
                                    "ErrorUnit", EnumValue (RateErrorModel::ERROR_UNIT_PACKET));
                device =  ss[i][h].Get(0);
                p2pDevice = device->GetObject<PointToPointNetDevice> ();;
                p2pDevice->SetAttribute ("ReceiveErrorModel", PointerValue (rem2));
            }

            //Assign subnet
            pair<char* , char*> subnet_base = getLinkBaseIpAddress(i, h);
            char *subnet = subnet_base.first;
            char *base =subnet_base.second;
            //cout<<"Network Link; "<<i<<" <--> "<<nbr<< " Nodecontainer.size: "<<ss[i][h].GetN() << endl;
            //cout<<subnet<<" , "<<base<<endl;
            address.SetBase (subnet, "255.255.255.0",base);
            ipSsContainer[i][h] = address.Assign(ss[i][h]);
        }           
    }
}

void Topology::connect_switches_and_hosts(PointToPointHelper &p2p, NodeContainer &tors, NodeContainer *rackhosts, double failparam){
    vector<NetDeviceContainer> sh[num_tor];     
    vector<Ipv4InterfaceContainer> ipShContainer[num_tor];
    for(int i=0; i<num_tor; i++){
        sh[i].resize(getNumHostsInRack(i));
        ipShContainer[i].resize(getNumHostsInRack(i));
    }
    for (int i=0;i<num_tor;i++){
        for (int h=0; h<getNumHostsInRack(i); h++){         

            int offset_host = OffsetHost(GetHostNumber(i, h));
            double silent_drop_rate1 = GetFailParam(pair<int, int>(i, offset_host), failparam);
            double silent_drop_rate2 = GetFailParam(pair<int, int>(offset_host, i), failparam);

            Ptr<NetDevice> device;
            Ptr<PointToPointNetDevice> p2pDevice;
            sh[i][h] = p2p.Install(tors.Get(i), rackhosts[i].Get(h));
            //i --> host
            if (silent_drop_rate1 > 0.0) {
                Ptr<RateErrorModel> rem1 = CreateObjectWithAttributes<RateErrorModel> (
                                    "ErrorRate", DoubleValue (silent_drop_rate1),
                                    "ErrorUnit", EnumValue (RateErrorModel::ERROR_UNIT_PACKET));
                device =  sh[i][h].Get(1);
                p2pDevice = device->GetObject<PointToPointNetDevice> ();;
                p2pDevice->SetAttribute ("ReceiveErrorModel", PointerValue (rem1));
            }
            //host --> i
            if (silent_drop_rate2 > 0.0) {
                Ptr<RateErrorModel> rem2 = CreateObjectWithAttributes<RateErrorModel> (
                                    "ErrorRate", DoubleValue (silent_drop_rate2),
                                    "ErrorUnit", EnumValue (RateErrorModel::ERROR_UNIT_PACKET));
                device =  sh[i][h].Get(0);
                p2pDevice = device->GetObject<PointToPointNetDevice> ();;
                p2pDevice->SetAttribute ("ReceiveErrorModel", PointerValue (rem2));
            }

            //Assign subnet
            pair<char* , char*> subnet_base = getHostBaseIpAddress(i, h);
            char *subnet = subnet_base.first;
            char *base =subnet_base.second;
            address.SetBase (subnet, "255.255.255.0",base);
            ipShContainer[i][h] = address.Assign(sh[i][h]);
        }
    }
}

void Topology::compute_all_pair_shortest_pathlens(){
    shortest_pathlens.resize(num_tor);
    for(int i = 0; i < num_tor; i++){
        shortest_pathlens[i] = vector<int>(num_tor, INT_MAX/2);
        shortest_pathlens[i][i] = 0;
        for (int nbr: networkLinks[i]){
            shortest_pathlens[i][nbr] = 1;
        }
    }

    //floyd warshall
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

void Topology::print_flow_path(int srchost, int desthost){
    vector<vector<int> > paths = getPaths(srchost, desthost);
    for (vector<int>& p: paths){
       cout<<"flowpath ";
       for (int node: p){
          cout<<node<<" ";
       }
       cout<<endl;
    }
    vector<vector<int> > reverse_paths = getPaths(desthost, srchost);
    for (vector<int>& p: reverse_paths){
       cout<<"flowpath_reverse ";
       for (int node: p){
          cout<<node<<" ";
       }
       cout<<endl;
    }
}

void Topology::print_flow_info(int srchost, int desthost, int bytes, ApplicationContainer& flowApp){
    assert (flowApp.GetN() == 1);
    Ptr<Application> app = flowApp.Get(0);
    Ptr<TcpSocketBase> tcp_socket_base = getSocketFromBulkSendApp(app);
    cout<<"Flowid "<<srchost<<" "<<desthost<<" "<<getHostIpAddress(srchost)
        <<" "<<getHostIpAddress(desthost)<<" "<<tcp_socket_base->GetLocalPort()
        <<" "<<tcp_socket_base->GetPeerPort()<<" "<<bytes<<" "
        <<app->GetStartTime()<<" "<<tcp_socket_base->GetFinishTime()
        <<" "<<tcp_socket_base->GetSentPackets()<<" "
        <<tcp_socket_base->GetLostPackets()<<" "
        <<tcp_socket_base->GetRandomlyLostPackets()<<" "
        <<tcp_socket_base->GetAckedPackets()/1444<<endl;
    print_flow_path(srchost, desthost);
}

void Topology::print_ip_addresses(){
    for (int i=0;i<num_tor;i++){
        for (int h=0;h<networkLinks[i].size();h++){
            int nbr = networkLinks[i][h];
            if (nbr < i) continue;
            pair<char* , char*> subnet_base = getLinkBaseIpAddress(i, h);
            char *subnet = subnet_base.first;
            char *base =subnet_base.second;
            address.SetBase (subnet, "255.255.255.0",base);
            Ipv4Address address1 = address.NewAddress(); //avoid postfix, infix issues by avoiding inline call
            Ipv4Address address2 = address.NewAddress();
            cout<<"link_ip "<<address1<<" "<<address2<<" "<<i<<" "<<nbr<<endl;
        }       
    }
    for (map<int, int>::iterator it = hostToTor.begin(); it != hostToTor.end(); it++){
        int host = it->first;
        int rack = it->second;
        int h = getHostIndexInRack(host);
        pair<char* , char*> subnet_base = getHostBaseIpAddress(rack, h);
        char *subnet = subnet_base.first;
        char *base =subnet_base.second;
        address.SetBase (subnet, "255.255.255.0",base);
        Ipv4Address address1 = address.NewAddress();
        Ipv4Address address2 = address.NewAddress();
        cout<<"host_ip "<<address1<<" "<<address2<<" "<<host<<" "<<rack<<endl;
        //char* address = getHostIpAddress(host);
        //cout<<"host_ip "<<address<<" "<<host<<endl;
    }
}

Ptr<TcpSocketBase> Topology::getSocketFromBulkSendApp(Ptr<Application> app){
    Ptr<BulkSendApplication> bulkSendApp = app->GetObject<BulkSendApplication> ();
    Ptr<Socket> socket = bulkSendApp->GetSocket();
    Ptr<TcpSocketBase> tcp_socket_base = socket->GetObject<TcpSocketBase>();
    return tcp_socket_base;
}

Ptr<TcpSocketBase> Topology::getSocketFromOnOffApp(Ptr<Application> app){
    Ptr<OnOffApplication> onOffApp = app->GetObject<OnOffApplication> ();
    Ptr<Socket> socket = onOffApp->GetSocket();
    Ptr<TcpSocketBase> tcp_socket_base = socket->GetObject<TcpSocketBase>();
    return tcp_socket_base;
}

void Topology::snapshot_flow(int srchost, int desthost, int bytes, ApplicationContainer& flowApp, Time startTime, Time snapshotTime){
    assert (flowApp.GetN() == 1);
    Ptr<Application> app = flowApp.Get(0);
    Ptr<TcpSocketBase> tcp_socket_base = getSocketFromOnOffApp(app);
    cout<<"Flowid "<<srchost<<" "<<desthost<<" "<<getHostIpAddress(srchost)
        <<" "<<getHostIpAddress(desthost)<<" "<<tcp_socket_base->GetLocalPort()
        <<" "<<tcp_socket_base->GetPeerPort()<<" "<<bytes<<" "
        <<startTime<<" "<<snapshotTime
        <<" "<<tcp_socket_base->GetSentPackets()<<" "
        <<tcp_socket_base->GetLostPackets()<<" "
        <<tcp_socket_base->GetRandomlyLostPackets()<<" "
        <<tcp_socket_base->GetAckedPackets()<<endl;
}

void Topology::adapt_network(){


}

void adapt_network(Topology& topology){
    topology.adapt_network();
}
