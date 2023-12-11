import numpy as np
import sys
import operator
import random
import matplotlib.pyplot as plt
from scipy.spatial import ConvexHull
 
import matplotlib
import matplotlib.font_manager as fm


datadir = sys.argv[1]
outfile = sys.argv[2]

PLOT_HYBRID_CAL = False

PLOT_ORIGINAL_PARAMS = False
# for wred
acc_original_params = {
                "bnet_int": (0.92, 0.994),
                "bnet_a2_p": (0.92, 0.994),
                "bnet_a2": (0.903, 0.994),
                "007_a2": (0.752, 1.0),
                "nb_int": (0.256, 0.333),
               }

matplotlib.rcParams['pdf.fonttype'] = 42
matplotlib.rcParams['ps.fonttype'] = 42
gs_font = fm.FontProperties(fname='../gillsans.ttf', size=25, weight='bold')
gs_font_leg = fm.FontProperties(fname='../gillsans.ttf', size=23, weight='bold')

#pcolors = ['#0e326d', '#a5669f', '#af0505', '#330066', '#cc6600']
pcolors = ['#028413', '#a5669f', '#af0505', '#330066', '#cc6600']
#pcolors = ['#0e326d', '#a5669f', '#028413', '#db850d','#000000',  '#af0505', '#330066', '#cc00cc', '#cc6600', '#999900']
#pcolors = ['#0e326d', '#028413', '#00112d']
markers = ['s', 'o', 'v', '^', 'p', '*', 'p', 'h']

list.reverse(pcolors)
light_grey=(0.5,0.5,0.5)

def Fscore(point):
    p, r = point
    rcprcal = (1.0/p) + (1.0/r)
    return 2.0/rcprcal

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


def GetHighestAccuracyPoint(params_acc, thresholds=None):
    points_all = [(params_acc[k][0], params_acc[k][1], k) for k in params_acc]
    #points List((p, r, params)
    print("Got", len(points_all), "parameters")
    def GetBestPoint(precision_threshold):
        high_precision_point = any([(p[0] > precision_threshold and p[1]>0.25) for p in points_all])
        while (not high_precision_point):
            precision_threshold -= 0.05
            high_precision_point = any([(p[0] > precision_threshold and p[1]>0.25) for p in points_all])
        points = []
        if high_precision_point:
            points = [p for p in points_all if p[0] > precision_threshold and p[1]>0.25]
            points = sorted(points,key=lambda x: (x[1], x[0]), reverse=True)
            #points = sorted(points,key=lambda x: (x[1]+x[0])/2.0, reverse=True)
        else:
            assert(False)
            #points = sorted(points_all,key=lambda x: (x[0] + x[1])/2.0, reverse=True)
            #points = sorted(points_all,key=lambda x: (x[0], x[1]), reverse=True)
        return points[0]
    '''
    def GetBestPoint(precision_threshold):
        high_precision_point = any([(p[0] > precision_threshold) for p in points_all])
        points = []
        if high_precision_point:
            points = [p for p in points_all if p[0] > precision_threshold]
            points = sorted(points,key=lambda x: (x[1], x[0]), reverse=True)
            #points = sorted(points,key=lambda x: (x[1]+x[0])/2.0, reverse=True)
        else:
            points = sorted(points_all,key=lambda x: (x[0] + x[1])/2.0, reverse=True)
            #points = sorted(points_all,key=lambda x: (x[0], x[1]), reverse=True)
        return points[0]
    '''
    ret = []
    prange = np.arange(0.6, 1.0, 0.025)
    if thresholds != None:
        prange = thresholds
    for p in prange: #[0.95]
        ret.append(GetBestPoint(p)) 
    return ret
    '''
    for p, r, params in points[:50]:
        params = [float(x) for x in params]
        print("Top params", params, "(%.3f, %.3f)"%(p, r), (p+r)/2)
    '''

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
bbox_x = 0.04
bbox_y = 0.9
xmin = 0.22 #0.18
ymin = 0.29 #0.18

points_arr = []
labels = []


def GetBestParams(file_prefix, thresholds=None):
    res_dict = dict()
    def PopulateForScheme(scheme, p_thresholds):
        params_acc = GetParamsFromFile(file_prefix + scheme)
        points_list = GetHighestAccuracyPoint(params_acc, p_thresholds)
        res_dict[scheme] = [k[2] for k in points_list]
    schemes = ["bnet_int", "bnet_a2_p", "bnet_a2", "nb_int", "007_a2"]
    for scheme in schemes:
        p_thresholds = thresholds
        if "bnet" in scheme and (thresholds != None and len(thresholds)==1):
            p_thresholds = [0.98]
        PopulateForScheme(scheme, p_thresholds)
    return res_dict

