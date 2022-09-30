import functools
import tensorflow as tf
from tensorflow import keras
from tensorflow.python.keras.utils.layer_utils import count_params
import pickle
import os
import tempfile
import tqdm
import matplotlib as mpl
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
import kerastuner as kt
import sklearn
from sklearn.metrics import confusion_matrix
from sklearn.model_selection import train_test_split
from sklearn.preprocessing import StandardScaler
from sklearn.model_selection import train_test_split, ShuffleSplit

METRICS = [
    keras.metrics.TruePositives(name='tp'),
    keras.metrics.FalsePositives(name='fp'),
    keras.metrics.TrueNegatives(name='tn'),
    keras.metrics.FalseNegatives(name='fn'),
    keras.metrics.BinaryAccuracy(name='accuracy'),
    keras.metrics.Precision(name='precision'),
    keras.metrics.Recall(name='recall'),
    #keras.metrics.SpecificityAtSensitivity(0.8, num_thresholds=1, name="SpecificityAtSensitivity"),
    keras.metrics.AUC(name='auc'),
    keras.metrics.AUC(name='prc', curve='PR'),  # precision-recall curve
]

def m2tex(model,modelName="model"):
    print("Model ", modelName)
    table=pd.DataFrame(columns=["Layer Type","(Input, output) shape","# Parameters"])
    for layer in model.layers:
        trainable_count = count_params(layer.trainable_weights)
        non_trainable_count = count_params(layer.non_trainable_weights)
        table = table.append({"Layer Type": layer.__class__.__name__,"(Input, output) shape":(layer.input_shape[1],layer.output_shape[1]),"# Parameters": trainable_count+non_trainable_count }, ignore_index=True)
    print(table.to_latex(caption=f"MLP architecture for {modelName}"))
    """stringlist = []
    model.summary(line_length=70, print_fn=lambda x: stringlist.append(x))
    del stringlist[1:-4:2]
    del stringlist[-1]
    for ix in range(1,len(stringlist)-3):
        tmp = stringlist[ix]
        stringlist[ix] = tmp[0:31]+"& "+tmp[31:59]+"& "+tmp[59:]+"\\\\ \hline"
    stringlist[0] = "Model: {} \\\\ \hline".format(modelName)
    stringlist[1] = stringlist[1]+" \hline"
    stringlist[-4] += " \hline"
    stringlist[-3] += " \\\\"
    stringlist[-2] += " \\\\"
    stringlist[-1] += " \\\\ \hline"
    prefix = ["\\begin{table}[]", "\\begin{tabular}{lll}"]
    suffix = ["\end{tabular}", "\caption{{Model summary for {}.}}".format(modelName), "\label{tab:model-summary}" , "\end{table}"]
    stringlist = prefix + stringlist + suffix 
    out_str = " \n".join(stringlist)
    out_str = out_str.replace("_", "\_")
    out_str = out_str.replace("#", "\#")
    print(out_str)
    """


#https://github.com/dcpatton/keras_tuner/blob/master/keras_tuning_part2.ipynb
def build_hypermodel(hp,input_shape, init_bias=0):
    METRICS = [
        keras.metrics.TruePositives(name='tp'),
        keras.metrics.FalsePositives(name='fp'),
        keras.metrics.TrueNegatives(name='tn'),
        keras.metrics.FalseNegatives(name='fn'),
        keras.metrics.BinaryAccuracy(name='accuracy'),
        keras.metrics.Precision(name='precision'),
        keras.metrics.Recall(name='recall'),
        #keras.metrics.SpecificityAtSensitivity(1.0, num_thresholds=1, name="SpecificityAtSensitivity"),
        keras.metrics.AUC(name='auc'),
        keras.metrics.AUC(name='prc', curve='PR'),  # precision-recall curve
    ]
    output_bias = init_bias
    if output_bias is not None:
        output_bias = tf.keras.initializers.Constant(output_bias)
    model = keras.Sequential()
    i = 0
    model.add(keras.layers.Dense(
            units=hp.Int(f'u_start', min_value=1, max_value=512, step=16), activation=hp.Choice(name=f'a_start',values=['relu','tanh','elu','selu','swish']),
            input_shape=input_shape))
    model.add(keras.layers.Dropout(0.5))
    for i in range(hp.Int('num_layers',1,3)):
        model.add(keras.layers.Dense(
            units=hp.Int(f'u_{i}', min_value=1, max_value=512, step=16), activation=hp.Choice(name=f'a_{i}',values=['relu','tanh','elu','selu','swish']),
            input_shape=input_shape))
        model.add(keras.layers.Dropout(0.5))
    model.add(keras.layers.Dense(1, activation=hp.Choice(name=f'final_layer', values=["softmax", "sigmoid"])))
    model.compile(
        optimizer=keras.optimizers.Adam(lr=1e-3),
        loss=keras.losses.BinaryCrossentropy(),
        metrics=METRICS)

    return model

