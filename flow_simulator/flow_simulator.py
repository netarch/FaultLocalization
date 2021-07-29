import sys
import multiprocessing
from multiprocessing import Process, Queue
import copy
import time
import random
import math
import numpy as np
import networkx as nx
from utils import *

nargs = 3
if not len(sys.argv) == nargs+1: 
	print("Required number of arguments:", nargs, ".", len(sys.argv)-1, "provided")
	print("Arguments: <network_file> <nfailures> <outfile>")
	sys.exit()

HOST_OFFSET = 10000

print("Random witness", random.randint(1, 100000))

network_file = sys.argv[1]
nfailed_links = int(sys.argv[2])
outfile = sys.argv[3]

topo = Topology()
topo.ReadGraphFromFile(network_file)
outfile = open(outfile,"w+")
topo.SetOutFile(outfile)
nflows, sumflowsize = topo.PrintLogsBlackHole(nfailed_links)
#nflows, sumflowsize = topo.PrintLogsSilentDrop(nfailed_links)

print("Sum flow size: ", sumflowsize, "Numflows", nflows)
