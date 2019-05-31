import sys
from heapq import heappush, heappop
from multiprocessing import Process, Queue
import math
import time
import joblib
from joblib import Parallel, delayed
import random
import numpy as np
from scipy.optimize import minimize
from utils import *
from plot_utils import *

file_prefix = sys.argv[1]
threshold_loss = float(sys.argv[2])
outfile = sys.argv[3]
minfile = int(sys.argv[4])
maxfile = int(sys.argv[5])

start_time = time.time()
logdata = get_data_from_logs(file_prefix, minfile, maxfile) 
failed_links = logdata['failed_links']
flows = logdata['flows']
links = logdata['links']
inverse_links = logdata['inverse_links']
link_statistics = logdata['link_statistics']
print("Read all log files in ", time.time() - start_time, "seconds")
print("num flows: ", len(flows))
print("num links: ", len(inverse_links))
print("num linkstats: ", len(link_statistics))

start_time = time.time()
forward_flows_by_link = get_forward_flows_by_link()
reverse_flows_by_link = []
if CONSIDER_REVERSE_PATH:
    reverse_flows_by_link = get_reverse_flows_by_link()
flows_by_link = get_flows_by_link(forward_flows_by_link, reverse_flows_by_link)
print("Time to partition flows by link: ", time.time() - start_time, "seconds")


def classify_flow_lossrate(flow, threshold_loss):
    return (flow.drop_rate > threshold_loss)

def compute_lossrate_scores():
   estimated_loss = estimate_lossrate_combine()
   actual_loss = [link_statistics[link].drop_rate()*100.0 for link in inverse_links]
   scores, expected_scores = get_link_scores(flows, inverse_links, forward_flows_by_link, reverse_flows_by_link, threshold_loss*0.75, classifier_func=classify_flow_lossrate, minfile_t=minfile, maxfile_t=maxfile)

   candidates = np.argsort(estimated_loss)
   for cd in candidates[-20:]:
      link = inverse_links[cd]
      print(link, estimated_loss[cd], actual_loss[cd], scores[link], expected_scores[link])

   for link in failed_links:
      ctr = links[link]
      print("Failed link: ", link, " estimated loss_rate: ", estimated_loss[ctr], " actual loss_rate: ", actual_loss[ctr], scores[link], expected_scores[link])

   alpha1 = [calc_alpha1(scores[link], expected_score[link]) for link in inverse_links]
   alpha2 = [calc_alpha2(scores[link], expected_score[link]) for link in inverse_links]

   colors = ['#af0505' for i in range(len(inverse_links))]
   for link in failed_links:
        ctr = links[link]
        colors[ctr] = '#028413'
   #TODO (Mingzhe): x: estimated loss rate and alpha1 and alpha2: alpha scores
   plot_scores(alpha1, alpha2, xlabel="alpha1", ylabel="alpha2", colors=colors)

def estimate_lossrate_combine():
   #construct random equations with large number of flows to minimize variance 
   prob = 0.05
   print("prob: ", prob)
   print("num equations: ", nequations)
   for flowid in range(len(flows)):
      flow = flows[flowid]
      if flowid%1000 == 0:
          print("Flow #", flowid)
      flow_paths = flow.get_paths()
      npaths = len(flow_paths)
      contr = 1.0/npaths
      tempcache = [] #to optimize performance
      for path in flow_paths:
         for v in range(1, len(path)):
            l = (path[v-1], path[v])
            s = links[l]
            #assert (inverse_links[s] == l)
            tempcache.append(s)
      equations = random.sample(range(nequations), int(prob*nequations))
      for i in equations:
          if random.uniform(0, 1) <= prob:
            # include this flow in the ith equation
            Y[i] += flow.drop_rate
            for s in tempcache:
               A[i][s] += contr
   print("A constructed")
   fun = lambda x: np.linalg.norm(np.dot(A,x)-Y)
   nlinks = len(inverse_links)
   sol = minimize(fun, np.zeros(nlinks), method='L-BFGS-B', bounds=[(0.,1.0) for x in xrange(nlinks)])
   X = sol['x']
   #temp = np.linalg.lstsq(A, Y, rcond=1.0e-7)
   #X = temp[0]
   #err = temp[1]
   ret = [100.0 * t for t in X]
   print(max(ret), np.mean(ret), len(ret))
   return ret

def calc_alpha(score, expected_score):
        return calc_alpha1(score, expected_score)

def calc_alpha1(score, expected_score):
        return score/max(0.00001, expected_score)

def calc_alpha2(score, expected_score):
        if (expected_score <= 1.0e-5):
                return 1.0
        delta = 1.0 - (score/expected_score)
        if(delta <= 0.0): 
                return 1.0
        else:
                alpha = math.exp(-delta)/math.pow(1.0 - delta, 1.0-delta)
                alpha = math.pow(alpha, expected_score)
                return min(1.0, alpha)

#By substituting loss(f) for E[loss(f)]
def estimate_lossrate_trivial():
   print("Running estimate_lossrate_trivial: ")
   A = np.zeros(shape=(len(flows), len(inverse_links)))
   Y = [flow.drop_rate for flow in flows]
   for ff in range(len(flows)):
        flow = flows[ff]
        if ff%200 == 0:
            print("Flow #", ff)
        flow_paths = flow.get_paths()
        npaths = len(flow_paths)
        contr = 1.0/npaths
        for path in flow_paths:
            for v in range(1, len(path)):
                l = (path[v-1], path[v])
                s = links[l]
                assert (inverse_links[s] == l)
                A[i][s] += 1.0/npaths
   print("A constructed")
   temp = np.linalg.lstsq(A, Y, rcond=1.0e-7)
   X = temp[0]
   err = temp[1]
   print(max(X), np.mean(X), len(X))
   candidates = np.argsort(X)
   for cd in candidates[-20:]:
      print(inverse_links[cd], X[cd])

   for link in failed_links:
      ctr = links[link]
      print("Failed link: ", link, " estimated loss_rate: ", X[ctr])


def plot_loss_rate_cdf(outfile): 
    fig, ax = plt.subplots()
    x = [flow.drop_rate * 100.0 for flow in flows]
    plot_cdf(x, "loss rate", outfile)

#for link in failed_links:
#    print("Failed link: ", link, " failure parameter: ", failed_links[link])
compute_lossrate_scores()
plot_loss_rate_cdf("lossrate_cdf_" + outfile)
