import numpy as np
import sys
import operator
import matplotlib.pyplot as plt
import matplotlib.font_manager as fm
 
PRECISION = False
SKEWED = False

#pcolors = ['#0e326d', '#a5669f', '#028413', '#00112d', '#af0505', '#db850d']
#pcolors = ['#0e326d', '#a5669f', '#028413', '#db850d','#000000',  '#af0505', '#330066', '#cc00cc', '#cc6600', '#999900']
pcolors = ['#0e326d', '#000000', '#028413', '#a5669f', '#db850d', '#af0505', '#330066', '#cc6600']
#pcolors = ['#0e326d', '#028413', '#00112d']
markers = ['o', 's', '^', 'v', 'p', '*', 'p', 'h']
gs_font = fm.FontProperties(fname='../gillsans.ttf', size=25, weight='bold')
gs_font_inset = fm.FontProperties(fname='../gillsans.ttf', size=15, weight='bold')
list.reverse(pcolors)
light_grey=(0.5,0.5,0.5)

def string(x):
    if x >= (1024 * 1024 * 1024):
        return str(x/(1024*1024*1024)) + 'B'    
    elif x >= (1024 * 1024):
        return str(x/(1024*1024)) + 'M'
    elif x >= 1024:
        return str(x/1024) + 'K'
    else:
        return str(x)

def Fscore(point):
    p, r = point
    rcprcal = (1.0/p) + (1.0/r)
    return 2.0/rcprcal

def plot_lines_inset(xs, ys, fig):
    # create plot
    ax = plt.axes([0.58, 0.28, 0.1, 0.5])
    xmin, xmax = 0.35, 0.45
    ymin, ymax = 0.45, 0.86
    #ax.set_xscale("log", base=2)
    #ax.set_yscale("log", base=2)
    nlines = len(xs)
    assert (len(ys) == nlines and len(labels) == nlines)
    for i in range(nlines):
        pcolor = pcolors[i]
        markerfacecolor = pcolor
        linestyle = '-'
        if "007" in labels[i]:
            linestyle = '-.'
            markerfacecolor = 'none'
        elif "NetBouncer" in labels[i]:
            linestyle = ':'
        elif "Flock (A1+A2+P)" in labels[i]:
            linestyle = '-'
        elif "Flock (A1)" in labels[i]:
            linestyle = ':'
        elif "Flock (A2)" in labels[i]:
            linestyle = '-.'
            markerfacecolor = 'none'
        #print (pcolor, labels[i], xs[i], ys[i])
        plt.plot(xs[i], ys[i], color=pcolor,  marker=markers[i], mew = 2.5, markersize = 12, markerfacecolor=markerfacecolor, markeredgecolor=pcolor, linestyle='None')
    plt.xlim(xmin=xmin,xmax=xmax)
    plt.ylim(ymin=ymin,ymax=ymax)
    plt.xticks([0.4])
    plt.grid(linestyle=':', linewidth=1, color='grey')
    axcolor='grey'
    label_fontsize=8
    for label in ax.get_xticklabels() :
        label.set_fontproperties(gs_font_inset)
        label.set_color(light_grey)
    for label in ax.get_yticklabels() :
        label.set_fontproperties(gs_font_inset)
        label.set_color(light_grey)
    ax.xaxis.set_tick_params(which='both', colors=axcolor)
    ax.yaxis.set_tick_params(which='both', colors=axcolor)
    for spine in ['bottom', 'top', 'right', 'left']:
        ax.spines[spine].set_color(axcolor)
        ax.spines[spine].set_linewidth(1.5)
    ticklabelcolor = 'black'
    for axis in ['top', 'bottom', 'left', 'right']:
        ax.spines[axis].set_color('black')
    #ax.spines['top'].set_linestyle(':')
    #ax.spines['right'].set_linestyle(':')
    axcolor='grey'
    #ax.xaxis.set_tick_params(width=2, length=8)
    #ax.yaxis.set_tick_params(width=2, length=8)
    ax.xaxis.set_tick_params(which='both', colors=axcolor)
    ax.yaxis.set_tick_params(which='both', colors=axcolor)
    ticklabelcolor = 'grey'
    #ax.tick_params(axis='x', colors=ticklabelcolor, direction="in")
    #ax.tick_params(axis='y', colors=ticklabelcolor, direction="in")




