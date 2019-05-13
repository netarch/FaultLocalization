import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

def plot_cdf(arr, label, outfile, ymin=0.0, ymax=1.0, xmin=None, xmax=None): 
    arr = sorted(arr)
    fig, ax = plt.subplots()
    y = [float(i)/len(arr) for i in range(len(arr))]
    pcolor = '#af0505' 
    ax.plot(arr, y, '-', color=pcolor,  lw=2.5,  dash_capstyle='round')
    label_fontsize=20
    ax.set_xlabel(label, fontsize=label_fontsize)
    ax.set_ylabel("cdf", fontsize=label_fontsize)
    if xmin != None and xmax != None:
        plt.xlim(xmin=xmin, xmax=xmax)
    elif xmin != None:
        plt.xlim(xmin=xmin)
    elif xmax != None:
        plt.xlim(xmax=xmax)
    plt.ylim(ymin=ymin, ymax=ymax)
    ax.grid(linestyle=':', linewidth=1, color='grey')
    plt.tick_params(labelsize=label_fontsize)
    plt.tight_layout()
    plt.savefig(outfile)

def plot_scores(scorex, scorey, xlabel, ylabel, colors):
    fig, ax = plt.subplots()
    plt.scatter(x, y, c=colors, marker='x', s=50, linewidth=2.5)
    label_fontsize=20
    ax.set_xlabel(xlabel, fontsize=label_fontsize)
    ax.set_ylabel(ylabel, fontsize=label_fontsize)
    plt.xlim(xmin=min(x),xmax=1.05*max(x))
    plt.ylim(ymin=min(y),ymax=1.05*max(y))
    ax.grid(linestyle=':', linewidth=1, color='grey')
    plt.tick_params(labelsize=label_fontsize)
    plt.tight_layout()
    plt.savefig(outfile)

