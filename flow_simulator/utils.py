import sys
import multiprocessing
from multiprocessing import Process, Queue
import copy
import time
import random
import math
import numpy as np
import networkx as nx

HOST_OFFSET = 10000


class Flow(object):
    def __init__(self, src, dst, flowsize, srcport=None, dstport=None):
        self.src = src
        self.dst = dst
        self.flowsize = flowsize
        self.srcport = srcport
        self.dstport = dstport

    def HashToPath(self, paths):
        ftuple = self.src, self.dst, self.srcport, self.dstport
        ind = hash(ftuple)%len(paths)
        return paths[ind]

class Topology(object):
    def __init__(self):
        self.G = None
        self.host_rack_map = dict()
        self.host_offset = 0
        self.racks = set()
        self.nservers = 0
        self.max_switch = 0
        self.failed_components = []
        self.outfile = None
    
    def SetOutFile(self, outfile):
        self.outfile = outfile

    def ReadGraphFromFile(self, network_file):
        self.G = nx.Graph()
        with open(network_file) as nf:
            for line in nf:
                if "->" in line:
                    tokens = line.split("->")
                    host = int(tokens[0])
                    rack = int(tokens[1])
                    self.G.add_edge(host+HOST_OFFSET, rack)
                    self.nservers += 1
                    self.host_rack_map[host] = rack
                    self.racks.add(rack)
                    #print(host, rack)
                else:
                    tokens = line.split()
                    rack1 = int(tokens[0])
                    rack2 = int(tokens[1])
                    self.max_switch = max(self.max_switch, rack1)
                    self.max_switch = max(self.max_switch, rack2)
                    self.G.add_edge(rack1, rack2)
        assert (HOST_OFFSET > self.max_switch)

    def SetFailedLinksSilentDrop(self, nfailed_links):
        edges = [(u,v) for (u,v) in self.G.edges]
        edges += [(v,u) for (u,v) in edges]
        random.shuffle(edges)
        self.failed_components = edges[0:nfailed_links]


    def PrintPath(self, prefix, path, out=sys.stdout):
        s = prefix
        for n in path:
            s += " " + str(n)
        print(s, file=out) 
        
    def GetHostRack(self, host):
        return self.host_rack_map[host]

    def GetParetoFlowSize(self):
        mean_bytes = 200.0 * 1024 
        shape = 1.05
        scale = mean_bytes * (shape - 1)/shape;
        flowsize = (np.random.pareto(shape) + 1) * scale
        while flowsize > 100 * 1024 * 1024:
            flowsize = (np.random.pareto(shape) + 1) * scale
        return flowsize

    def GetFlowsSkewed(self, nflows):
        #print("Using skewed_random TM: fraction_busy", fraction_busy, "nflows", nflows, "randint", random.randint(0,100000), "racks", nracks, "servers", nservers)
        nracks = len(self.racks)
        servers_per_rack = int(self.nservers/nracks)
        print("Num racks", nracks, "Servers per rack", servers_per_rack)
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
                    if i < self.nservers:
                        servers_busy.append(i + HOST_OFFSET)
            else:
                for i in range(servers_per_rack*rack, servers_per_rack*(rack+1)):
                    if i < self.nservers:
                        servers_idle.append(i+HOST_OFFSET)

        def skewed_random():
            if (random.random() <= traffic_with_busy_servers and len(servers_busy) > 0):
                return random.choice(servers_busy)
            else:
                return random.choice(servers_idle)
        flows = []
        for i in range(nflows):
            if i%50000 == 0:
                print("Finished", i, "flows")
            src = skewed_random()
            dst = skewed_random()
            src_rack = self.host_rack_map[src]
            dst_rack = self.host_rack_map[dst]
            flowsize = self.GetParetoFlowSize()
            srcport = random.randint(1000, 1100)
            dstport = random.randint(1000, 1100)
            flows.append(Flow(src, dst, flowsize, srcport, dstport))
        return flows

    def GetFlowsUniform(self, nflows):
        print("Using uniform_random TM, nflows", nflows, "randint", random.randint(0,100000), "racks", len(self.racks), "servers", self.nservers)
        def uniform_random():
            return HOST_OFFSET + random.randint(0, self.nservers-1)
        flows = []
        for i in range(nflows):
            if i%50000 == 0:
                print("Finished", i, "flows")
            src = uniform_random()
            dst = uniform_random()
            src_rack = self.host_rack_map[src]
            dst_rack = self.host_rack_map[dst]
            flowsize = self.GetParetoFlowSize()
            srcport = random.randint(1000, 1100)
            dstport = random.randint(1000, 1100)
            flows.append(Flow(src, dst, flowsize, srcport, dstport))
        return flows

    def GetFlowsBlackHole(self, nflows):
        print("Using black hole TM, nflows", nflows, "randint", random.randint(0,100000), "racks", len(self.racks), "servers", self.nservers)
        def uniform_random():
            return HOST_OFFSET + random.randint(0, self.nservers-1)
        flows = []
        nflows_per_src_dst = 100
        for i in range(int(nflows/nflows_per_src_dst)):
            src = uniform_random()
            dst = uniform_random()
            src_rack = self.host_rack_map[src]
            dst_rack = self.host_rack_map[dst]
            for t in range(nflows_per_src_dst):
                if (i * nflows_per_src_dst + t)%50000 == 0:
                    print("Finished", i, "flows")
                flowsize = self.GetParetoFlowSize()
                srcport = random.randint(1000, 1100)
                dstport = random.randint(1000, 1100)
                flows.append(Flow(src, dst, flowsize, srcport, dstport))
        return flows

    def GetDropProbBlackHole(self, flows, nfailed_pairs, all_rack_pair_paths): 
        fail_prob = dict()
        edges = [(u,v) for (u,v) in self.G.edges]
        edges += [(v,u) for (u,v) in edges]
        src_dst_pairs = [(flow.src, flow.dst) for flow in flows]
        random.shuffle(src_dst_pairs)
        failed_src_dst_pairs = src_dst_pairs[:nfailed_pairs]
        for src, dst in failed_src_dst_pairs:
            src_rack = self.host_rack_map[src]
            dst_rack = self.host_rack_map[dst]
            all_paths = all_rack_pair_paths[src_rack][dst_rack]
            links = set()
            for path in all_paths:
                links.update([(path[i], path[i+1]) for i in range(len(path)-1)])
            print(links)
            failed_link = random.choice(list(links))
            failed_component = (failed_link, src, dst)
            print("Failing:", failed_component)
            self.failed_components.append(failed_component)
            fail_prob[failed_component] = random.uniform(0.01, 0.1)
        return fail_prob

    def GetDropProbSilentDrop(self): 
        fail_prob = dict()
        edges = [(u,v) for (u,v) in self.G.edges]
        edges += [(v,u) for (u,v) in edges]
        for edge in edges:
            if edge in self.failed_components:
                fail_prob[edge] = random.uniform(0.002, 0.020)
                print("Failing_link", edge[0], edge[1], fail_prob[edge], file=self.outfile)
                #fail_prob[edge] = 0.004
            else:
                fail_prob[edge] = random.uniform(0.0000, 0.0001)
        return fail_prob


    def GetAllRackPairPaths(self):
        all_rack_pair_paths = dict()
        for src_rack in self.racks:
            if src_rack % 10 == 0:
                print ("printing paths", src_rack)
            src_paths = dict()
            for dst_rack in self.racks:
                paths = list(nx.all_shortest_paths(self.G, source=src_rack, target=dst_rack))
                src_paths[dst_rack] = paths
                for path in paths:
                    self.PrintPath("FP", path, out=self.outfile)
            all_rack_pair_paths[src_rack] = src_paths
        return all_rack_pair_paths


    def PrintLogsBlackHole(self, nfailed_pairs):
        nflows = 100 * self.nservers
        flows = self.GetFlowsBlackHole(nflows)
        all_rack_pair_paths = self.GetAllRackPairPaths()
        fail_prob = self.GetDropProbBlackHole(flows, nfailed_pairs, all_rack_pair_paths)
        curr = 0
        host_offset_copy = HOST_OFFSET + 0
        print("Host offset copy", host_offset_copy)
        def GetFailProb(link, flow):
            if (link, flow.src, flow.dst) in fail_prob:
                return fail_prob[(link, flow.src, flow.dst)]
            else:
                return 0.0001
        sumflowsize = 0
        for flow in flows:
            src_rack = self.host_rack_map[flow.src]
            dst_rack = self.host_rack_map[flow.dst]
            #print(src, dst, flowsize)
            path_taken = flow.HashToPath(all_rack_pair_paths[src_rack][dst_rack])
            first_link = (flow.src + host_offset_copy, path_taken[0])
            last_link = (path_taken[-1], flow.dst + host_offset_copy)
            flow_dropped = False
            if (random.random() <= GetFailProb(first_link, flow) or random.random() <= GetFailProb(last_link, flow)):
                flow_dropped = True
            for k in range(0, len(path_taken)-1):
                u = path_taken[k]
                v = path_taken[k+1]
                if (random.random() <= GetFailProb((u,v), flow)):
                    flow_dropped = True
            print("FID ", host_offset_copy + flow.src, host_offset_copy + flow.dst, src_rack, dst_rack, flow.flowsize, 0.0, file=self.outfile)
            self.PrintPath("FPT", path_taken, out=self.outfile)
            print("SS ", 1000.0 * random.random(), 1, int(flow_dropped), 0, file=self.outfile)
            sumflowsize += flow.flowsize
        return nflows, sumflowsize
            



    def PrintLogsSilentDrop(self, nfailed_links):
        self.SetFailedLinksSilentDrop(nfailed_links)
        fail_prob = self.GetDropProbSilentDrop()
        nflows = 100 * self.nservers
        #flows = topo.GetFlowsSkewed(nflows)
        flows = self.GetFlowsUniform(nflows)
        all_rack_pair_paths = self.GetAllRackPairPaths()
        packetsize = 1500 #bytes
        sumflowsize = 0
        host_offset_copy = HOST_OFFSET + 0
        print("Host offset copy", host_offset_copy)
        curr = 0
        for flow in flows:
            curr += 1
            if curr%50000 == 0:
                print("Finished", curr, "flows")
            src_rack = self.host_rack_map[flow.src]
            dst_rack = self.host_rack_map[flow.dst]
            #print(src, dst, flowsize)
            packets_sent = math.ceil(flow.flowsize/packetsize)
            #path_taken = random.choice(list(nx.all_shortest_paths(G, source=src_rack, target=dst_rack)))
            path_taken = random.choice(all_rack_pair_paths[src_rack][dst_rack])
            packets_dropped = 0
            for i in range(packets_sent):
                first_link = (flow.src + host_offset_copy, path_taken[0])
                last_link = (path_taken[-1], flow.dst + host_offset_copy)
                if (random.random() <= fail_prob[first_link] or random.random() <= fail_prob[last_link]):
                    packets_dropped += 1
                else:
                    for k in range(0, len(path_taken)-1):
                        u = path_taken[k]
                        v = path_taken[k+1]
                        if (random.random() <= fail_prob[(u,v)]):
                            packets_dropped += 1
                            break
            print("FID ", host_offset_copy + flow.src, host_offset_copy + flow.dst, src_rack, dst_rack, packetsize * packets_sent, 0.0, file=self.outfile)
            self.PrintPath("FPT", path_taken, out=self.outfile)
            print("SS ", 1000.0 * random.random(), packets_sent, packets_dropped, 0, file=self.outfile)
            sumflowsize += flow.flowsize
        return nflows, sumflowsize
