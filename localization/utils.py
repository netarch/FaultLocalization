import sys
from heapq import heappush, heappop
from multiprocessing import Process, Queue
import pyximport; pyximport.install()
import math
import time
import random
import numpy as np
from scipy.optimize import minimize
import flow as FlowStruct
from flow import *

CONSIDER_REVERSE_PATH = False
PATH_KNOWN = FlowStruct.PATH_KNOWN
VERBOSE = False

link_speed_pps = 833333
packet_size_bytes = 1500

class LinkStats:
    def __init__(self, src, dest, drop_rate, avg_qlen, max_qlen):
        self.src = src
        self.dest = dest
        self.sum_drop_rates = drop_rate
        self.sum_qlen = avg_qlen
        self.max_qlen_reading = avg_qlen
        self.max_qlen = max_qlen
        self.num_readings = 1

    def append_reading(self, linkstats):
        assert (self.src == linkstats.src)  
        assert (self.dest == linkstats.dest)    
        assert (self.max_qlen == linkstats.max_qlen)    
        self.num_readings += linkstats.num_readings
        self.sum_drop_rates += linkstats.sum_drop_rates
        self.sum_qlen += linkstats.sum_qlen
        self.max_qlen_reading = max(self.max_qlen_reading, linkstats.max_qlen_reading)
        
    def drop_rate(self):
        return self.sum_drop_rates/self.num_readings

    def avg_qlen_ms(self):
        return self.sum_qlen * 1000.0/(packet_size_bytes * self.num_readings * link_speed_pps)

    def max_avg_qlen_ms(self):
        return self.max_qlen_reading * 1000.0/(packet_size_bytes * link_speed_pps)

