
import numpy as np
import pickle
from numpy.lib.polynomial import poly
import tqdm
import seaborn as sns
import matplotlib.pyplot as plt
import matplotlib.patches as mpatches
from sklearn.preprocessing import StandardScaler
import scipy.stats#.ttest_ind

def prepare_data(poly_idx):
    with open("captures_multiclass.pickle", "rb") as fp:
        all_traces = pickle.load(fp)
    with open("values_multiclass.pickle", "rb") as fp:
        values = pickle.load(fp)
    sample_len = len(all_traces[1])
    sample_num = len(all_traces)
    sample_percentage = 0.035
    print("sample num", sample_num)
    print("sample len", sample_len)
    TRACED_VALUE = 0  # (1<<17) # 0
    labels_mapping = {TRACED_VALUE: 1}  # {0: 1, 32: 1, -16: 2} # (1<<17):1
    MIN_RANGE = -92  # 130980
    MAX_RANGE = 92  # 131071
    for v in values:
        if v[poly_idx] not in labels_mapping:
            labels_mapping[v[poly_idx]] = 0
    labels = labels_mapping.keys()
    # print(labels_mapping)
    X = []  # numpy.empty(shape=(sample_num, sample_len))
    # numpy.empty(shape=(sample_num, 4), dtype=numpy.int64)#,dtype=numpy.int32)
    y = []
    nonzero_counter = 0
    zero_counter = 0
    for i in tqdm.tqdm(range(sample_num)):
        if not (values[i][poly_idx] >= MIN_RANGE and values[i][poly_idx] <= MAX_RANGE):
            continue
        y_label = [len(labels) if x not in labels else labels_mapping[x]
                   for x in values[i]]
        if y_label[poly_idx] == labels_mapping[MIN_RANGE]:
            X.append(all_traces[i][:sample_len])
            y.append(y_label)
            nonzero_counter += 1
        # if  X is None:
        #    X = numpy.array(self.all_traces[i][:sample_len])
        #    y = numpy.array(y_label).reshape(1,-1)
        # if y_label[0] == 0:
        #    X = numpy.vstack((X, self.all_traces[i][:sample_len]))
        #    y = numpy.vstack((y, numpy.array(y_label)))
    for i in tqdm.tqdm(range(sample_num)):
        y_label = [len(labels) if x not in labels else labels_mapping[x]
                   for x in values[i]]
        # (scipy.bincount(y)[:,0], minlength=2)[1]/scipy.bincount(y[:,0], minlength=2)[0]) <= sample_percentage:
        if y_label[poly_idx] == labels_mapping[TRACED_VALUE] and zero_counter/nonzero_counter <= sample_percentage:
            X.append(all_traces[i][:sample_len])
            y.append(y_label)
            zero_counter += 1
    X = np.array(X)
    y = np.array(y)
    return X, y


