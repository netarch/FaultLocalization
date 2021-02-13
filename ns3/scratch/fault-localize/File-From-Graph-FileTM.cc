// Construction of Fat-tree Architecture
// Authors: Linh Vu, Daji Wong

/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2013 Nanyang Technological University 
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Authors: Linh Vu <linhvnl89@gmail.com>, Daji Wong <wong0204@e.ntu.edu.sg>
 * Modifiedy by: Vipul Harsh <vipulharsh93@gmail.com>
 */

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <map>
#include <set>
#include <cassert>
#include <stdio.h>
#include <stdlib.h>

#include "ns3/flow-monitor-module.h"
#include "ns3/bridge-helper.h"
#include "ns3/bridge-net-device.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/csma-module.h"
#include "ns3/ipv4-nix-vector-helper.h"
#include "ns3/random-variable-stream.h"
#include "ns3/ipv4-routing-table-entry.h"

#include "topology.h"
/*
    - Adapted from the paper "Towards Reproducible Performance Studies of Datacenter Network Architectures Using An Open-Source Simulation Approach"

    - The code is constructed in the following order:
        1. Creation of Node Containers from a topology file
        2. Initialize settings for On/Off Application/bulk.send application, traffic matrix read from a file
        3. Connect hosts to top of the rack switches
        4. Connect network switch using edges read from the topology file
        6. Start Simulation

    
    - Simulation Settings, along with examples:
        - Simulation running time: 100 seconds
        - Packet size: 1024 bytes
        - Data rate for packet sending: 1 Mbps
        - Data rate for device channel: 1000 Mbps
        - Delay time for device: 0.001 ms
        - Communication pairs selection: Random Selection with uniform probability
        - Traffic flow pattern: Exponential random traffic

        - Statistics Output:
                - Flowmonitor XML output file: Fat-tree.xml is located in the /statistics folder
            

*/

using namespace ns3;
using namespace std;
NS_LOG_COMPONENT_DEFINE ("DC-Architecture");

void EchoProgress(double delay_seconds){
    Simulator::Schedule (Seconds(delay_seconds), &EchoProgress, delay_seconds);
    cout<<"Finished: "<<Simulator::Now().GetSeconds()<<" seconds"<<endl;
}

void SnapshotFlows(Topology* topology, ApplicationContainer* flow_app, double snapshot_period_seconds){
    Simulator::Schedule (Seconds(snapshot_period_seconds), &SnapshotFlows, topology,
                         flow_app, snapshot_period_seconds);
    double curr_time_seconds = Simulator::Now().GetSeconds();
    for (int ff = 0; ff<topology->flows.size(); ff++){
        if(flow_app[ff].GetStartTime().GetSeconds() < curr_time_seconds){
            cout<<"Recording snapshot "<<" "<<flow_app[ff].GetStartTime().GetSeconds()
                  <<" "<<curr_time_seconds<<" "<<snapshot_period_seconds<<endl;
            topology->SnapshotFlow(topology->flows[ff], flow_app[ff], flow_app[ff].GetStartTime(), Simulator::Now());
        }
    }
}