def get_data_from_logfile(filename):
    get_data_start_time = time.time()
    failed_links = dict()
    flows = []
    links = dict()
    inverse_links = []
    link_statistics = dict()
    ctr = 0
    num_flows_with_retransmission = 0
    with open(filename, encoding = "ISO-8859-1") as f:
    #with open(filename) as f:
        flow = None
        for line in f.readlines():
            tokens = line.split()
            #if "threshold_latency" in line:
            #    threshold_latency = float(tokens[1])
            if "Failing_link" in line:
                src = int(tokens[1])
                dst = int(tokens[2])
                failparam = float(tokens[3])
                failed_links[(src, dst)] = failparam
                if VERBOSE:
                    print("Failed link ", src, dst, failparam)
            elif "Flowid=" in line:
                #Flowid= 3 2 10000000 1078.309922 1086.700814 10096 0 0
                if flow != None and len(flow.paths)>0: #log the previous flow
                    assert (flow.path_taken != None)
                    assert (flow.reverse_path_taken != None)
                    if flow.get_latest_packets_sent() > 0:
                        #print(flow.starttime_ms)
                        if flow.get_latest_packets_lost() > 0:
                            num_flows_with_retransmission += 1
                        if not flow.discard_flow():
                            flows.append(flow)
                    #flow.print_flow_metric(sys.stdout)
                #print(flow.starttime_ms)
                #Flowid= 3 157 31302 1073.072934 21 0 0
                src = int(tokens[1])
                dest = int(tokens[2])
                nbytes = int(tokens[3])
                start_time_ms = float(tokens[4])
                flow = Flow(src, 0, 0, dest, 0, 0, nbytes, start_time_ms)
            elif "Snapshot" in line:
                snapshot_time_ms = float(tokens[1])
                num_sent = int(tokens[2])
                num_lost = int(tokens[3])
                num_randomly_lost = int(tokens[4])
                flow.add_snapshot(snapshot_time_ms, num_sent, num_lost, num_randomly_lost)
            elif "flowpath_reverse" in line:
                #flowpath_reverse_taken 14 6 10
                path = []
                for i in range(1, len(tokens)):
                    path.append(int(tokens[i]))
                if(len(path) > 0):
                    flow.add_reverse_path(path, reverse_path_taken=("_taken" in line))
                for v in range(1, len(path)):
                    link = (path[v-1], path[v])
                    if link not in links:
                        links[link] = ctr
                        #print("Link:",link," ctr:",ctr)
                        assert(len(inverse_links) == ctr)
                        inverse_links.append(link)
                        ctr += 1

            elif "flowpath" in line:
                #flowpath 10 2 14
                #print(filename, line)
                path = []
                for i in range(1, len(tokens)):
                    path.append(int(tokens[i]))
                if(len(path) > 0):
                    flow.add_path(path, path_taken=("_taken" in line))
                for v in range(1, len(path)):
                    link = (path[v-1], path[v])
                    if link not in links:
                        links[link] = ctr
                        #print("Link:",link," ctr:",ctr)
                        assert(len(inverse_links) == ctr)
                        inverse_links.append(link)
                        ctr += 1

            elif "link_statistics" in line:
                #link_statistics 0 8 avg_drop_rate 0.028156 avg_queue_size 75986 max_queue_size 153000
                s = int(tokens[1])
                t = int(tokens[2])
                lossrate = 0
                if (tokens[3] == "avg_drop_rate"):
                    lossrate = float(tokens[4])
                else:
                    lossrate = float(tokens[3])
                avg_qlen = 0
                if(len(tokens)>6):
                    avg_qlen = float(tokens[6])
                max_qlen = 0
                if(len(tokens)>8):
                    max_qlen = float(tokens[8])
                ls = LinkStats(s, t, lossrate, avg_qlen, max_qlen)
                if (s, t) not in link_statistics:
                    link_statistics[(s, t)] = LinkStats(s, t, lossrate, avg_qlen, max_qlen) 
                else:
                    link_statistics[(s, t)].append_reading(ls)
            elif "# numrecords=" in line:
                # !TODO: Hack to ignore irrelevant things in the logfile
                # This line appears after all the relevant lines
                break
            else:
                pass
                #print("Unrecognized log statement: ", line)
        if flow != None and len(flow.paths)>0: #log the previous flow
            assert (flow.path_taken != None)
            assert (flow.reverse_path_taken != None)
            if flow.get_latest_packets_sent() > 0:
                if flow.get_latest_packets_lost() > 0:
                    num_flows_with_retransmission += 1
                #print(flow.starttime_ms)
                if not flow.discard_flow():
                    flows.append(flow)
        #print("Num flows with retransmission", num_flows_with_retransmission)

    if VERBOSE:
        print("Read log file in ", time.time() - get_data_start_time, "seconds")
    return {'failed_links':failed_links, 'flows':flows, 'links':links, 'inverse_links':inverse_links, 'link_statistics':link_statistics}


def get_forward_flows_by_link(flows, inverse_links, max_finish_time_ms):
    flows_by_link = dict()
    for link in inverse_links:
        flows_by_link[link] = []
    for ff in range(len(flows)):
        flow = flows[ff]
        flow_paths = flow.get_paths(max_finish_time_ms)
        for path in flow_paths:
            for v in range(1, len(path)):
                l = (path[v-1], path[v])
                flows_by_link[l].append(ff)
    return flows_by_link

def get_reverse_flows_by_link(flows, inverse_links, max_finish_time_ms):
    flows_by_link = dict()
    for link in inverse_links:
        flows_by_link[link] = []
    for ff in range(len(flows)):
        flow = flows[ff]
        flow_reverse_paths = flow.get_reverse_paths(max_finish_time_ms)
        for path in flow_reverse_paths:
            for v in range(1, len(path)):
                l = (path[v-1], path[v])
                flows_by_link[l].append(ff)
    return flows_by_link