def plot_lines(xs, ys, yerrs, labels, xlabel, ylabel, outfile):
    # create plot
    fig, ax = plt.subplots()
    #ax.set_xscale("log", base=2)
    #ax.set_yscale("log", base=2)
    nlines = len(xs)
    assert (len(ys) == nlines and len(labels) == nlines)
    for i in range(nlines):
        pcolor = pcolors[i]
        markerfacecolor = pcolor
        linestyle = '-'
        if "007" in labels[i]:
            linestyle = '-.'
            markerfacecolor = 'none'
        elif "NetBouncer" in labels[i]:
            linestyle = ':'
        elif "Flock (A1+A2+P)" in labels[i]:
            linestyle = '-'
        elif "Flock (A1)" in labels[i]:
            linestyle = ':'
        elif "Flock (A2)" in labels[i]:
            linestyle = '-.'
            markerfacecolor = 'none'
        #print (pcolor, labels[i], xs[i], ys[i])
        ax.plot(xs[i], ys[i], color=pcolor,  lw=2,  marker=markers[i], mew = 2.5, markersize = 12, markerfacecolor=markerfacecolor, markeredgecolor=pcolor, linestyle=linestyle, dash_capstyle='round', label=labels[i])
        #ax.plot(xs[i], ys[i], '-', color=pcolor,  lw=3.5,  dash_capstyle='round', label=labels[i])
        #ax.scatter(xs[i], ys[i], s=70, color=pcolor, alpha=1.0, marker='x', label=labels[i])
        #ax.errorbar(xs[i], ys[i],  yerr=yerrs[i], color=pcolor,  lw=3.0,  dash_capstyle='round', label=labels[i], fmt='-')
    label_fontsize=25
    xmin = 0.1
    ymin = 0.0
    plt.xlim(xmin=xmin,xmax=1.41)
    plt.ylim(ymin=ymin, ymax=1.0)
    ticklabelcolor = 'black'
    xticks = np.array([0.2, 0.6, 1.0, 1.4])
    ax.xaxis.set_ticks(xticks)
    xticks = np.array([string(x) for x in xticks])
    ax.set_xticklabels(xticks, color=ticklabelcolor)
    ax.set_xlabel(xlabel, fontsize=label_fontsize)
    ax.set_ylabel(ylabel, fontsize=label_fontsize)
    xlabels = ax.get_xticklabels()
    xlabels[0] = ""
    print(xlabels)
    xticks = ax.xaxis.get_major_ticks()
    #xticks[0].label1.set_visible(False)
    #ax.set_xticklabels(xlabels)
    #print("xs", xs)
    #print(max([max(x) for x in xs]))
    xmax=1.00*max([max(x) for x in xs])
    #plt.xlim(xmin=2,xmax=12.0)
    ax.grid(linestyle=':', linewidth=1, color='grey')
    label_fontsize=25
    #if True or not SKEWED:
    #    handles, labels = plt.gca().get_legend_handles_labels()
    #    order = [0,1,2,3,4,5,6]
    #    leg = ax.legend([handles[idx] for idx in order],[labels[idx] for idx in order], bbox_to_anchor=(0.4, 0.65), borderaxespad=0, loc=2, numpoints=2, handlelength=3, prop=gs_font, fontsize=label_fontsize)
    bbox_x, bbox_y = 0.4, 0.80
    if SKEWED:
        bbox_x, bbox_y = 0.4, 0.8
    leg = ax.legend(bbox_to_anchor=(bbox_x, bbox_y), borderaxespad=0, loc=2, numpoints=2, handlelength=3, prop=gs_font, fontsize=label_fontsize)
    #leg.get_frame().set_linewidth(0.0)
    ax.set_xlabel(xlabel, fontproperties=gs_font, fontsize=label_fontsize)
    ax.set_ylabel(ylabel, fontproperties=gs_font, fontsize=label_fontsize)
    plt.tick_params(labelsize=label_fontsize)
    #lim = ax.get_xlim()
    #ax.set_xticks(list(ax.get_xticks()) + [1.0e-4])
    #ax.set_xlim(lim)
    axcolor='black'
    ax.xaxis.set_tick_params(width=2, length=10)
    ax.yaxis.set_tick_params(width=2, length=15)
    ax.xaxis.set_tick_params(which='both', colors=axcolor)
    ax.yaxis.set_tick_params(which='both', colors=axcolor)
    ax.spines['bottom'].set_color(axcolor)
    ax.spines['top'].set_color(axcolor)
    ax.spines['right'].set_color(axcolor)
    ax.spines['left'].set_color(axcolor)
    ticklabelcolor = 'black'
    ax.tick_params(axis='x', colors=ticklabelcolor, direction="in")
    ax.tick_params(axis='y', colors=ticklabelcolor, direction="in")

    for axis in ['top', 'bottom', 'left', 'right']:
        ax.spines[axis].set_color(light_grey)
    ax.spines['top'].set_linestyle(':')
    ax.spines['right'].set_linestyle(':')

    #plot_lines_inset(xs, ys, fig)
    xc = [0.35, 0.35, 0.45, 0.45, 0.35]
    yc = [0.4, 0.9, 0.9, 0.4, 0.4]
    #ax.plot(xc, yc, color="black",lw=1.5)
    # ax.annotate(" ", xy=(0.72,  0.57), xytext=(0.46, 0.45), arrowprops=dict(arrowstyle="->",lw=2.5),zorder=4)
    for label in ax.get_xticklabels() :
        label.set_fontproperties(gs_font)
        label.set_color(light_grey)
    for label in ax.get_yticklabels() :
        label.set_fontproperties(gs_font)
        label.set_color(light_grey)
    #ax.tick_params(axis='x', labelsize=19, color=light_grey)
    plt.tight_layout()
    plt.savefig(outfile)

