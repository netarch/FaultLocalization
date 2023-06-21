import numpy as np
import sys
import operator
import random
import matplotlib.pyplot as plt
from scipy.spatial import ConvexHull
import matplotlib.font_manager as fm
 
gs_font = fm.FontProperties(fname='../gillsans.ttf', size=30, weight='bold')
gs_font_legend = fm.FontProperties(fname='../gillsans.ttf', size=30, weight='bold')

#pcolors = ['#0e326d', '#a5669f', '#028413', '#db850d','#000000',  '#af0505', '#330066', '#cc00cc', '#cc6600', '#999900']
#pcolors = ['#0e326d', '#000000', '#028413', '#a5669f', '#af0505', '#db850d', '#330066', '#cc6600']// uncomment for 007
pcolors = ['#0e326d', '#000000', '#028413', '#a5669f', '#db850d', '#af0505', '#330066', '#cc6600']
#pcolors = ['#0e326d', '#a5669f', '#028413', '#db850d','#000000',  '#af0505', '#330066', '#cc00cc', '#cc6600', '#999900']
#pcolors = ['#0e326d', '#028413', '#00112d']
markers = ['o', 's', '^', 'v', 'p', '*', 'p', 'h']

light_grey=(0.5,0.5,0.5)
list.reverse(pcolors)

def string(x):
    if x >= (1024 * 1024 * 1024):
        return str(x/(1024*1024*1024)) + 'B'    
    elif x >= (1024 * 1024):
        return str(x/(1024*1024)) + 'M'
    elif x >= 1024:
        return str(x/1024) + 'K'
    else:
        return str(x)

fig, ax = plt.subplots()
xmin = 0.54
ymin = 0.32

def Fscore(point):
    p, r = point
    rcprcal = (1.0/p) + (1.0/r)
    return 2.0/rcprcal

def string_to_numeric_array(s):
    return [float(x) for x in s.split()]

def GetParamsFromFile(pfile):
    def IsNumber(s):
        try:
            float(s)
            return True
        except ValueError:
            return False
    params_acc = dict()
    with open(pfile) as f:
        for line in f.readlines():
            #"Epoch: ", i+minfile, p, r
            #max_traceroutes_per_switch: 2 nfails: 0 latency_threshold: 2.0
            line = line.replace('(', ' ')
            line = line.replace('[', ' ')
            line = line.replace(')', ' ')
            line = line.replace(']', ' ')
            line = line.replace(',', ' ')
            if line.isspace():
                continue
            tokens = line.split()
            if len(tokens)>2 and all([IsNumber(token) for token in tokens]):
                params = tuple([float(x) for x in tokens[:len(tokens)-2]])
                precision = float(tokens[-2])
                recall = float(tokens[-1])
                params_acc[params] = (precision, recall)
    return params_acc

datadir = sys.argv[1]
outfile = sys.argv[2]
labels = []
schemes = ["bnet_int", "bnet_a1_a2_p", "bnet_a2", "bnet_a1_p", "nb_int", "bnet_a1", "nb_a1", "007_a2"]
sim_params = []
for scheme in schemes:
    params = GetParamsFromFile(datadir + "/t1_test_" + scheme)
    assert (len(params) == 1)
    param = list(params.keys())[0]
    pr = params[param]
    sim_params.append([scheme, param, pr])

'''
sim_params = [["bnet_int", [0.003, 0.0002, -15.0] , (1.000, 0.974)],
              ["bnet_a1_a2_p", [0.002, 0.0003, -15.0] , (1.000, 0.987)],
              ["bnet_a2", [0.007, 0.0003, -35.0] , (1.000, 0.951)],
              ["bnet_a1_p", [0.006, 0.0003, -5.0] , (0.982, 0.816)],
              ["nb_int", [0.011, 0.002, 0.075] , (0.997, 0.791)],
              ["bnet_a1", [0.0005, 0.000175, -4.0] , (1.000, 0.903)],
              ["nb_a1", [0.001, 0.007, 0.125] , (0.570, 0.350)],
              ["007_a2", [0.0145] , (0.613, 1.000)]
             ]
'''

label_reference = [("nb_a1", "NetBouncer (A1)"), ("net_bouncer_a1", "NetBouncer (A1)"), ("bnet_a1_a2_p", "Flock (A1+A2+P)")]
label_reference += [("bnet_a1_p", "Flock (A1+P)"), ("bnet_apk", "Flock (INT)"), ("nb_apk", "NetBouncer (INT)"), ("nb_int", "NetBouncer (INT)"), ("bnet_int", "Flock (INT)")]
label_reference += [("bnet_a2_p", "Flock (A2+P)"), ("bnet_a1", "Flock (A1)"), ("bnet_a2", "Flock (A2)"), ("007_a2", "007 (A2)")]

ctr = 0
for scheme, params, pr in sim_params:
    print (scheme, pr, Fscore(pr), 1-Fscore(pr))
    label = dict(label_reference)[scheme]
    linestyle = '-'
    markerfacecolor = pcolors[ctr]
    if "007" in label:
        markerfacecolor = 'none'
    elif "Flock (A2)" in label:
        markerfacecolor = 'none'
    ax.scatter([pr[0]], [pr[1]], s=300, facecolor=markerfacecolor, color=pcolors[ctr], alpha=1.0, marker=markers[ctr], linewidth=2.5, label=label, zorder = 10)
    ctr += 1
    

xlabel = "Precision"
ylabel = "Recall"

label_fontsize=30
ax.set_xlabel(xlabel, fontproperties=gs_font, fontsize=label_fontsize)
ax.set_ylabel(ylabel, fontproperties=gs_font, fontsize=label_fontsize)
for label in ax.get_xticklabels() :
    label.set_fontproperties(gs_font)
    label.set_color(light_grey)
for label in ax.get_yticklabels() :
    label.set_fontproperties(gs_font)
    label.set_color(light_grey)
for axis in ['top', 'bottom', 'left', 'right']:
    ax.spines[axis].set_color(light_grey)
ax.spines['top'].set_linestyle(':')
ax.spines['right'].set_linestyle(':')
plt.xlim(xmin=xmin, xmax=1.018)
plt.ylim(ymin=ymin, ymax=1.03)
ax.grid(linestyle=':', linewidth=1, color='grey')
ticklabelcolor = light_grey
ax.xaxis.set_tick_params(width=2, length=15)
ax.yaxis.set_tick_params(width=2, length=10)
ax.tick_params(axis='x', colors=ticklabelcolor, direction="in")
ax.tick_params(axis='y', colors=ticklabelcolor, direction="in")
#xticks = np.array([300, 1200, 4800, 19200, 76800])
#ax.xaxis.set_ticks(xticks)
#xticks = np.array([string(x) for x in xticks])
#ax.set_xticklabels(xticks, color=ticklabelcolor)A
handles, labels = plt.gca().get_legend_handles_labels()
#order = [2,1,0]
print ("handles", handles, "labels", labels)
#leg = ax.legend(handles,labels, bbox_to_anchor=(0.25, 0.87), borderaxespad=0, loc=2, numpoints=1, handlelength=1, markerscale=1., prop=gs_font_legend)
#leg = ax.legend([handles[idx] for idx in order],[labels[idx] for idx in order], bbox_to_anchor=(0.001, 0.48), borderaxespad=0, loc=2, numpoints=2, handlelength=2, scatterpoints=1, fontsize=label_fontsize)
#leg.get_frame().set_linewidth(0.0)
ax.set_xlabel(xlabel)
ax.set_ylabel(ylabel)
plt.tick_params(labelsize=25)
plt.tight_layout()
plt.savefig(outfile)
