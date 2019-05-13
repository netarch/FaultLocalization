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
import utils
from utils import *
from plot_utils import *
from common_testing_system import *
from common_bayesian_net import *


def get_precision_recall_bayesian_cilia(filename, min_start_time_ms, max_finish_time_ms, nprocesses):
    return get_precision_recall_estimator(filename, min_start_time_ms, max_finish_time_ms, compute_hypothesis_bayesian_network_cilia, [], nprocesses)
   


if __name__ == '__main__':
    #for link in failed_links:
    #    print("Failed link: ", link, " failure parameter: ", failed_links[link])
    filename = sys.argv[1]
    utils.THRESHOLD_ACTIVE_PROBES = 0.0
    nprocesses = 30
    utils.VERBOSE = True
    #get_precision_recall_trend(file_prefix, minfile, maxfile, [nfails], nprocesses)
    start_time = time.time()
    precision_recall, info = get_precision_recall_bayesian_cilia(filename, 0.0, 100000000.0, nprocesses)
    #precision_recall = get_precision_recall_bayesian(file_prefix, nprocesses)
    print(filename, "precision recall", precision_recall, "execution time", time.time() - start_time, "seconds")
    #plot_latency_cdf(file_prefix, "latency_" + outfile, nprocesses)
    #plot_latency_cdf_file(file_prefix, minfile, maxfile, "latency_" + outfile)