def string_to_numeric_array(s):
    return [float(x) for x in s.split()]

datadir = sys.argv[1]
outfile = sys.argv[2]

if "skewed" in outfile:
    SKEWED = True

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

## Random traffic
drop_rate_strs = ["0.002", "0.004", "0.006", "0.01"] #, "0.014"]# , 0.018, 0.022]
drop_rates = [100 * float(x) for x in drop_rate_strs]


def GetPrArr(scheme):
    file_prefix = datadir + "/" + "test_random_" #0.014_bnet_a2
    ret = []
    for drop_rate_str in drop_rate_strs:
        filename = file_prefix + drop_rate_str + "_" + scheme
        params = GetParamsFromFile(filename)
        assert (len(params) == 1)
        param = list(params.keys())[0]
        pr = params[param]
        ret.append(pr)
    return ret


flock_a1 = GetPrArr("bnet_a1")
flock_a1_p = GetPrArr("bnet_a1_p")
flock_int = GetPrArr("bnet_int")
flock_a1_a2_p = GetPrArr("bnet_a1_a2_p")
flock_a2 = GetPrArr("bnet_a2")
nb_int = GetPrArr("nb_int")
nb_a1 = GetPrArr("nb_a1")
doubleO7_a2 = GetPrArr("007_a2")




'''
nb_a1 = [(0.875, 0.03125), (0.947917, 0.3125), (0.84375, 0.6875), (0.833333, 0.9375), (0.890625, 1), (0.877604, 1)]
flock_a1 = [(1, 0.09375), (0.96875, 0.375), (1, 0.59375), (1, 0.90625), (1, 0.96875), (1, 1)] #-10
flock_a1_p = [(1, 0.09375), (1, 0.65625), (1, 0.875), (1, 1), (1, 1), (1, 1)]
nb_int = [(1, 0.0625), (1, 0.53125), (0.984375, 0.90625), (1, 1), (0.979167, 1), (1, 1)]
flock_int = [(1, 0.1875), (1, 0.71875), (1, 0.875), (1, 1), (1, 1), (1, 1)]
flock_a1_a2_p = [(1, 0.21875), (1, 0.71875), (1, 0.875), (1, 1), (1, 1), (1, 1)]
doubleO7_a2 = [(1, 0.21875), (1, 0.5), (1, 0.625), (1, 0.9375), (1, 0.96875), (1, 1)]
flock_a2 = [(1, 0.03125), (1, 0.65625), (1, 0.96875), (1, 1), (1, 1), (0.984375, 1)]
'''

## Skewed traffic
if (SKEWED):
    #40Gbps
    flock_int = [(0.984375, 0.5), (0.984375, 0.96875), (0.96875, 1), (0.96875, 1), (0.984375, 1), (0.984375, 1), ]
    flock_a1_a2_p = [(0.979167, 0.4375), (0.984375, 0.84375), (0.96875, 1), (0.96875, 1), (1, 1), (0.984375, 1), ]
    flock_a1 = [(1, 0.0625), (1, 0.4375), (0.96875, 0.71875), (1, 0.8125), (1, 0.96875), (1, 1), ]
    flock_a1_p = [(0.9375, 0.34375), (0.984375, 1), (0.984375, 1), (0.963542, 1), (0.984375, 1), (1, 1), ]
    flock_a2 = [(0.947917, 0.375), (0.901042, 0.625), (0.932292, 0.75), (0.921875, 0.96875), (0.9375, 1), (0.984375, 1), ]
    nb_int = [(0.96875, 0.0625), (0.90625, 0.75), (0.984375, 0.9375), (0.96875, 1), (0.921875, 1), (0.916667, 1), ]
    nb_a1 = [(0.90625, 0.0625), (0.96875, 0.34375), (0.880208, 0.71875), (0.875, 0.9375), (0.885417, 1), (0.901042, 1), ]
    doubleO7_a2 = [(0.9375, 0.0625), (0.96875, 0.15625), (0.90625, 0.0625), (0.9375, 0.0625), (0.9375, 0.09375), (1, 0.125)]