sim_params = []
hw_params = []
if PLOT_HYBRID_CAL:
    sim_params = GetBestParams(datadir + "/cal_", thresholds=[0.98])
    hw_params = GetBestParams(datadir + "/cal_hw_", thresholds=[0.98])

train_params = GetBestParams(datadir + "/train_")
test_params = GetBestParams(datadir + "/test_")

best_train_params = GetBestParams(datadir + "/train_", thresholds=[0.98])


print("train_params", train_params)
print("test_params", test_params)

'''
train_params = {"bnet_int": [0.003, 0.0001, -15],
              "bnet_a2_p": [0.003, 0.0001, -15],
              "bnet_a2": [0.009, 0.0004, -25],
              "nb_int": [0.56, 0.00025, 0.31],
              "007": [0.0594]} #wred

test_params = {"bnet_int": [0.002, 0.0005, -200.0],
              "bnet_a2_p": [0.002, 0.0005, -45.0],
              "bnet_a2":[0.002, 0.0005, -200.0],
              "nb_int": [0.51, 0.0205, 0.41],
              "007": [0.125]} #wred
'''
label_reference = [("nb_a1", "NetBouncer (A1)"), ("net_bouncer_a1", "NetBouncer (A1)"), ("bnet_a1_a2_p", "Flock (A1+A2+P)")]
label_reference += [("bnet_a1_p", "Flock (A1+P)"), ("bnet_apk", "Flock (INT)"), ("nb_apk", "NetBouncer (INT)"), ("nb_int", "NetBouncer (INT)"), ("bnet_int", "Flock (INT)")]
label_reference += [("bnet_a2_p", "Flock (A2+P)"), ("bnet_a1", "Flock (A1)"), ("bnet_a2", "Flock (A2)"), ("007", "007 (A2)")]


def GetPlotPoints(params_list, params_acc):
    plot_points = []
    for params in params_acc.keys():
        pr = params_acc[params]
        for env_params in params_list:
            param_eq = (len(params) == len(env_params))
            if param_eq:
                for i in range(len(params)):
                    if abs(params[i] - env_params[i]) > 1.0e-7:
                        param_eq = False
            if param_eq:
                #print(params, train_param, pr)
                plot_points.append(pr)
    return plot_points

def GetLabelScheme(infile):
    label = infile
    if '/' in label:
        label = str(infile[infile.rindex('/')+1:])
    scheme = None
    for k in params_keys:
        if k in label:
            scheme = k
            break
    for k,v in label_reference:
        if k in label:
            label = v
            break
    return label, scheme

params_keys = train_params.keys()
ctr=0
infiles = [datadir + "/test_" + scheme for scheme in params_keys]
errors = []
for infile in infiles:
    params_acc = dict()
    label, scheme = GetLabelScheme(infile)
    print("\n\n\nLabel", label, "Scheme", scheme) 
    train_param_list = []
    if scheme in train_params:
        train_param_list = train_params[scheme]
    best_train_params_list = []
    if scheme in best_train_params:
        best_train_params_list = best_train_params[scheme]
    #print(train_param_list)
    params_acc = GetParamsFromFile(infile)
    points = []
    best_acc = 0
    #print(params_acc)
    plot_points = GetPlotPoints(train_param_list, params_acc)
    best_point = GetPlotPoints(best_train_params_list, params_acc)[0]

    print("Best point", best_point, Fscore(best_point))
    errors.append(1.0 - Fscore(best_point))

    marker = markers[ctr] #'o'
    pcolor = pcolors[ctr]
    markerfacecolor = pcolor
    linestyle = ':'
    plot_points = sorted(plot_points,key=lambda x: (x[0], -x[1]), reverse=True)
    print("plotting", plot_points)
    p = [x[0] for x in plot_points]
    r = [x[1] for x in plot_points]
    #ax.scatter(p, r, s=200, facecolor=markerfacecolor, color=pcolors[ctr], alpha=1.0, marker=marker, linewidth=2.5, zorder=10, label = label)
    ax.plot(p, r, color=pcolor, lw=2, dash_capstyle='round', marker=marker, markersize=20, markeredgewidth=2.5, markerfacecolor='None', markeredgecolor=markerfacecolor, linestyle=linestyle)
    ctr += 1

print("************ HW Calibration *************")

