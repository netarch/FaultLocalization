import numpy as np
import sys
import operator
import matplotlib.pyplot as plt
import matplotlib.font_manager as fm
 

#pcolors = ['#0e326d', '#a5669f', '#028413', '#00112d', '#af0505', '#db850d']
#pcolors = ['#0e326d', '#a5669f', '#028413', '#db850d','#000000',  '#af0505', '#330066', '#cc00cc', '#cc6600', '#999900']
pcolors = ['#0e326d', '#000000', '#028413', '#a5669f', '#db850d', '#af0505', '#330066', '#cc6600']
#pcolors = ['#0e326d', '#028413', '#00112d']
markers = ['o', 's', '^', 'v', 'p', '*', 'p', 'h']
gs_font = fm.FontProperties(fname='../gillsans.ttf', size=28, weight='bold')
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
        if "Micro" in labels[i]:
            linestyle = ':'
        elif "Operator" in labels[i]:
            linestyle = '-.'
            markerfacecolor = 'none'
        #print (pcolor, labels[i], xs[i], ys[i])
        ax.plot(xs[i], ys[i], color=pcolor,  lw=2,  marker=markers[i], mew = 2.5, markersize = 16, markerfacecolor=markerfacecolor, markeredgecolor=pcolor, linestyle=linestyle, dash_capstyle='round', label=labels[i])
        #ax.plot(xs[i], ys[i], '-', color=pcolor,  lw=3.5,  dash_capstyle='round', label=labels[i])
        #ax.scatter(xs[i], ys[i], s=70, color=pcolor, alpha=1.0, marker='x', label=labels[i])
        #ax.errorbar(xs[i], ys[i],  yerr=yerrs[i], color=pcolor,  lw=3.0,  dash_capstyle='round', label=labels[i], fmt='-')
    label_fontsize=25
    plt.xlim(xmin=400,xmax=3500)
    plt.ylim(ymin=0, ymax=33)
    ticklabelcolor = 'black'
    xticks = np.array([500, 1500, 2500, 3500])
    ax.xaxis.set_ticks(xticks)
    xticks = np.array([str(x) for x in xticks])
    ax.set_xticklabels(xticks, color=ticklabelcolor)
    ax.set_xlabel(xlabel, fontsize=label_fontsize)
    ax.set_ylabel(ylabel, fontsize=label_fontsize)
    xlabels = ax.get_xticklabels()
    xlabels[0] = ""
    print(xlabels)
    xticks = ax.xaxis.get_major_ticks()
    #xticks[0].label1.set_visible(False)
    #ax.set_xticklabels(xlabels)
    ax.grid(linestyle=':', linewidth=1, color='grey')
    label_fontsize=25
    #if True or not SKEWED:
    #    handles, labels = plt.gca().get_legend_handles_labels()
    #    order = [0,1,2,3,4,5,6]
    #    leg = ax.legend([handles[idx] for idx in order],[labels[idx] for idx in order], bbox_to_anchor=(0.4, 0.65), borderaxespad=0, loc=2, numpoints=2, handlelength=3, prop=gs_font, fontsize=label_fontsize)
    bbox_x, bbox_y = 0.03, 0.99
    leg = ax.legend(bbox_to_anchor=(bbox_x, bbox_y), borderaxespad=0, loc=2, numpoints=1, handlelength=2, prop=gs_font, fontsize=label_fontsize)
    leg.get_frame().set_linewidth(0.0)
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

outfile = sys.argv[1]

ftk = [10, 12, 14, 16]
svrs = [3*int(k**3/4) for k in ftk]
micro = [7.2, 9.4, 10.1, 9.5]
operator = [14.55, 17.6, 19.6, 24.6]

avg_x = [operator[i]/micro[i] for i in range(len(micro))]
print("Average reduction in steps", np.mean(avg_x), np.mean(micro), np.mean(operator))

ys = [micro, operator]
labels = ["MicroTel", "Operator"]
xs = [svrs for x in ys]
yerrs = []
ylabel = "Num steps"
xlabel = "Hosts in topology"
plot_lines(xs, ys, yerrs, labels, xlabel, ylabel, outfile)
