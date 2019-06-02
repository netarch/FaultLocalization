import sys
from heapq import heappush, heappop
import math
import time
import random

PATH_KNOWN = True

class Flow:
    # Initialize flow without snapshot
    def __init__(self, src, srcip, srcport, dest, destip, destport, nbytes, start_time_ms):
        self.src = src
        self.srcip = srcip
        self.srcport = srcport
        self.dest = dest
        self.destip = destip
        self.destport = destport
        self.nbytes = nbytes
        self.start_time_ms = start_time_ms
        self.snapshots = []
        self.snapshot_ptr = -1
        self.paths = []
        self.reverse_paths = []
        self.path_taken = None
        self.reverse_path_taken = None

    def add_path(self, path, path_taken=False):
        self.paths.append(path)
        if path_taken:
            self.set_path_taken(path)

    def add_reverse_path(self, path, reverse_path_taken=False):
        self.reverse_paths.append(path)
        if reverse_path_taken:
            self.set_reverse_path_taken(path)

    def set_path_taken(self, p):
        self.path_taken = p;

    def set_reverse_path_taken(self, p):
        self.reverse_path_taken = p

    def get_latest_packets_sent(self):
        if len(self.snapshots) == 0:
            return 0
        else:
            return self.snapshots[-1][1]

    def get_latest_packets_lost(self):
        if len(self.snapshots) == 0:
            return 0
        else:
            return self.snapshots[-1][2]

    def any_snapshot_before(self, finish_time_ms):
        return (len(self.snapshots) > 0 and self.snapshots[0][0] <= finish_time_ms)

    def add_snapshot(self, snapshot_time_ms, packets_sent, lost_packets, randomly_lost_packets):
        if (len(self.snapshots) > 0):
            assert(snapshot_time_ms > self.snapshots[-1][0])
        snapshot = [snapshot_time_ms, packets_sent, lost_packets, randomly_lost_packets]
        self.snapshots.append(snapshot)

    def print_flow_snapshots(self, outfile):
        for snapshot in self.snapshots:
            snapshot_time_ms = snapshot[0]
            packets_sent = snapshot[1]
            lost_packets = snapshot[2]
            randomly_lost_packets = snapshot[3]
            print("Snapshot ", snapshot_time_ms, packets_sent, lost_packets, randomly_lost_packets, file=outfile)

    def print_flow_metrics(self, outfile=sys.stdout):
        print("Flowid=", self.src, self.dest, self.nbytes, self.start_time_ms, file=outfile)

    def printinfo(self, outfile=sys.stdout):
        self.print_flow_metrics(outfile)
        self.print_flow_snapshots(outfile)
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
            

    def get_paths(self, max_finish_time_ms):
        if PATH_KNOWN or self.traceroute_flow(max_finish_time_ms):
            return [self.path_taken]
        else:
            return self.paths
    
    def get_reverse_paths(self, max_finish_time_ms):
        if PATH_KNOWN or self.traceroute_flow(max_finish_time_ms):
            return [self.reverse_path_taken]
        else:
            return self.reverse_paths

    def flow_finished(self):
        return (self.finish_time_ms > 0)

    #flow_label_weights_func: a function which assigns two weights to each flow : (good_weight, bad_weight)
    def update_snapshot_ptr(self, max_finish_time_ms):
        ptr = self.snapshot_ptr
        assert(ptr==-1 or self.snapshots[ptr][0] < max_finish_time_ms)
        while(ptr+1 < len(self.snapshots) and self.snapshots[ptr+1][0] < max_finish_time_ms):
            ptr += 1
        self.snapshot_ptr = ptr

    def get_drop_rate(self, max_finish_time_ms):
        self.update_snapshot_ptr(max_finish_time_ms)
        if self.snapshot_ptr >= 0:
            snapshot = self.snapshots[self.snapshot_ptr]
            return float(snapshot[2])/snapshot[1]
        return 0

    def get_packets_lost_before_finish_time(self, max_finish_time_ms):
        self.update_snapshot_ptr(max_finish_time_ms)
        if self.snapshot_ptr >= 0:
            snapshot = self.snapshots[self.snapshot_ptr]
            return snapshot[2]
        return 0
            
    def traceroute_flow(self, max_finish_time_ms):
        return (PATH_KNOWN or self.get_packets_lost_before_finish_time(max_finish_time_ms) > 0)

    def label_weights_func(self, max_finish_time_ms):
        self.update_snapshot_ptr(max_finish_time_ms)
        gw = 0
        bw = 0
        if self.snapshot_ptr >= 0:
            snapshot = self.snapshots[self.snapshot_ptr]
            gw = snapshot[1] - snapshot[2]
            bw = snapshot[2]
        return (gw, bw)