if PLOT_HYBRID_CAL:
    ctr=0
    infiles = [datadir + "/cal_hw_" + scheme for scheme in params_keys]
    for infile in infiles:
        params_acc = dict()
        label, scheme = GetLabelScheme(infile)
        print("\n\n\nLabel", label, "Scheme", scheme) 
        sim_param_list = []
        if scheme in sim_params:
            sim_param_list = sim_params[scheme]
        hw_param_list = []
        if scheme in hw_params:
            hw_param_list = hw_params[scheme]
        print(sim_param_list, hw_param_list)
        params_acc = GetParamsFromFile(infile)
        points = []
        best_acc = 0
        #print(params_acc)
        plot_points = GetPlotPoints(sim_param_list, params_acc)
        hw_plot_points = GetPlotPoints(hw_param_list, params_acc)

        marker = markers[ctr] #'o'
        pcolor = pcolors[ctr]
        markerfacecolor = pcolor
        linestyle = ':'
        if PLOT_ORIGINAL_PARAMS:
            p, r = acc_original_params[scheme]
            plot_points = [(p, r)]
        p = [x[0] for x in plot_points]
        r = [x[1] for x in plot_points]
        print("plotting", plot_points)
        for x in plot_points:
            print(scheme, x, Fscore(x))
        ax.scatter(p, r, s=600, facecolor=pcolors[ctr], color=pcolors[ctr], alpha=1.0, marker=marker, linewidth=2.5, zorder=10, label=label)
        ctr += 1

xlabel = "Precision"
ylabel = "Recall"
#plot_scatter(points_arr, labels, xlabel, ylabel)

label_fontsize=25
ax.set_xlabel(xlabel, fontsize=label_fontsize)
ax.set_ylabel(ylabel, fontsize=label_fontsize)
plt.xlim(xmin=xmin, xmax=1.0)
plt.ylim(ymin=ymin, ymax=1.0)
ax.grid(linestyle=':', linewidth=1, color='grey')
ticklabelcolor = light_grey
#xticks = np.array([300, 1200, 4800, 19200, 76800])
#ax.xaxis.set_ticks(xticks)
#xticks = np.array([string(x) for x in xticks])
#ax.set_xticklabels(xticks, color=ticklabelcolor)A
handles, labels = plt.gca().get_legend_handles_labels()
#order = [2,1,0]
print ("handles", handles, "labels", labels)

ax.xaxis.set_tick_params(width=2, length=15)                                                             
ax.yaxis.set_tick_params(width=2, length=15)                                                             
ax.tick_params(axis='x', colors=ticklabelcolor, direction="in")                                          
ax.tick_params(axis='y', colors=ticklabelcolor, direction="in")      


#schemes = ["bnet_int", "bnet_a2_p", "bnet_a2", "nb_int", "007_a2"]
print("Error vs 007", errors[4]/errors[2], "vs nb_int", errors[3]/errors[0], "bnet_a2_p vs 007", errors[4]/errors[1], "bnet_a2_p vs bnet_a2", errors[2]/errors[1])

if "link_flap" not in outfile:
    leg = ax.legend(handles,labels, bbox_to_anchor=(bbox_x, bbox_y), borderaxespad=0, loc=2, numpoints=1, handlelength=1, scatterpoints=1, markerscale=1., prop=gs_font_leg)
    #leg = ax.legend([handles[idx] for idx in order],[labels[idx] for idx in order], bbox_to_anchor=(0.001, 0.48), borderaxespad=0, loc=2, numpoints=2, handlelength=2, scatterpoints=1, fontsize=label_fontsize)
    leg.get_frame().set_linewidth(0.0)
#plt.legend(frameon=False)
ax.set_xlabel(xlabel, fontproperties=gs_font, fontsize=label_fontsize)
ax.set_ylabel(ylabel, fontproperties=gs_font, fontsize=label_fontsize)

for label in ax.get_xticklabels() :
    label.set_fontproperties(gs_font)
    label.set_color(light_grey)
for label in ax.get_yticklabels() :
    label.set_fontproperties(gs_font)
    label.set_color(light_grey)
ax.set_xlabel(xlabel, fontproperties=gs_font, fontsize=label_fontsize)
ax.set_ylabel(ylabel, fontproperties=gs_font, fontsize=label_fontsize)
for axis in ['top', 'bottom', 'left', 'right']:
    ax.spines[axis].set_color(light_grey)
ax.spines['top'].set_linestyle(':')
ax.spines['right'].set_linestyle(':')

plt.tick_params(labelsize=label_fontsize)
plt.tight_layout()
plt.savefig(outfile)
