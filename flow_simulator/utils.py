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


def TupleHash(t):
    res = 1
    for num in t:
        res = res * 8191 + (num * 2654435761 % 1000000009)
    return res


class Flow(object):
    def __init__(self, src, dst, flowsize, srcport=None, dstport=None):
        self.src = src
        self.dst = dst
        self.flowsize = flowsize
        if srcport == None:
            srcport = random.randint(1000, 1100)
        if dstport == None:
            dstport = random.randint(1000, 1100)
        self.srcport = srcport
        self.dstport = dstport

    def HashToPath(self, paths):
        ftuple = self.src, self.dst, self.srcport, self.dstport
        ind = TupleHash(ftuple) % len(paths)
        # print(ftuple, ind)
        return paths[ind]

    def IsFlowActive(self):
        return self.src < HOST_OFFSET or self.dst < HOST_OFFSET


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
        self.failed_src_dst_pairs = None

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
                    # self.G.add_edge(host+HOST_OFFSET, rack)
                    self.G.add_edge(host, rack)
                    self.nservers += 1
                    self.host_rack_map[host] = rack
                    self.racks.add(rack)
                    # print(host, rack)
                else:
                    tokens = line.split()
                    rack1 = int(tokens[0])
                    rack2 = int(tokens[1])
                    self.max_switch = max(self.max_switch, rack1)
                    self.max_switch = max(self.max_switch, rack2)
                    self.G.add_edge(rack1, rack2)
        assert HOST_OFFSET > self.max_switch

    def SetFailedLinksSilentDrop(self, nfailed_links):
        edges = [(u, v) for (u, v) in self.G.edges]
        edges += [(v, u) for (u, v) in edges]
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
        scale = mean_bytes * (shape - 1) / shape
        flowsize = (np.random.pareto(shape) + 1) * scale
        while flowsize > 100 * 1024 * 1024:
            flowsize = (np.random.pareto(shape) + 1) * scale
        return flowsize

    def GetFlowsSkewed(self, nflows):
        # print("Using skewed_random TM: fraction_busy", fraction_busy, "nflows", nflows, "randint", random.randint(0,100000), "racks", nracks, "servers", nservers)
        nracks = len(self.racks)
        servers_per_rack = int(self.nservers / nracks)
        print("Num racks", nracks, "Servers per rack", servers_per_rack)
        servers_busy = []
        servers_idle = []
        # fraction_busy = random.uniform(0.1, 0.2)
        fraction_busy = 0.05
        racks_busy = random.sample(list(range(nracks)), int(fraction_busy * nracks))
        # traffic_with_busy_servers = random.uniform(0.35, 0.70)
        for rack in range(nracks):
            # if random.random() <= fraction_busy:
            if rack in racks_busy:
                for i in range(servers_per_rack * rack, servers_per_rack * (rack + 1)):
                    if i < self.nservers:
                        servers_busy.append(i + HOST_OFFSET)
            else:
                for i in range(servers_per_rack * rack, servers_per_rack * (rack + 1)):
                    if i < self.nservers:
                        servers_idle.append(i + HOST_OFFSET)

        def skewed_random():
            if random.random() <= traffic_with_busy_servers and len(servers_busy) > 0:
                return random.choice(servers_busy)
            else:
                return random.choice(servers_idle)

        flows = []
        for i in range(nflows):
            if i % 50000 == 0:
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
        print(
            "Using uniform_random TM, nflows",
            nflows,
            "randint",
            random.randint(0, 100000),
            "racks",
            len(self.racks),
            "servers",
            self.nservers,
        )

        def uniform_random():
            return HOST_OFFSET + random.randint(0, self.nservers - 1)

        flows = []
        for i in range(nflows):
            if i % 50000 == 0:
                print("Finished", i, "flows")
            src = uniform_random()
            dst = uniform_random()
            src_rack = self.host_rack_map[src]
            dst_rack = self.host_rack_map[dst]
            flowsize = self.GetParetoFlowSize()
            flows.append(Flow(src, dst, flowsize, srcport, dstport))
        return flows

    def ReadFlowsFromFile(G, flows_file):
        flows = []
        with open(flows_file, "r") as ff:
            for line in ff:
                src, dst, flowsize = [int(x) for x in line.split()[:3]]
                flows.append(Flow(src, dst, flowsize))
        return flows

    def GetFlowsBlackHole(self, nflows):
        print(
            "Using black hole TM, nflows",
            nflows,
            "randint",
            random.randint(0, 100000),
            "racks",
            len(self.racks),
            "servers",
            self.nservers,
        )

        def uniform_random():
            return HOST_OFFSET + random.randint(0, self.nservers - 1)

        flows = []
        nflows_per_src_dst = 1000
        for i in range(int(nflows / nflows_per_src_dst)):
            src = uniform_random()
            dst = uniform_random()
            src_rack = self.host_rack_map[src]
            dst_rack = self.host_rack_map[dst]
            for t in range(nflows_per_src_dst):
                if (i * nflows_per_src_dst + t) % 50000 == 0:
                    print("Finished", i * nflows_per_src_dst + t, "flows")
                flowsize = self.GetParetoFlowSize()
                srcport = random.randint(1000, 2000)
                dstport = random.randint(1000, 2000)
                flows.append(Flow(src, dst, flowsize, srcport, dstport))
        return flows

    def GetDropProbBlackHole(self, flows, nfailed_pairs, all_rack_pair_paths):
        fail_prob = dict()
        edges = [(u, v) for (u, v) in self.G.edges]
        edges += [(v, u) for (u, v) in edges]
        src_dst_pairs = [(flow.src, flow.dst) for flow in flows]
        random.shuffle(src_dst_pairs)
        failed_src_dst_pairs = src_dst_pairs[:nfailed_pairs]
        self.failed_src_dst_pairs = set(failed_src_dst_pairs)
        for src, dst in failed_src_dst_pairs:
            src_rack = self.host_rack_map[src]
            dst_rack = self.host_rack_map[dst]
            all_paths = all_rack_pair_paths[src_rack][dst_rack]
            links = set()
            devices = set()
            for path in all_paths:
                links.update([(path[i], path[i + 1]) for i in range(len(path) - 1)])
                devices.update([path[i] for i in range(1, len(path) - 1)])
            # print(src, dst, "links:", links, "devices:", devices)
            # failed_link = random.choice(list(links))
            # failed_component = (failed_link, src, dst)
            failed_device = random.choice(list(devices))
            failed_component = (failed_device, src, dst)
            print("Failing:", failed_component)
            self.failed_components.append(failed_component)
            fail_prob[failed_component] = random.uniform(0.05, 0.1)
        return fail_prob

    def GetDropProbMisconfiguredACL(self, nfailures):
        fail_prob = dict()
        ndrop_racks = 1
        drop_racks = random.sample(list(self.racks), ndrop_racks)
        drop_hosts = [
            host
            for host in self.host_rack_map.keys()
            if self.host_rack_map[host] in drop_racks
        ]
        print("Drop racks", drop_racks)
        print("Drop hosts", drop_hosts)
        switches = [
            v for v in self.G.nodes() if v < HOST_OFFSET and v not in self.racks
        ]
        # print("switches", switches)
        random.shuffle(switches)
        failed_switches = switches[:nfailures]
        for failed_sw in failed_switches:
            for drop_host in drop_hosts:
                dst_rack = self.host_rack_map[drop_host]
                assert dst_rack in drop_racks
                failed_component = (failed_sw, drop_host)
                self.failed_components.append(failed_component)
                fail_prob[failed_component] = 1.0
        return fail_prob

    def GetDropProbSilentDrop(self):
        fail_prob = dict()
        edges = [(u, v) for (u, v) in self.G.edges]
        edges += [(v, u) for (u, v) in edges]
        for edge in edges:
            if edge in self.failed_components:
                fail_prob[edge] = random.uniform(0.002, 0.020)
                print(
                    "Failing_link", edge[0], edge[1], fail_prob[edge], file=self.outfile
                )
                # fail_prob[edge] = 0.004
            else:
                fail_prob[edge] = random.uniform(0.0000, 0.0001)
        return fail_prob

    def PrintPaths(self, all_sw_pair_paths):
        switches = [node for node in self.G.nodes() if node < HOST_OFFSET]
        for src_sw in switches:
            if src_sw % 10 == 0:
                print("Printing paths", src_sw)
            src_paths = all_sw_pair_paths[src_sw]
            for dst_sw in switches:
                # print(src_rack, dst_rack, len(src_paths[dst_rack]), file=self.outfile)
                for path in src_paths[dst_sw]:
                    if path != []:
                        self.PrintPath("FP", path, out=self.outfile)

    def GetAllSwitchPairPaths2(self):
        all_sw_pair_paths = dict()
        switches = [node for node in self.G.nodes() if node < HOST_OFFSET]
        all_pair_dists = [
            None for i in range(self.max_switch + 1)
        ]  # dict(nx.all_pairs_shortest_path_length(self.G))
        print(self.max_switch, "max_switch")
        for src_sw in switches:
            if src_sw % 50 == 0:
                print("getting path lens", src_sw)
            src_dists = dict(nx.single_source_shortest_path_length(self.G, src_sw))
            # print("src_rack", src_rack)
            all_pair_dists[src_sw] = src_dists

        def GetPaths(src, dst):
            if src == dst:
                return [[dst]]
            else:
                ret = []
                curr_dist = all_pair_dists[src][dst]
                for nbr in self.G.neighbors(src):
                    if nbr < HOST_OFFSET and all_pair_dists[nbr][dst] + 1 == curr_dist:
                        nbr_paths = GetPaths(nbr, dst)
                        ret += [[src] + path for path in nbr_paths]
                return ret

        for src_sw in switches:
            if src_sw % 10 == 0:
                print("Getting Paths", src_sw)
            src_paths = dict()
            for dst_sw in switches:
                if src_sw == dst_sw:
                    src_paths[dst_sw] = [[]]
                else:
                    src_paths[dst_sw] = GetPaths(src_sw, dst_sw)
                # print(src_rack, dst_rack, len(src_paths[dst_rack]), file=self.outfile)
            all_sw_pair_paths[src_sw] = src_paths
        return all_sw_pair_paths

    def GetAllRackPairPaths(self):
        all_rack_pair_paths = dict()
        for src_rack in self.racks:
            if src_rack % 10 == 0:
                print("printing paths", src_rack)
            src_paths = dict()
            for dst_rack in self.racks:
                # TODO: parallelize, check if nx.all_shortest_paths are thread safe
                paths = list(
                    nx.all_shortest_paths(self.G, source=src_rack, target=dst_rack)
                )
                src_paths[dst_rack] = paths
                for path in paths:
                    self.PrintPath("FP", path, out=self.outfile)
            all_rack_pair_paths[src_rack] = src_paths
        return all_rack_pair_paths

    def PrintLogsBlackHole(self, nfailed_pairs, failfile):
        nflows = 100 * self.nservers
        print("Nflows", nflows, "nservers", self.nservers)
        flows = self.GetFlowsBlackHole(nflows)
        all_rack_pair_paths = self.GetAllSwitchPairPaths2()
        self.PrintPaths(all_rack_pair_paths)
        fail_prob = self.GetDropProbBlackHole(flows, nfailed_pairs, all_rack_pair_paths)
        for device, src, dst in fail_prob:
            print(
                "Failing_component",
                src,
                dst,
                device,
                fail_prob[(device, src, dst)],
                file=failfile,
            )
        curr = 0
        print("Fail prob", fail_prob)

        def GetFailProb(device, flow):
            if (device, flow.src, flow.dst) in fail_prob:
                return fail_prob[(device, flow.src, flow.dst)]
            else:
                return 0.0001

        sumflowsize = 0
        for flow in flows:
            src_rack = self.host_rack_map[flow.src]
            dst_rack = self.host_rack_map[flow.dst]
            # print(src, dst, flowsize)
            # if (flow.src, flow.dst) in self.failed_src_dst_pairs:
            #    print("Failed pair", flow.src, flow.dst)
            path_taken = flow.HashToPath(all_rack_pair_paths[src_rack][dst_rack])
            # print(path_taken, src_rack, dst_rack)
            # first_link = (flow.src, src_rack)
            # last_link = (dst_rack, flow.dst)
            flow_dropped = False
            # if (random.random() < GetFailProb(first_link, flow) or random.random() <= GetFailProb(last_link, flow)):
            #    flow_dropped = True
            for k in range(0, len(path_taken) - 1):
                if random.random() <= GetFailProb(path_taken[k], flow):
                    flow_dropped = True
            if flow_dropped:
                print("Dropping flow", flow.src, flow.dst)
            print(
                "FID ",
                flow.src,
                flow.dst,
                src_rack,
                dst_rack,
                flow.flowsize,
                0.0,
                file=self.outfile,
            )
            self.PrintPath("FPT", path_taken, out=self.outfile)
            print(
                "SS ",
                1000.0 * random.random(),
                1,
                int(flow_dropped),
                0,
                file=self.outfile,
            )
            sumflowsize += flow.flowsize
        return nflows, sumflowsize

    def GetCompleteFlowPath(self, flow, all_sw_pair_paths):
        src_sw = flow.src
        dst_sw = flow.dst
        if flow.src >= HOST_OFFSET:
            src_sw = self.host_rack_map[flow.src]
        if flow.dst >= HOST_OFFSET:
            dst_sw = self.host_rack_map[flow.dst]
        # print (flow.src, flow.dst, src_sw, dst_sw)
        if len(all_sw_pair_paths[src_sw][dst_sw]) == 0:
            print(src_sw, dst_sw)
        path_taken = flow.HashToPath(all_sw_pair_paths[src_sw][dst_sw])
        if src_sw == dst_sw:
            path_taken = [src_sw]
        complete_path = []
        if flow.src != src_sw:
            complete_path.append(flow.src)
        complete_path += path_taken
        if flow.dst != dst_sw:
            complete_path.append(flow.dst)
        return complete_path

    def PrintLogsMisconfiguredACL(self, nfailures, failfile, flowsfile):
        # nflows = 200 * self.nservers
        # flows = self.GetFlowsUniform(nflows)
        flows = self.ReadFlowsFromFile(flowsfile)
        nflows = len(flows)
        print("Nflows", nflows, "nservers", self.nservers)
        fail_prob = self.GetDropProbMisconfiguredACL(nfailures)
        for device, dst in fail_prob:
            print(
                "Failing_component",
                device,
                dst,
                fail_prob[(device, dst)],
                file=failfile,
            )
        curr = 0
        print("Fail prob", fail_prob)

        def GetFailProb(device, flow):
            if (device, flow.dst) in fail_prob:
                return fail_prob[(device, flow.dst)]
            else:
                return 0.0001

        sumflowsize = 0
        all_sw_pair_paths = self.GetAllSwitchPairPaths2()
        failed_switches = list(set([failed_cmp[0] for failed_cmp in fail_prob.keys()]))
        drop_hosts = [failed_cmp[1] for failed_cmp in fail_prob.keys()]

        drop_racks = list(set([self.host_rack_map[host] for host in drop_hosts]))
        print("drop_racks", drop_racks, failed_switches)
        for fsw in failed_switches:
            first_hops = set.union(
                *[
                    set([path[1] for path in all_sw_pair_paths[fsw][drop_rack]])
                    for drop_rack in drop_racks
                ]
            )
            print(
                "Failed switch", fsw, "Drop racks", drop_racks, "First_hops", first_hops
            )
            npaths = sum(
                [
                    len(set([path[1] for path in all_sw_pair_paths[fsw][drop_rack]]))
                    for drop_rack in drop_racks
                ]
            )
            assert npaths > 0
            if npaths == 1:
                for drop_rack in drop_racks:
                    for path in all_sw_pair_paths[fsw][drop_rack]:
                        print("Failing_link", fsw, path[1], 0, file=self.outfile)
            else:
                print("Failing_link", fsw, fsw, 0, file=self.outfile)

        self.PrintPaths(all_sw_pair_paths)

        for flow in flows:
            complete_path = self.GetCompleteFlowPath(flow, all_sw_pair_paths)
            flow_dropped = False
            for k in range(0, len(complete_path)):
                if random.random() <= GetFailProb(complete_path[k], flow):
                    flow_dropped = True
            # if flow.dst in drop_hosts:
            # print("Flow", flow.src, flow.dst, complete_path)
            # rint(complete_path, flow.src, flow.dst, src_sw, dst_sw, path_taken)
            assert len(complete_path) > 2
            print(
                "FID ",
                flow.src,
                flow.dst,
                complete_path[1],
                complete_path[-2],
                flow.flowsize,
                0.0,
                file=self.outfile,
            )
            self.PrintPath("FPT", complete_path[1:-1], out=self.outfile)
            print(
                "SS ",
                1000.0 * random.random(),
                1,
                int(flow_dropped),
                0,
                file=self.outfile,
            )
            sumflowsize += flow.flowsize
        return nflows, sumflowsize

    def PrintLogsSilentDrop(self, nfailed_links, flows_file="NA"):
        self.SetFailedLinksSilentDrop(nfailed_links)
        fail_prob = self.GetDropProbSilentDrop()
        nflows = 100 * self.nservers
        flows = []
        if flows_file == "NA":
            # flows = self.GetFlowsSkewed(nflows)
            flows = self.GetFlowsUniform(nflows)
        else:
            flows = self.ReadFlowsFromFile(flows_file)
            nflows = len(flows)
        all_sw_pair_paths = self.GetAllSwitchPairPaths2()
        self.PrintPaths(all_sw_pair_paths)
        packetsize = 1500  # bytes
        sumflowsize = 0
        curr = 0
        for flow in flows:
            curr += 1
            if curr % 50000 == 0:
                print("Finished", curr, "flows")
            complete_path = self.GetCompleteFlowPath(flow, all_sw_pair_paths)
            assert flow.flowsize > 0 or flow.IsFlowActive()
            # print(src, dst, flowsize)
            packets_sent = math.ceil(flow.flowsize / packetsize)
            if flow.IsFlowActive():
                packets_sent = 40
            # path_taken = random.choice(list(nx.all_shortest_paths(G, source=src_rack, target=dst_rack)))
            packets_dropped = 0
            for i in range(packets_sent):
                for k in range(0, len(complete_path) - 1):
                    u = complete_path[k]
                    v = complete_path[k + 1]
                    if random.random() <= fail_prob[(u, v)]:
                        packets_dropped += 1
                        break
            print(
                "FID ",
                flow.src,
                flow.dst,
                complete_path[1],
                complete_path[-2],
                packetsize * packets_sent,
                0.0,
                file=self.outfile,
            )
            self.PrintPath("FPT", complete_path[1:-1], out=self.outfile)
            print(
                "SS ",
                1000.0 * random.random(),
                packets_sent,
                packets_dropped,
                0,
                file=self.outfile,
            )
            sumflowsize += flow.flowsize
        return nflows, sumflowsize