def make_model(input_shape, init_bias=0):
    METRICS = [
        keras.metrics.TruePositives(name='tp'),
        keras.metrics.FalsePositives(name='fp'),
        keras.metrics.TrueNegatives(name='tn'),
        keras.metrics.FalseNegatives(name='fn'),
        keras.metrics.BinaryAccuracy(name='accuracy'),
        keras.metrics.Precision(name='precision'),
        keras.metrics.Recall(name='recall'),
        keras.metrics.AUC(name='auc'),
        keras.metrics.AUC(name='prc', curve='PR'),  # precision-recall curve
        #keras.metrics.SpecificityAtSensitivity(1.0, num_thresholds=1, name="SpecificityAtSensitivity"),
    ]
    output_bias = init_bias
    if output_bias is not None:
        output_bias = tf.keras.initializers.Constant(output_bias)
    model = keras.Sequential([
        keras.layers.Dense(
            302, activation='relu',
            input_shape=input_shape),
        keras.layers.Dropout(0.5),
        keras.layers.Dense(150, activation='relu'),
        keras.layers.Dropout(0.5),
        keras.layers.Dense(75, activation='relu'),
        keras.layers.Dense(1, activation='sigmoid',
                        bias_initializer=output_bias),
    ])

    model.compile(
        optimizer=keras.optimizers.Adam(lr=1e-3),
        loss=keras.losses.BinaryCrossentropy(),
        metrics=METRICS)

    return model

def prepare_data(poly_idx):
    with open("captures_multiclass_xof.pickle", "rb") as fp:
        all_traces = pickle.load(fp)
    with open("values_multiclass_xof.pickle", "rb") as fp:
        values = pickle.load(fp)
    sample_len = len(all_traces[1])
    sample_num = len(all_traces)
    sample_percentage = 0.035
    print("sample num", sample_num)
    print("sample len", sample_len)
    TRACED_VALUE = 0#(1<<17) # 0
    labels_mapping = {TRACED_VALUE:1}#{0: 1, 32: 1, -16: 2} # (1<<17):1
    MIN_RANGE = -92#130980
    MAX_RANGE = 92#131071
    for v in values:
        if v[poly_idx] not in labels_mapping:
            labels_mapping[v[poly_idx]] = 0
    labels = labels_mapping.keys()
    #print(labels_mapping)
    X = []#numpy.empty(shape=(sample_num, sample_len))
    y = []#numpy.empty(shape=(sample_num, 4), dtype=numpy.int64)#,dtype=numpy.int32)
    nonzero_counter = 0
    zero_counter = 0
    for i in tqdm.tqdm(range(sample_num)):
        if not (values[i][poly_idx] >= MIN_RANGE and values[i][poly_idx]  <= MAX_RANGE):
            continue
        y_label = [len(labels) if x not in labels else labels_mapping[x] for x in values[i]]
        if y_label[poly_idx] == labels_mapping[MIN_RANGE]:
            X.append(all_traces[i][:sample_len])
            y.append(y_label)
            nonzero_counter +=1 
        #if  X is None:
        #    X = numpy.array(self.all_traces[i][:sample_len])
        #    y = numpy.array(y_label).reshape(1,-1)
        #if y_label[0] == 0:
        #    X = numpy.vstack((X, self.all_traces[i][:sample_len]))
        #    y = numpy.vstack((y, numpy.array(y_label)))
    for i in tqdm.tqdm(range(sample_num)):
        y_label = [len(labels) if x not in labels else labels_mapping[x] for x in values[i]]
        if y_label[poly_idx] == labels_mapping[TRACED_VALUE] and zero_counter/nonzero_counter <= sample_percentage: #(scipy.bincount(y)[:,0], minlength=2)[1]/scipy.bincount(y[:,0], minlength=2)[0]) <= sample_percentage:
            X.append(all_traces[i][:sample_len])
            y.append(y_label)
            zero_counter +=1
    X = np.array(X)
    y = np.array(y)
    return X,y
    