def get_flows_by_link(forward_flows_by_link, reverse_flows_by_link, inverse_links):
    if not CONSIDER_REVERSE_PATH:
        return forward_flows_by_link
    flows_by_link = dict()
    for l in inverse_links:
        flows_by_link[l] = forward_flows_by_link[l] + reverse_flows_by_link[l]
    return flows_by_link

def get_data_structures_from_logdata(logdata, max_finish_time_ms):
    start_time = time.time()
    failed_links = logdata['failed_links']
    flows = logdata['flows']
    links = logdata['links']
    inverse_links = logdata['inverse_links']
    link_statistics = logdata['link_statistics']
    #print("num flows: ", len(flows))
    #print("num links: ", len(inverse_links))
    #print("num linkstats: ", len(link_statistics))
    start_time = time.time()
    forward_flows_by_link = get_forward_flows_by_link(flows, inverse_links, max_finish_time_ms)
    reverse_flows_by_link = dict()
    if CONSIDER_REVERSE_PATH:
        reverse_flows_by_link = get_reverse_flows_by_link(flows, inverse_links, max_finish_time_ms)
    flows_by_link = get_flows_by_link(forward_flows_by_link, reverse_flows_by_link, inverse_links)
    return flows, links, inverse_links, flows_by_link, forward_flows_by_link, reverse_flows_by_link, link_statistics, failed_links


def calc_alpha(score, expected_score):
    return calc_alpha1(score, expected_score)

def calc_alpha1(score, expected_score):
    return score/max(1.0e-6, expected_score)

def calc_alpha2(score, expected_score):
    if (expected_score <= 1.0e-5):
        return 1.0
    delta = 1.0 - (score/expected_score)
    if(delta <= 0.0): 
        return 1.0
    else:
        alpha = math.exp(-delta)/math.pow(1.0 - delta, 1.0-delta)
        alpha = math.pow(alpha, expected_score)

def get_link_scores(flows, inverse_links, forward_flows_by_link, reverse_flows_by_link, min_start_time_ms, max_finish_time_ms, active_flows_only=False):
    scores = dict()
    expected_scores = dict()
    for l in inverse_links:
        score = 0.0
        expected_score = 0.0
        forward_flows = forward_flows_by_link[l]
        for ff in forward_flows:
            flow = flows[ff]
            if flow.start_time_ms < min_start_time_ms or (active_flows_only and not flow.is_active_flow()):
                continue
            weights = flow.label_weights_func(max_finish_time_ms)
            expected_score += (weights[0] + weights[1])/len(flow.get_paths(max_finish_time_ms))
            score += weights[1]

        if CONSIDER_REVERSE_PATH:
            reverse_flows = reverse_flows_by_link[l]
            for ff in reverse_flows:
                flow = flows[ff]
                if flow.start_time_ms < min_start_time_ms or (active_flows_only and not flow.is_active_flow()):
                    continue
                weights = flow.label_weights_func(max_finish_time_ms)
                expected_score += (weights[0] + weights[1])/len(flow.get_reverse_paths(max_finish_time_ms))
                score += weights[1]
        scores[l] = score
        expected_scores[l] = expected_score
    return (scores, expected_scores)

#p_r_1[param] += p_r_2[param] (Tuple addition)
def combine_p_r_dicts(p_r_1, p_r_2, parameter_space):
    for param in parameter_space:
        p1, r1 = p_r_1[param]
        p2, r2 = p_r_2[param]
        p_r_1[param] = (p1+p2, r1+r2)

def get_precision_recall(failed_links, predicted_hypothesis):
    predicted_hypothesis_set = set(predicted_hypothesis)
    failed_links_set = set(failed_links)
    num_correct = sum([1.0 for h in predicted_hypothesis_set if h in failed_links_set])
    precision = 1.0
    if len(predicted_hypothesis) > 0:
        precision = num_correct/len(predicted_hypothesis_set)
    recall = 1.0
    if len(failed_links_set) > 0:
        recall = num_correct/len(failed_links_set)
    return precision, recall
   