def Fscore(point):
    p, r = point
    if p<1.0e-3 or r<1.0e-3:
        return 0
    rcprcal = (1.0/p) + (1.0/r)
    return 2.0/rcprcal

error_vs_nb_a1 = 0.0
error_vs_007 = 0.0
error_vs_nb_int = 0.0

accs = []
for scheme in [nb_a1, flock_a1, doubleO7_a2, flock_a2, nb_int, flock_int, flock_a1_a2_p]:
    accs.append((sum(i for i, j in scheme)/len(scheme), sum(j for i, j in scheme)/len(scheme)))

print(accs)
errors = [1.0 - Fscore(acc) for acc in accs]
print(errors)
print("vs nb_a1", errors[0]/errors[1], "vs 007", errors[2]/errors[3], "vs nb_int", errors[4]/errors[5])

print("flock(a1_p) vs nb_a1", errors[0]/errors[6], "flock(a1_a2_p) vs 007", errors[2]/errors[6], "flock(a1_p) vs flock(a1)", errors[0]/errors[1]) 

for i in range(len(drop_rates)):
    fscore_007 = Fscore(doubleO7_a2[i])
    fscore_flock_a2 = Fscore(flock_a2[i])
    fscore_nb_int = Fscore(nb_int[i])
    fscore_flock_int = Fscore(flock_int[i])
    fscore_nb_a1 = Fscore(nb_a1[i])
    fscore_flock_a1 = Fscore(flock_a1[i])
    fscore_flock_a1_p = Fscore(flock_a1_p[i])
    e_nb_a1 =  (1.0-fscore_nb_a1)/max(1.0e-30, (1.0-fscore_flock_a1))
    e_007 = (1.0 - fscore_007)/max(1.0e-30, (1.0 - fscore_flock_a2))
    e_nb_int = (1.0-fscore_nb_int)/max(1.0e-30, (1.0-fscore_flock_int))
    e_fa1_fa1p = (1.0-fscore_flock_a1)/max(1.0e-30, fscore_flock_a1_p)
    print("Drop rate :", drop_rates[i], "vs nb_a1", e_nb_a1, " vs 007", e_007, " vs nb_int", e_nb_int, "flock_a1_p vs flock_a1", e_fa1_fa1p)
    error_vs_nb_a1 += e_nb_a1
    error_vs_007 += e_007
    error_vs_nb_int += e_nb_int



b = 1
if PRECISION:
    b = 0
def GetAccuracyArr(p_r_arr):
    return [Fscore(x) for x in p_r_arr]
    #return [(x[0]+x[1])/2.0 for x in p_r_arr]
    #return [x[b] for x in p_r_arr]

#ys = [doubleO7, bayesian_net_active, bayesian_net]
labels_ys = [("Flock (INT)", flock_int), ("Flock (A1+A2+P)", flock_a1_a2_p), ("Flock (A2)", flock_a2),
             ("Flock (A1+P)", flock_a1_p), ("NetBouncer (INT)", nb_int), ("Flock (A1)", flock_a1),
             ("NetBouncer (A1)", nb_a1), ("007 (A2)", doubleO7_a2)]
labels = [x[0] for x in labels_ys]
ys = [x[1] for x in labels_ys]

ys = [GetAccuracyArr(x) for x in ys]
xs = [drop_rates for x in ys]
if SKEWED:
    inds = [i for i in range(len(xs)) if labels[i] not in ["NetBouncer (A1)", "Flock (A1)"]]
    xs = [xs[i] for i in inds]
    ys = [ys[i] for i in inds]
    labels = [labels[i] for i in inds]
    markers = [markers[i] for i in inds]
    pcolors = [pcolors[i] for i in inds]

yerrs = []

if PRECISION:
    ylabel = "precision"
else:
    ylabel = "recall"
ylabel = "Fscore"
xlabel = "drop rate (%)"
plot_lines(xs, ys, yerrs, labels, xlabel, ylabel, outfile)
