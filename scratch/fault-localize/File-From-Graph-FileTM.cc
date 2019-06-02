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

inline int truncateBytes(int bytes){
   return (bytes < 10? 0 : bytes);
}

void echo_progress(double delay_seconds){
    Simulator::Schedule (Seconds(delay_seconds), &echo_progress, delay_seconds);
    cout<<"Finished: "<<Simulator::Now ().GetSeconds ()<<" seconds"<<endl;
}

void snapshot_flows(Topology* topology, ApplicationContainer** flow_app, double snapshot_period_seconds){
    Simulator::Schedule (Seconds(snapshot_period_seconds), &snapshot_flows, topology, flow_app, snapshot_period_seconds);
    double curr_time_seconds = Simulator::Now().GetSeconds();
    double next_snapshot_time = curr_time_seconds + snapshot_period_seconds;
    //!TODO Hack for the fault localization to analyse trends
    //Time mid = Time::FromDouble((curr_time_seconds + next_snapshot_time)/2, Time::S);
    for (int i=0;i<topology->total_host;i++){
        for (int j=0;j<topology->total_host;j++){
            int bytes = truncateBytes((int)(topology->serverTM[i][j]));
            if(bytes < 1 || flow_app[i][j].GetStartTime().GetSeconds() >= curr_time_seconds) continue;
            //cout<<"Recording snapshot "<<" "<<flow_app[i][j].GetStartTime().GetSeconds()<<" "<<curr_time_seconds<<" "<<snapshot_period_seconds<<endl;
            topology->snapshot_flow(i, j, bytes, flow_app[i][j], flow_app[i][j].GetStartTime(), Simulator::Now());
            if(flow_app[i][j].GetStartTime().GetSeconds() + snapshot_period_seconds >= curr_time_seconds){
                //Print path information for the first time
                topology->print_flow_path(i, j);
            }
        }
    }
}

