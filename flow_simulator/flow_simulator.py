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

nargs = 4
if not len(sys.argv) == nargs + 1:
    print("Required number of arguments:", nargs, ".", len(sys.argv) - 1, "provided")
    print("Arguments: <network_file> <nfailures> <flowsfile> <outfile> ")
    sys.exit()

HOST_OFFSET = 10000

print("Random witness", random.randint(1, 100000))

# tuple1 = (1573, 1031, 10390, 10650)
# tuple2 = (1573, 1029, 10390, 10650)
# print(TupleHash(tuple1), TupleHash(tuple1)%25, TupleHash(tuple2), TupleHash(tuple2)%25)

network_file = sys.argv[1]
nfailures = int(sys.argv[2])
flows_file = sys.argv[3]
outfile = sys.argv[4]
fail_file = outfile + ".fails"

topo = Topology()
topo.ReadGraphFromFile(network_file)
outfile = open(outfile, "w+")
topo.SetOutFile(outfile)
fail_file = open(fail_file, "w+")
nflows, sumflowsize = topo.PrintLogsBlackHole(nfailures, fail_file)
# nflows, sumflowsize = topo.PrintLogsMisconfiguredACL(nfailures, fail_file, flows_file)
# nflows, sumflowsize = topo.PrintLogsSilentDrop(nfailures, flows_file)

print("Sum flow size: ", sumflowsize, "Numflows", nflows)