class UnpackKerasClassifier():

    def learn_and_save(self, poly_idx):
        #https://dcpatton.medium.com/hyperparameter-optimization-with-the-keras-tuner-part-2-acc05612e05e
        #https://github.com/dcpatton/keras_tuner/blob/master/keras_tuning_part2.ipynb
        EPOCHS = 100
        BATCH_SIZE = 512
        test_size =0.2
        early_stopping = tf.keras.callbacks.EarlyStopping(
            monitor='val_precision', 
            verbose=1,
            patience=30,
            mode='max',
            restore_best_weights=True)
        X,y = prepare_data(poly_idx)
        y = y[:,poly_idx]
        print(y)
        neg, pos = np.bincount(y[:].reshape(-1))
        total = neg + pos
        print('Examples:\n    Total: {}\n    Positive: {} ({:.2f}% of total)\n'.format(
            total, pos, 100 * pos / total))
        model = make_model(X[0].shape, init_bias=np.log(pos/total))
        model.summary()
        all_idxs = range(len(X))
        train_idx, test_idx = train_test_split(all_idxs, test_size=0.3)
        train_idx, val_idx = train_test_split(train_idx, test_size=0.3)
        #train_idx, test_idx = list(ShuffleSplit(n_splits=1,test_size=test_size).split(X,y))[0]
        #train_idx, val_idx =  list(ShuffleSplit(n_splits=1,test_size=test_size).split(X[train_idx],y[train_idx]))[0]

        train_x = X[train_idx]
        train_y = y[train_idx]
        test_x = X[test_idx]
        test_y = y[test_idx]
        val_x = X[val_idx]
        val_y = y[val_idx]
        
        scaler = StandardScaler()
        train_features = scaler.fit_transform(train_x)
        test_features = scaler.transform(test_x)
        val_features = scaler.transform(val_x)
        #results = model.evaluate(train_features,train_y, batch_size=BATCH_SIZE, verbose=0)
        #print("Loss: {:0.4f}".format(results[0]))
        # Scaling by total/2 helps keep the loss to a similar magnitude.
        # The sum of the weights of all examples stays the same.
        weight_for_0 = (1 / neg) * (total / 2.0)
        weight_for_1 = (1 / pos) * (total / 2.0)

        class_weight = {0: weight_for_0, 1: weight_for_1}

        print('Weight for class 0: {:.2f}'.format(weight_for_0))
        print('Weight for class 1: {:.2f}'.format(weight_for_1))

        build_hp_function = functools.partial(build_hypermodel,input_shape=X[0].shape, init_bias=np.log(pos/total))
        tuner = kt.Hyperband(hypermodel=build_hp_function,
                            objective=kt.Objective('val_prc', direction="max"),
                            max_epochs=100)
        #tuner = kt.Hyperband(hypermodel=build_hp_function,
        #                objective=kt.Objective('val_prc', direction="max"),
        #                max_trials=300,
        #                #num_initial_points=2,
        #                directory=f'test_dir_{poly_idx}_xof',
        #                project_name='a',
        #                )

        tuner.search_space_summary()
        tuner.search(train_features, train_y, epochs=EPOCHS, batch_size=BATCH_SIZE, 
             validation_data=(val_features, val_y), 
             verbose=1, callbacks=[early_stopping],class_weight = class_weight)
        best_hps = tuner.get_best_hyperparameters(num_trials = 1)[0]
        model = tuner.hypermodel.build(best_hps)
        model.summary()
        print("#### Now need to train ###")
        careful_bias_history =model.fit(train_features, train_y,batch_size=BATCH_SIZE,epochs=EPOCHS*500,validation_data=(val_features,val_y),callbacks=[early_stopping],verbose=1)#,class_weight=class_weight)#callbacks=[early_stopping]
        

        baseline_results = model.evaluate(test_features, test_y,
                                        batch_size=BATCH_SIZE, verbose=0)
        for name, value in zip(model.metrics_names, baseline_results):
            print(name, ': ', value)
        model.save(f"keras_classifier_{poly_idx}_xof.model")
        with open(f"scaler_model_{poly_idx}_xof.bin", "wb") as fp:
            pickle.dump(scaler, fp)
        self.scaler = scaler
        self.model = model

    def predict(self, data):

        scaled_data = self.scaler.transform(data)
        return self.model.predict(scaled_data)

    @classmethod
    def from_saved(cls, poly_idx):
        obj = cls()
        obj.model = keras.models.load_model(f"keras_classifier_{poly_idx}_xof.model")
        obj.model.summary()
        obj.model.compile(
        optimizer=keras.optimizers.Adam(lr=1e-3),
        loss=keras.losses.BinaryCrossentropy(),
        metrics=METRICS)
        m2tex(obj.model, f"coefficient {poly_idx}")
        with open(f"scaler_model_{poly_idx}_xof.bin", "rb") as fp:
            obj.scaler = pickle.load(fp)
        X,y = prepare_data(poly_idx)
        test_y = y[:,poly_idx]
        test_features = obj.scaler.transform(X)
        baseline_results = obj.model.evaluate(test_features, test_y,
                                        batch_size=20, verbose=0)
        for name, value in zip(obj.model.metrics_names, baseline_results):
            print(name, ': ', value)
        return obj

class PolyClassifier():

    def learn_and_save(self):
        self.classifiers = []
        for poly_idx in range(4):
            print(f"### Now doing {poly_idx} ###")
            clf = UnpackKerasClassifier()
            clf.learn_and_save(poly_idx)
            self.classifiers.append(clf)
    
    def predict(self, data):
        return [clf.predict(data) for clf in self.classifiers]

    @classmethod
    def from_saved(cls):
        obj = cls()
        obj.classifiers = []
        for poly_idx in range(4):
            obj.classifiers.append(UnpackKerasClassifier.from_saved(poly_idx))
        return obj

    



if __name__ == "__main__":
    c = PolyClassifier()
    c.learn_and_save()