int main(int argc, char *argv[])
{
   //For fast I/O
   ios_base::sync_with_stdio(false);
   int nreqarg = 7;
   if(argc <= nreqarg){
      cout<<nreqarg-1<<" arguments required, "<<argc-1<<" given."<<endl;
      cout<<"Usage> <exec> <topology_file> <tm_file> <result_file> <drop queue limit(e.g. 100 for a limit of 100 packets)> <data rate for on/off (e.g. 10 for 10 Kbps)> <nfails> <failparam> <seed>"<<endl;
      exit(0);
   }
   string topology_filename = argv[1];
   string tm_filename = argv[2];
   string result_filename = argv[3];
   int drop_queue_limit = atoi(argv[4]);
   string on_off_datarate = argv[5];
   int nfails = atoi(argv[6]);
   double failparam = atof(argv[7]);
   int seed = atoi(argv[8]);
   RngSeedManager::SetSeed (seed);
   RngSeedManager::SetRun (seed);
   srand(seed);
   srand48(seed);


   cout<<"Running topology: "<<topology_filename<<", output result to "<<result_filename<<endl;
   cout<<"Setting drop queue limit: "<<drop_queue_limit<<endl;

  Config::SetDefault ("ns3::DropTailQueue::MaxPackets", UintegerValue (drop_queue_limit));

//==== Simulation Parameters ====//
   double simTimeInSec = 1.0;
   double warmupTimeInSec = 1.0;
   bool onOffApplication = true;	

//=========== Define topology  ===========//
//
   Topology topology;
   topology.readTopologyFromFile(topology_filename);
   topology.readServerTmFromFile(tm_filename);

// Initialize parameters for On/Off application
//
	int port = 9; //port for background app
	int packetSizeBytes = 1500;
	string dataRate_OnOff = on_off_datarate + "Kbps"; 
	//char maxBytes [] = "0" ; //"50000"; // "0"; // unlimited

// Initialize parameters for PointToPoint protocol
//
	char dataRate [] = "10Gbps";
	uint64_t delay_us = 5; //microseconds


// Fail some links, by setting loss rates
    topology.chooseFailedLinks(nfails);
    double silent_drop_rate = failparam;
	
// Output some useful information
//	
    std::cout << "Total number of hosts =  "<< topology.total_host<<"\n";
    if (onOffApplication){
        std::cout << "On/Off flow data rate = "<< dataRate_OnOff<<"."<<endl;
    }

// Initialize Internet Stack and Routing Protocols
//	
	InternetStackHelper internet;
	Ipv4NixVectorHelper nixRouting; 
	Ipv4StaticRoutingHelper staticRouting;
	Ipv4GlobalRoutingHelper globalRouting;
	Ipv4ListRoutingHelper list;
	//list.Add (staticRouting, 0);	
	//list.Add (nixRouting, 10);	
	list.Add (globalRouting, 20);	
	internet.SetRoutingHelper(list);

//=========== Creation of Node Containers ===========//

    NodeContainer tors;				// NodeContainer for all ToR switches
    tors.Create(topology.num_tor);
    internet.Install(tors);

    NodeContainer rackhosts[topology.num_tor];
    for (int i=0; i<topology.num_tor;i++){  	
        rackhosts[i].Create (topology.getNumHostsInRack(i));
        internet.Install (rackhosts[i]);		
    }

//=========== Initialize settings for On/Off Application ===========//
//

// Generate traffics for the simulation
//
   cout<<"Creating application flows .... "<<endl;

   double max_traffic = 0.0;
   for(int i=0; i<topology.total_host; i++){
      //cout<<"row.size(): "<<topology.serverTM[i].size()<<endl;
      max_traffic = max(max_traffic, *std::max_element(topology.serverTM[i].begin(),topology.serverTM[i].end()));
   }
   double traffic_wt = 1.0;
   cout<<"Max Traffic: "<<max_traffic<<", Weighted: "<<max_traffic * traffic_wt<<endl;

   ApplicationContainer** flow_app = new ApplicationContainer*[topology.total_host];
   for(int i=0; i<topology.total_host; i++)
      flow_app[i] = new ApplicationContainer[topology.total_host];
   int nflows = 0;
   long long total_bytes=0;

   double onTime = 1000000.000, offTime = 0.000;
   Ptr<ExponentialRandomVariable> rvOn = CreateObject<ExponentialRandomVariable> ();
   rvOn->SetAttribute ("Mean", DoubleValue (onTime));
   rvOn->SetAttribute ("Bound", DoubleValue (0.0));
   Ptr<ExponentialRandomVariable> rvOff = CreateObject<ExponentialRandomVariable> ();
   rvOff->SetAttribute ("Mean", DoubleValue (offTime));
   rvOff->SetAttribute ("Bound", DoubleValue (0.0));
   for(int i=0; i<topology.total_host; i++){
      for(int j=0; j<topology.total_host; j++){
         int bytes = truncateBytes((int)(topology.serverTM[i][j] * traffic_wt));
         //cout<<"bytes: "<<bytes<<endl;
         if(bytes < 1) continue;
         nflows++;
         total_bytes += bytes;
         char* bytes_c = new char[50];
         sprintf(bytes_c,"%d",bytes);
         string bytes_s = bytes_c;
         char *src_ip = topology.getHostIpAddress(i);
         char *remote_ip = topology.getHostIpAddress(j);

         OnOffHelper oo = OnOffHelper("ns3::TcpSocketFactory",Address(InetSocketAddress(Ipv4Address(remote_ip), port))); // ip address of server
         BulkSendHelper source ("ns3::TcpSocketFactory", Address(InetSocketAddress(Ipv4Address(remote_ip), port)));
         // Initialize On/Off Application with addresss of server
         if(onOffApplication){
            //cout<<"Initializing On Off flow"<<endl;
            //Simulate flows that never stop by having a very large onTime
            oo.SetAttribute("DataRate",StringValue (dataRate_OnOff));      
            oo.SetAttribute("PacketSize",UintegerValue (packetSizeBytes));
            //oo.SetAttribute ("Remote", Address(InetSocketAddress(Ipv4Address(remote_ip), port)));
            oo.SetAttribute("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1000000.0]"));
            oo.SetAttribute("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0.0]"));
            oo.SetAttribute("MaxBytes",StringValue (bytes_c));
         }
         //Initialize BulkSend application
         else{
            // Set the amount of data to send in bytes.  Zero is unlimited.
            source.SetAttribute("SendSize",UintegerValue (packetSizeBytes));
            source.SetAttribute("MaxBytes",StringValue (bytes_c));
         }
         //cout<<i<<"-->"<<j<<std::endl;
         int irack = topology.getHostRack(i);
         // Install On/Off Application on the source
         if(onOffApplication){
            NodeContainer onoff;
            onoff.Add(rackhosts[irack].Get(topology.getHostIndexInRack(i)));
            flow_app[i][j] = oo.Install (onoff);
            //std::cout << "Finished creating On/Off traffic"<<"\n";
         }
         //For bulk send
         else{
            NodeContainer bulksend;
            bulksend.Add(rackhosts[irack].Get(topology.getHostIndexInRack(i)));
            flow_app[i][j] = source.Install (bulksend);
            //flow_app[i][j].Start (Seconds (1.0));
            //flow_app[i][j].Stop (Seconds (simTimeInSec));
            //std::cout << "Finished creating Bulk send and Packet Sink Applications"<<"\n";
         }
      }
   }
   cout<<"Total Flows in the system: "<<nflows<<" carrying "<<total_bytes<<" bytes"<<endl;

   for(int i=0;i<topology.total_host; i++){
      //Create packet sink application on every server
      PacketSinkHelper sink ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), port));
      int irack = topology.getHostRack(i);
      ApplicationContainer sinkApps = sink.Install (rackhosts[irack].Get(topology.getHostIndexInRack(i)));
      sinkApps.Start (Seconds (0.0));
      sinkApps.Stop (Seconds (warmupTimeInSec + simTimeInSec));
   }
   cout<<"Finished creating applications"<<endl;
   