int main(int argc, char *argv[]){
    //For fast I/O
    ios_base::sync_with_stdio(false);
    int nreqarg = 7;
    if(argc <= nreqarg){
        cout<<nreqarg-1<<" arguments required, "<<argc-1<<" given. "<< endl
            <<"Usage> <exec> <topology_file> <tm_file> <result_file> "
            <<"<drop queue limit(e.g. 100 for a limit of 100 packets)> "
            <<"<data rate for on/off (e.g. 10 for 10 Kbps)> <nfails> <fail_param> <seed>"<<endl;
        return 0;
    }
    string topology_filename = argv[1];
    string tm_filename = argv[2];
    string result_filename = argv[3];
    int drop_queue_limit = atoi(argv[4]);
    string on_off_datarate = argv[5];
    int nfails = atoi(argv[6]);
    double fail_param = atof(argv[7]);
    int seed = atoi(argv[8]);
    seed = nfails * 100003 + seed;
    RngSeedManager::SetSeed (seed);
    RngSeedManager::SetRun (seed);
    srand(seed);
    srand48(seed);

    cout<<"Running topology: "<<topology_filename<<", output result to "<<result_filename<<endl;
    cout<<"Setting drop queue limit: "<<drop_queue_limit<<endl;

    Config::SetDefault("ns3::DropTailQueue::MaxPackets", UintegerValue (drop_queue_limit));

    // Simulation parameters
    double sim_time_seconds = 20; //0.5;
    double warmup_time_seconds = 1.0;

    // Define topology
    Topology topology;
    topology.ReadTopologyFromFile(topology_filename);
    topology.ReadFlowsFromFile(tm_filename);

    // Initialize parameters for On/Off application

    int packet_size_bytes = 1500;
    string data_rate_on_off = on_off_datarate + "Kbps"; 
    string data_rate_on_off_inf = "5Gbps"; 
    //char maxBytes [] = "0" ; //"50000"; // "0"; // unlimited

    // Initialize parameters for PointToPoint protocol
    char dataRate [] = "1Gbps"; //"40Gbps";
    uint64_t delay_us = 10; //2.5; //microseconds

    // Fail some links, by setting loss rates
    double silent_drop_rate = fail_param;
    topology.ChooseFailedLinks(nfails);

    cout << "Total number of hosts =  "<< topology.total_host<<"\n";
    cout << "On/Off flow data rate = "<< data_rate_on_off<<" Infinite data rate = "
         << data_rate_on_off_inf<<endl;

    // Initialize Internet Stack and Routing Protocols
    InternetStackHelper internet;
    Ipv4GlobalRoutingHelper global_routing_helper;
    Ipv4ListRoutingHelper list;
    list.Add (global_routing_helper, 20);   
    internet.SetRoutingHelper(list);

    NodeContainer tors;
    tors.Create(topology.num_tor);
    internet.Install(tors);

    NodeContainer rack_hosts[topology.num_tor];
    for (int i=0; i<topology.num_tor;i++){      
        rack_hosts[i].Create (topology.GetNumHostsInRack(i));
        internet.Install (rack_hosts[i]);       
    }
    //Function that returns node pointer within a container
    auto GetNodePointer = [&topology, &tors, &rack_hosts](int node){ 
        if (topology.IsNodeHost(node)){
            int rack = topology.GetHostRack(node);
            return rack_hosts[rack].Get(topology.GetHostIndexInRack(node));
        }
        else{
            return tors.Get(node);
        }
    };

    /* Create flows */
    int nflows =  topology.flows.size();
    cout<<"Creating flows .... "<<endl;
    int max_traffic = 0;
    for (int ff=0; ff<nflows; ff++)
        max_traffic = max(max_traffic, topology.flows[ff].nbytes);
    cout<<"Max Traffic: "<<max_traffic<<endl;
    ApplicationContainer* flow_app = new ApplicationContainer[nflows];
    long long total_bytes=0;
    for (int ff = 0; ff < nflows; ff++){
        Flow& flow = topology.flows[ff];
        total_bytes += flow.nbytes;
        
        //!TODO: Get IPs correctly
        //Get IP of node in the topology
        char *dest_node_ip = topology.GetFlowDestIpAddress(flow);

        char* bytes_c = new char[50];
        sprintf(bytes_c,"%d",flow.nbytes);

        int port = topology.GetFirstUnusedPort(flow.src_node, flow.dest_node);

        //cout<<src_host<<"-->"<<dest_host<<endl;
        Address dest_ip_address(InetSocketAddress(Ipv4Address(dest_node_ip), port));

        // Initialize On/Off Application with ip addresss of server
        OnOffHelper oo = OnOffHelper("ns3::TcpSocketFactory", dest_ip_address);
        oo.SetAttribute("DataRate",StringValue (flow.data_rate));
        oo.SetAttribute("PacketSize",UintegerValue (packet_size_bytes));
        // Simulate flows that never stop by having a very large onTime
        oo.SetAttribute("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=10000000.0]"));
        oo.SetAttribute("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0.0]"));
        oo.SetAttribute("MaxBytes",StringValue (bytes_c));
        //cout << "Dest address " << dest_node_ip << " maxbytes " << bytes_c << " datarate "<< flow.data_rate <<endl;
        NodeContainer on_off_container;
        Ptr<Node> src_node_ptr = GetNodePointer(flow.src_node);
        on_off_container.Add(src_node_ptr);
        flow_app[ff] = oo.Install(on_off_container);
        //cout << "Finished creating On/Off traffic"<<"\n";
    }
    cout<< "Total application flows in the system: "<<nflows<<" carrying "
        << total_bytes<<" bytes "<< endl;

    // Create packet sink application on every node
    for (int node: topology.nodes){
        int first_unused_port = topology.GetFirstUnusedPort(node);
        for (int port=topology.GetStartPort(); port <= first_unused_port; port++){
            Ptr<Node> node_ptr = GetNodePointer(node);
            /* TCP sink */
            PacketSinkHelper sink ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), port));
            ApplicationContainer sink_apps = sink.Install(node_ptr);
            sink_apps.Start(Seconds(0.0));
            sink_apps.Stop(Seconds(warmup_time_seconds + sim_time_seconds));
        }
    }
    cout<<"Finished creating applications"<<endl;

    // Initialize PointtoPointHelper for normal and failed links
    PointToPointHelper p2p;
    p2p.SetDeviceAttribute ("DataRate", StringValue (dataRate));
    p2p.SetChannelAttribute ("Delay", TimeValue (MicroSeconds (delay_us)));

    // Connect switches to hosts
    topology.ConnectSwitchesAndHosts(p2p, tors, rack_hosts, silent_drop_rate);
    cout << "Finished connecting tors and hosts  "<< endl;

    // Connect switches to switches
    topology.ConnectSwitchesAndSwitches(p2p, tors, silent_drop_rate);
    cout << "Finished connecting switches and switches  "<< endl;

    // Set functions that need to be periodically invoked in the simulation
    Simulator::Schedule(Seconds(warmup_time_seconds), &EchoProgress, sim_time_seconds/100.0);
    double snapshot_period_seconds = sim_time_seconds/8.0;
    Simulator::Schedule(Seconds(warmup_time_seconds + snapshot_period_seconds), &SnapshotFlows,
                        &topology, flow_app, snapshot_period_seconds);

    //Check if ECMP routing is working 
    //Config::SetDefault("ns3::Ipv4GlobalRouting::FlowEcmpRouting", BooleanValue(true));
    cout<<"Populating routing tables:"<<endl;
    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
    //Ptr<OutputStreamWrapper> routing_stream = Create<OutputStreamWrapper> (&cout);
    //for(int i=0; i<num_tor; i++)
    //   global_routing_helper.PrintRoutingTableAt(Seconds(5.0), tors.Get(i), routing_stream);
    topology.PrintAllPairShortestPaths();

    cout << "Starting simulation.. "<<"\n";
    int nactive_flows = 0, npassive_flows = 0;
    for (int ff=0; ff<nflows; ff++){
        int bytes = topology.flows[ff].nbytes;
        //add some small random delay to unsynchronize    
        if (bytes == 0){ //infinite length flow, active
            //!TODO: Hack, bytes set to 0 to identify active probe flows
            flow_app[ff].Start(Seconds(warmup_time_seconds + rand() * sim_time_seconds * 0.0001/RAND_MAX)); 
            nactive_flows++;
        }
        else{
            flow_app[ff].Start(Seconds(warmup_time_seconds + rand() * sim_time_seconds/RAND_MAX));
            npassive_flows++;
        }
        flow_app[ff].Stop(Seconds(warmup_time_seconds + sim_time_seconds));
    }
    cout<<"Active Flows: "<<nactive_flows<<", Passive Flows: "<<npassive_flows<<endl;

    // Calculate Throughput using Flowmonitor
    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll();

    // Run simulation.
    Simulator::Stop(Seconds(warmup_time_seconds + sim_time_seconds));
    Simulator::Run();
    /* XML file results
       monitor->CheckForLostPackets ();
       monitor->SerializeToXmlFile(result_filename, true, true);
    */
    cout << "Simulation finished " << endl;
    // PrintFlowInfo should be called before Destory() and PrintIpAddresses after Destroy()
    Simulator::Destroy();
    topology.PrintIpAddresses();
    NS_LOG_INFO("Done.");
    return 0;
}
