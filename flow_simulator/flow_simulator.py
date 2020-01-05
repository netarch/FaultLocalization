import sys
import multiprocessing
from multiprocessing import Process, Queue
import copy
import time
import random
import math
import numpy as np
import networkx as nx

nargs = 3
if not len(sys.argv) == nargs+1: 
	print("Required number of arguments:", nargs, ".", len(sys.argv)-1, "provided")
	print("Arguments: <network_file> <nfailures> <outfile>")
	sys.exit()

HOST_OFFSET = 10000

print("Random witness", random.randint(1, 100000))

network_file = sys.argv[1]
nfailed_links = int(sys.argv[2])
outfilename = sys.argv[3]
G = nx.Graph()
host_rack_map = dict()
host_offset = 0
racks = set()
nservers = 0
max_switch = 0

with open(network_file) as nf:
    for line in nf:
        if "->" in line:
            tokens = line.split("->")
            host = int(tokens[0])
            rack = int(tokens[1])
            G.add_edge(host+host_offset, rack)
            nservers += 1
            host_rack_map[host] = rack
            racks.add(rack)
            #print(host, rack)
        else:
            tokens = line.split()
            rack1 = int(tokens[0])
            rack2 = int(tokens[1])
            max_switch = max(max_switch, rack1)
            max_switch = max(max_switch, rack2)
            G.add_edge(rack1, rack2)

assert (HOST_OFFSET > max_switch)
edges = [(u,v) for (u,v) in G.edges]
edges += [(v,u) for (u,v) in edges]
random.shuffle(edges)
failed_links = edges[0:nfailed_links]
fail_prob = dict()

for edge in edges:
    if edge  in failed_links:
        fail_prob[edge] = random.uniform(0.002, 0.020)
        #fail_prob[edge] = 0.004
    else:
        fail_prob[edge] = random.uniform(0.0000, 0.0001)


def PrintPath(prefix, path, out=sys.stdout):
    s = prefix
    for n in path:
        s += " " + str(n)
    print(s, file=out) 
    
def GetHostRack(host):
    return host_rack_map[host]

nracks = len(racks)
servers_per_rack = int(nservers/nracks)
print("Num racks", nracks, "Servers per rack", servers_per_rack)

#nflows = random.randint(200000, 400000)
nflows = 200 * nservers
#nflows = 250
servers_busy = []
servers_idle = []
#fraction_busy = random.uniform(0.1, 0.2)
fraction_busy = 0.05
racks_busy = random.sample(list(range(nracks)), int(fraction_busy * nracks))
#traffic_with_busy_servers = random.uniform(0.35, 0.70)
for rack in range(nracks):
    #if random.random() <= fraction_busy:
    if rack in racks_busy:
        for i in range(servers_per_rack*rack, servers_per_rack*(rack+1)):
            if i < nservers:
                servers_busy.append(i + HOST_OFFSET)
    else:
        for i in range(servers_per_rack*rack, servers_per_rack*(rack+1)):
            if i < nservers:
                servers_idle.append(i+HOST_OFFSET)


#print("Using skewed_random TM: fraction_busy", fraction_busy, "nflows", nflows, "randint", random.randint(0,100000), "racks", nracks, "servers", nservers)
print("Using uniform_random TM, nflows", nflows, "randint", random.randint(0,100000), "racks", nracks, "servers", nservers)

#print("Servers busy", servers_busy, "Servers idle", servers_idle)

def GetFlows(thread_num, nflows, G, fail_prob, servers_busy, servers_idle, host_rack_map, racks, outfile):
    flows = []
    #nflows = 250
    mean_bytes = 200.0 * 1024 
    shape = 1.05
    scale = mean_bytes * (shape - 1)/shape;
    traffic_with_busy_servers = 0.50

    def skewed_random():
        if (random.random() <= traffic_with_busy_servers and len(servers_busy) > 0):
            return random.choice(servers_busy)
        else:
            return random.choice(servers_idle)

    def uniform_random():
        return HOST_OFFSET + random.randint(0, nservers-1)

    random_server_generator = uniform_random
    packetsize = 1500 #bytes
    sumflowsize = 0
    host_offset_copy = host_offset + 0
    print("Host offset copy", host_offset_copy)

    all_rack_pair_paths = dict()
    for src_rack in racks:
        if src_rack % 10 == 0:
            print ("printing paths", src_rack)
        src_paths = dict()
        for dst_rack in racks:
            paths = list(nx.all_shortest_paths(G, source=src_rack, target=dst_rack))
            src_paths[dst_rack] = paths
            if thread_num == 0:
                for path in paths:
                    PrintPath("FP", path, out=outfile)
        all_rack_pair_paths[src_rack] = src_paths

    for i in range(nflows):
        if i%50000 == 0:
            print("Finished", i, "flows")
        src = random_server_generator()
        dst = random_server_generator()
        src_rack = host_rack_map[src]
        dst_rack = host_rack_map[dst]
        #src = random.randint(0, servers-1)
        #dst = random.randint(0, servers-1)
        flowsize = (np.random.pareto(shape) + 1) * scale
        while flowsize > 100 * 1024 * 1024:
            flowsize = (np.random.pareto(shape) + 1) * scale
        #print(src, dst, flowsize)
        packets_sent = math.ceil(flowsize/packetsize)
        #path_taken = random.choice(list(nx.all_shortest_paths(G, source=src_rack, target=dst_rack)))
        path_taken = random.choice(all_rack_pair_paths[src_rack][dst_rack])
        packets_dropped = 0
        for i in range(packets_sent):
            first_link = (src + host_offset_copy,path_taken[0])
            last_link = (path_taken[-1], dst + host_offset_copy)
            if (random.random() <= fail_prob[first_link] or random.random() <= fail_prob[last_link]):
                packets_dropped += 1
            else:
                for k in range(0, len(path_taken)-1):
                    u = path_taken[k]
                    v = path_taken[k+1]
                    if (random.random() <= fail_prob[(u,v)]):
                        packets_dropped += 1
                        break
        print("FID ", host_offset_copy + src, host_offset_copy + dst, src_rack, dst_rack, packetsize * packets_sent, 0.0, file=outfile)
        PrintPath("FPT", path_taken, out=outfile)
        print("SS ", 1000.0 * random.random(), packets_sent, packets_dropped, 0, file=outfile)
        sumflowsize += flowsize
    return sumflowsize

outfile = open(outfilename,"w+")

for (u,v) in failed_links:
    print("Failing_link", u, v, fail_prob[(u,v)], file=outfile)

sumflowsize = GetFlows(0, nflows, G, fail_prob, servers_busy, servers_idle, host_rack_map, racks, outfile)

print("Sum flow size: ", sumflowsize, "Numflow", nflows, "fraction_busy", fraction_busy)