// Initialize PointtoPointHelper for normal and failed links
//	
	PointToPointHelper p2p;
  	p2p.SetDeviceAttribute ("DataRate", StringValue (dataRate));
  	p2p.SetChannelAttribute ("Delay", TimeValue (MicroSeconds (delay_us)));
    Ptr<RateErrorModel> rem = CreateObjectWithAttributes<RateErrorModel> (
                                "ErrorRate", DoubleValue (silent_drop_rate),
                                "ErrorUnit", EnumValue (RateErrorModel::ERROR_UNIT_PACKET));
	/*
    PointToPointHelper p2p_failed;
  	p2p_failed.SetDeviceAttribute ("DataRate", StringValue (dataRate));
  	p2p_failed.SetChannelAttribute ("Delay", TimeValue (MicroSeconds (delay_us)));
  	p2p_failed.SetDeviceAttribute ("ReceiveErrorModel", PointerValue (rem));
    */

//=========== Connect switches to hosts ===========//
//
    topology.connect_switches_and_hosts(p2p, tors, rackhosts);
	std::cout << "Finished connecting tors and hosts  "<< "\n";

//=========== Connect  switches to switches ===========//
//
    topology.connect_switches_and_switches(p2p, rem, tors, silent_drop_rate);
    std::cout << "Finished connecting switches and switches  "<< "\n";
    std::cout << "------------- "<<"\n";

//=========== Start the simulation ===========//
//


    Simulator::Schedule (Seconds(warmupTimeInSec), &echo_progress, simTimeInSec/100.0);

    double snapshot_period_seconds = simTimeInSec/50.0;
    Simulator::Schedule (Seconds(warmupTimeInSec + snapshot_period_seconds), &snapshot_flows, &topology, flow_app, snapshot_period_seconds);


    //Check if ECMP routing is working 
    //Config::SetDefault("ns3::Ipv4GlobalRouting::FlowEcmpRouting", BooleanValue(true));
    cout<<"Populating routing tables:"<<endl;
    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

   Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper> (&std::cout);
   //for(int i=0; i<num_tor; i++)
   //   globalRouting.PrintRoutingTableAt (Seconds (5.0), tors.Get(i), routingStream);

   std::cout << "Start Simulation.. "<<"\n";
   for (int i=0;i<topology.total_host;i++){
      for (int j=0;j<topology.total_host;j++){
         int bytes = truncateBytes((int)(topology.serverTM[i][j] * traffic_wt));
         if(bytes < 1) continue;
		   //flow_app[i][j].Start (Seconds (warmupTimeInSec));
           //add some small random delay to unsynchronize    
		   //flow_app[i][j].Start (Seconds (warmupTimeInSec + rand() * simTimeInSec * 0.001/RAND_MAX)); 
		   flow_app[i][j].Start (Seconds (warmupTimeInSec + rand() * simTimeInSec/RAND_MAX));
  		   flow_app[i][j].Stop (Seconds (warmupTimeInSec + simTimeInSec));
      }
	}

// Calculate Throughput using Flowmonitor
//
  	FlowMonitorHelper flowmon;
	Ptr<FlowMonitor> monitor = flowmon.InstallAll();


// Run simulation.
//
  	NS_LOG_INFO ("Run Simulation.");
  	Simulator::Stop (Seconds(simTimeInSec+1.0));
  	Simulator::Run ();

    /* XML file results
  	monitor->CheckForLostPackets ();
  	monitor->SerializeToXmlFile(result_filename, true, true);
    */

	std::cout << "Simulation finished "<<"\n";

    for (int i=0;i<topology.total_host;i++){
        for (int j=0;j<topology.total_host;j++){
            int bytes = truncateBytes((int)(topology.serverTM[i][j] * traffic_wt));
            if(bytes < 1) continue;
            //topology.print_flow_info(i, j, bytes, flow_app[i][j]);
        }
    }

    //Print flow info before Destory() and Ip addresses after Destroy()
  	Simulator::Destroy ();
    topology.print_ip_addresses();

  	NS_LOG_INFO ("Done.");

	return 0;
}




