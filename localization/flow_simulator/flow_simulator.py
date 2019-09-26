import sys
import multiprocessing
from multiprocessing import Process, Queue
import copy
import time
import random
import math
import numpy as np
import networkx as nx

nargs = 2
if not len(sys.argv) == nargs+1: 
	print("Required number of arguments:", nargs, ".", len(sys.argv)-1, "provided")
	print("Arguments: <network_file> <outfile>")
	sys.exit()

network_file = sys.argv[1]
outfilename = sys.argv[2]
G = nx.Graph()
host_rack_map = dict()
host_offset = 10000
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

assert (host_offset > max_switch)
nfailed_links = 10
edges = [(u,v) for (u,v) in G.edges]
edges += [(v,u) for (u,v) in edges]
random.shuffle(edges)
failed_links = edges[0:nfailed_links]
fail_prob = dict()

for edge in edges:
    if edge  in failed_links:
        fail_prob[edge] = random.uniform(0.001, 0.005)
    else:
        fail_prob[edge] = random.uniform(0.00005, 0.0001)


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
                servers_busy.append(i)
    else:
        for i in range(servers_per_rack*rack, servers_per_rack*(rack+1)):
            if i < nservers:
                servers_idle.append(i)


print("Using skewed_random TM: fraction_busy", fraction_busy, "nflows", nflows, "randint", random.randint(0,100000), "racks", nracks, "servers", nservers)

#print("Servers busy", servers_busy, "Servers idle", servers_idle)

def get_flows(nflows, G, fail_prob, servers_busy, servers_idle, outfile, response_queue):
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
    random_server_generator = skewed_random
    packetsize = 1500 #bytes
    sumflowsize = 0
    for i in range(nflows):
        if i%1000 == 0:
            print("Finished", i, "flows")
        src = random_server_generator()
        dst = random_server_generator()
        #src = random.randint(0, servers-1)
        #dst = random.randint(0, servers-1)
        flowsize = (np.random.pareto(shape) + 1) * scale
        while flowsize > 200 * 1024 * 1024 or flowsize < 10 * 1024:
            flowsize = (np.random.pareto(shape) + 1) * scale
        #print(src, dst, flowsize)
        packets_sent = math.ceil(flowsize/packetsize)
        paths = list(nx.all_shortest_paths(G, source=src+host_offset, target=dst+host_offset))
        path_taken = random.choice(paths)
        packets_dropped = 0
        for i in range(packets_sent):
            for k in range(0, len(path_taken)-1):
                u = path_taken[k]
                v = path_taken[k+1]
                if (random.random() <= fail_prob[(u,v)]):
                    packets_dropped += 1
                    break
        print("Flowid= ", src, dst, packetsize * packets_sent, 0.0, file=outfile)
        print("Snapshot ", 1.0, packets_sent, packets_dropped, 0, file=outfile)
        for path in paths:
            s = "flowpath"                                                                                             
            if path == path_taken:
                s += "_taken"
            for n in path:
                s += " " + str(n)
            print(s, file=outfile) 
        sumflowsize += flowsize
    response_queue.put(sumflowsize)
    return

nprocesses = 40
outfiles = [open(outfilename + "/" + str(i),"w+") for i in range(nprocesses)]
for (u,v) in failed_links:
    print("Failing_link", u, v, fail_prob[(u,v)], file=outfiles[0])

procs = []
G_copies = []
servers_busy_copies = []
servers_idle_copies = []
fail_prob_copies = []
start_time = time.time()
for i in range(nprocesses):
    G_copies.append(copy.deepcopy(G))
    servers_busy_copies.append(list(servers_busy))
    servers_idle_copies.append(list(servers_idle))
    fail_prob_copies.append(copy.deepcopy(fail_prob))
print("Arrays copied for parallel execution in ", time.time() - start_time, " seconds")

response_queue = Queue()
for i in range(nprocesses):
    proc = Process(target=get_flows, args=(int(nflows/nprocesses), G_copies[i], fail_prob_copies[i], servers_busy_copies[i], servers_idle_copies[i], outfiles[i], response_queue))
    procs.append(proc)
for proc in procs:
    proc.start()
sumflowsize = 0
for i in range(nprocesses):
    sumflowsize += response_queue.get()

print("Sum flow size: ", sumflowsize, "Numflow", nflows, "fraction_busy", fraction_busy)