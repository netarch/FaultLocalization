import sys
from heapq import heappush, heappop
from multiprocessing import Process, Queue
import pyximport; pyximport.install()
import math
import time
import joblib
from joblib import Parallel, delayed
import random
import numpy as np
from scipy.optimize import minimize

class Link:
    def __init__(self, srcip, destip, src, dest):
        self.srcip = srcip
        self.destip = destip
        self.src = src
        self.dest = dest

class Flow:
    def __init__(self, src, srcip, dest, destip, nbytes, start_time_ms, finish_time_ms, packets_sent, lost_packets, randomly_lost_packets):
        self.src = src
        self.srcip = srcip
        self.dest = dest
        self.destip = destip
        self.nbytes = nbytes
        self.start_time_ms = start_time_ms
        self.finish_time_ms = finish_time_ms
        self.packets_sent = packets_sent
        self.lost_packets = lost_packets
        self.randomly_lost_packets = randomly_lost_packets
        self.paths = []
        self.reverse_paths = []
        self.path_taken = None
        self.reverse_path_taken = None

    def add_path(self, p):
        self.paths.append(p)

    def add_reverse_path(self, p):
        self.reverse_paths.append(p)

    def set_path_taken(self, p):
        self.path_taken = p;

    def set_reverse_path_taken(self, p):
        self.reverse_path_taken = p

    def print_flow_metrics(self, outfile):
        print("Flowid=", self.src, self.dest, self.nbytes, self.start_time_ms, self.finish_time_ms, self.packets_sent, self.lost_packets, self.randomly_lost_packets, file=outfile)

    def printinfo(self, outfile):
        self.print_flow_metrics(outfile)
        for p in self.paths:
            s = "flowpath"
            if p == self.path_taken:
                s += "_taken"
            for n in p:
                s += " " + str(n)
            print(s, file=outfile)
        for p in self.reverse_paths:
            s = "flowpath_reverse"
            if p == self.reverse_path_taken:
                s += "_taken"
            for n in p:
                s += " " + str(n)
            print(s, file=outfile)
            

    
def convert_path(p, hostip_to_host, linkip_to_link, host_switch_ip):
    path = []
    assert (len(p) >= 1)
    srchost = p[0]
    destrack = p[-1]
    if (not (srchost in hostip_to_host and destrack in host_switch_ip)):
        print(p)
    assert (srchost in hostip_to_host and destrack in host_switch_ip)
    for i in range(1, len(p)-1):
        link = linkip_to_link[p[i]]
        assert (link.srcip == p[i])
        path.append(link.src)
    path.append(host_switch_ip[destrack])
    return path


def process_logfile(filename, max_start_time_ms, outfilename):
    flows = []
    start_simulation = False
    linkip_to_link = dict()
    hostip_to_host = dict()
    host_to_hostip = dict()
    host_switch_ip = dict()
    flow_route_taken = dict()
    curr_flow = None
    recording = False
    outfile = open(outfilename,"w+")
    with open(filename, encoding = "ISO-8859-1") as f:
        flow = None
        for line in f.readlines():
            if "Failing_link" in line:
                print(line.rstrip(), file=outfile);
                continue
            tokens = line.split()
            if (not start_simulation) and ("Start Simulation.." in line):
                start_simulation = True
            if not start_simulation:
                continue;
            elif "link_ip" in line:
                srcip = tokens[1]
                destip = tokens[2]
                src = int(tokens[3])
                dest = int(tokens[4])
                linkip_to_link[srcip] = Link(srcip, destip, src, dest)
                linkip_to_link[destip] = Link(destip, srcip, dest, src)
            elif "host_ip" in line:
                host_ip1 = tokens[1] #switch
                host_ip2 = tokens[2] #host
                host = int(tokens[3])
                rack = int(tokens[4])
                hostip_to_host[host_ip2] = host
                host_to_hostip[host] = host_ip2
                host_switch_ip[host_ip1] = rack
            elif "hop_taken" in line:
                srcip = tokens[1]
                destip = tokens[2]
                srcport = int(tokens[3])
                destport = int(tokens[4])
                if srcport > 1000000 or destport > 10000000 or "102.102.102.102" in srcip or "102.102.102.102" in destip:
                    #junk entry due to arp maybe
                    continue
                hop_ip = tokens[6]
                flow = (srcip, destip, srcport, destport)
                if flow not in flow_route_taken:
                    flow_route_taken[flow] = []
                flow_route_taken[flow].append(hop_ip)
            elif "Flowid" in line:
                #Flowid 0 2 10.0.0.2 10.0.1.2 100000000 +1039438292.0ns 74215 0 0
                src = int(tokens[1])
                dest = int(tokens[2])
                srcip = tokens[3]
                destip = tokens[4]
                srcport = int(tokens[5])
                destport = int(tokens[6])
                nbytes = int(tokens[7])
                start_time_ms = float(tokens[8].strip('+ns')) * 1.0e-6
                finish_time_ms = float(tokens[9].strip('+ns')) * 1.0e-6
                packets_sent = int(tokens[10])
                lost_packets = int(tokens[11])
                randomly_lost_packets = int(tokens[12])
                if (curr_flow != None and len(curr_flow.paths) > 0 and recording):
                    flows.append(curr_flow)
                curr_flow = Flow(src, srcip, dest, destip, nbytes, start_time_ms, finish_time_ms, packets_sent, lost_packets, randomly_lost_packets)
                flow_tuple = (srcip, destip, srcport, destport)
                reverse_flow_tuple = (destip, srcip, destport, srcport)
                #if flow_tuple not in flow_route_taken or reverse_flow_tuple not in flow_route_taken or 
                if start_time_ms >= max_start_time_ms or packets_sent==0:
                    print("Ignoring flow", end=" ")
                    curr_flow.printinfo(sys.stdout)
                    recording = False
                    continue
                assert(flow_tuple in flow_route_taken and reverse_flow_tuple in flow_route_taken)
                recording = True
                curr_flow.set_path_taken(flow_route_taken[flow_tuple])
                curr_flow.set_reverse_path_taken(flow_route_taken[reverse_flow_tuple])
            elif "flowpath_reverse" in line:
                #flowpath_reverse 0 2 1
                path = []
                for i in range(1, len(tokens)):
                    path.append(int(tokens[i]))
                if(len(path) > 0 and recording):
                    curr_flow.add_reverse_path(path)
            elif "flowpath" in line:
                #flowpath 0 2 1
                path = []
                for i in range(1, len(tokens)):
                    path.append(int(tokens[i]))
                if(len(path) > 0 and recording):
                    curr_flow.add_path(path)
            else:
                pass
                #print("Unrecognized log statement: ", line)
        if curr_flow != None and len(curr_flow.paths)>0: #log the previous flow
            if curr_flow.packets_sent > 0:
                flows.append(curr_flow)
        for flow in flows:
            flow.set_path_taken(convert_path(flow.path_taken, hostip_to_host, linkip_to_link, host_switch_ip))
            flow.set_reverse_path_taken(convert_path(flow.reverse_path_taken, hostip_to_host, linkip_to_link, host_switch_ip))
            flow.printinfo(outfile)
            if flow.lost_packets > 0:
                flow.printinfo(sys.stdout)

#return {'failed_links':failed_links, 'flows':flows, 'links':links, 'inverse_links':inverse_links, 'link_statistics':link_statistics}

if __name__ == '__main__':
    filename = sys.argv[1]
    max_start_time_sec = float(sys.argv[2])
    outfilename = sys.argv[3]
    process_logfile(filename, max_start_time_sec * 1000.0, outfilename)

