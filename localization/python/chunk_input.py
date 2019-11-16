import sys
import time
import random

filename = sys.argv[1]
nchunks = int(sys.argv[2])
prefix_dir = sys.argv[3]

lines = []
with open(filename, encoding = "ISO-8859-1") as f:
    for line in f.readlines():
        lines.append(line)

def write_lines_to_file(lines, fname):
    with open(fname, 'w') as f:
        for line in lines:
            f.write(line)

start = 0
nlines = len(lines)
chunk_size = int(nlines/nchunks)
for i in range(nchunks):
    end = min(nlines - 1, start + chunk_size - 1)
    if i == nchunks - 1:
        end = nlines - 1
    while end < nlines - 1 and (not "Flowid" in lines[end]):
        end += 1
    fname = prefix_dir + "/" + str(i)
    print("writing", end-start, "lines to chunk", i, start, end)
    write_lines_to_file(lines[start:end], fname)
    start = end