def plot_poly_idx(poly_idx):
    fig = plt.figure()
    plt.figure().clear()
    X, y = prepare_data(poly_idx)
    scaler = StandardScaler()
    X = scaler.fit_transform(X)
    HISTPLOT = list(range(35, 50))
    # for cf, zgdata in training_kx_data.groupby(['Kx']):
    for idx in list(range(1000))+list(range(-1000, 0)):
        row = X[idx, i*100:(i+1)*150]  # 0-150,100-300,200-450,300-600
        plt.plot(row, alpha=.01, color=(
            'red' if y[idx][poly_idx] == 0 else 'blue'))
    #plt.legend({'$y_{i,j}=0$': 'red','$y_{i,j}!=0$': 'blue'})

    red_patch = mpatches.Patch(color='red', label='$y_{i,j} =0$')
    blue_patch = mpatches.Patch(color='blue', label='$y_{i,j} !=0$')
    plt.legend(handles=[red_patch, blue_patch])
    plt.savefig(f"plot_traces_{poly_idx}_scaled.png")
    #f, axes = plt.subplots(nrows=len(HISTPLOT), ncols=1, figsize=(10, .8*len(HISTPLOT)))

    """LABELS = {'$x=0$': 0, '$x!=0$': 1}#, '$x=2$': 512, '$x=3$': 786, '$x=4$': 1024}

    for i, ax in zip(HISTPLOT, axes):
        for j, (l, lval) in enumerate(LABELS.items()):
            v = X[y[:,poly_idx]==lval][:, i]
            print(X[y[:,poly_idx]==lval].shape)
            if len(v) == 0: continue
            v = v[np.abs(v) < 2]
            ax.hist(v, density=True, bins=np.linspace(np.min(v), np.max(v), len(np.unique(v))-1), alpha=.5)
        ax.set_ylabel(f'#{1890+i}')
        ax.set_yticks([])
        ax.set_xticks([])
        ax.spines['top'].set_visible(False)
        ax.spines['right'].set_visible(False)
        ax.spines['left'].set_visible(False)

    axes[0].set_title('Distribution of Power Usage of the Dilithium power consumption\n')
    axes[-1].legend(LABELS.keys())
    axes[-1].set_xlabel('Normalized Voltage')
    plt.show()
    """


class TTest(object):
    def __init__(self, nb_samples, nb_curves=1, name="T-Test"):
        self.nb_samples = nb_samples
        self.name = name
        self.counts = [0, 0]
        self.sum_x = np.zeros((2, nb_samples), dtype=float)
        self.sum_xx = np.zeros((2, nb_samples), dtype=float)

    def __del__(self): pass
    def get_name(self): return self.name

    def add_curve(self, curve, group):
        group = group
        self.counts[group] += 1
        self.sum_x[group] += curve
        self.sum_xx[group] += curve * curve

    def get_result(self):
        n_0 = self.counts[0]
        n_1 = self.counts[1]
        self.mean_0 = self.sum_x[0] / n_0
        self.mean_1 = self.sum_x[1] / n_1
        var_0 = self.sum_xx[0] / n_0
        var_0 -= self.mean_0 * self.mean_0
        var_1 = self.sum_xx[1] / n_1
        var_1 -= self.mean_1 * self.mean_1
        res = self.mean_0 - self.mean_1
        res /= np.sqrt((var_0/n_0) + (var_1/n_1))
        return res

    def get_result_scipy(self, X, y, poly_idx):
        res = []
        for i in tqdm.tqdm(range(self.nb_samples)):
            fixed_samples = X[y[:,poly_idx]==0][:,i]
            random_samples = X[y[:,poly_idx]!=0][:,i]
            print(scipy.stats.ttest_ind(fixed_samples, random_samples,equal_var=False))
            res.append(scipy.stats.ttest_ind(fixed_samples, random_samples,equal_var=False).statistic)
        return res



def t_test(poly_idx):
    X, y = prepare_data(poly_idx)
    print(X.shape)
    t = TTest(X.shape[1])
    curve = t.get_result_scipy(X,y,poly_idx)
    
    #for i in range(X.shape[0]):
    #    t.add_curve(X[i], y[i][poly_idx])
    #curve = t.get_result()
    #print("Mean",t.mean_0)
    plt.clf()
    ax = plt.subplot(1,1,1)
    ax.set_title(f"T-test for power consumption of coefficient $(i+{poly_idx})$")
    ticks_step = len(curve)/5
    # round to hundred
    if ticks_step > 100:
            ticks_step = int(ticks_step/100)
            ticks_step = ticks_step * 100
    plt.xticks(np.arange(0, len(curve), step=ticks_step))
    plt.xlabel('Sample')
    plt.ylabel("T-test value")
    plt.plot(curve)
    plt.savefig(f"plot_traces_t_test_{poly_idx}.png")


if __name__ == "__main__":
    for i in range(0,4):
        t_test(i)
    # for i in range(0,4):
    #    plot_poly_idx(i)
