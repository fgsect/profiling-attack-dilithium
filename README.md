# Project overview
This repository contains the code for the paper `Profiling Side-Channel Attacks on Dilithium: A Small Bit-Fiddling Leak Breaks It All`.

The code is structured in the following way:
`secret_key_retrieval` contains the source code for the secret key retrieval as described in section 5.
Most of the attack logic is implemented in `secret_key_retrieval/attack/side_channel_attack.cpp`, 
`secret_key_retrieval/attack/ilp_solver.cpp` and ``secret_key_retrieval/attack/integer_lwe.cpp`.

The project uses the `NTL` library and `SCIP` solver to solve the ILP.

The machine learning setup can be found in `proof_of_concept/machine_learning` and 
the chipwhisperer firmware in `proof_of_concept/chipwhisperer/victims/unpack_xof/`

# Usage instructions
1. First, run the docker like so:
```
./run_ntl_docker.sh
```
This will create a docker containing NTL and SCIP and will give you a shell in that docker.
2. In the docker container, navigate into `/current_dir`. Here, your current directory will be mounted
```
cd current_dir/secret_key_retrieval/src
```
3. Build the project (with NIST security level 2)
```
./build.sh
```
4. For the theoretical evaluations, run:
```
./build/main <false-positive rate> <true positive rate> <threshold>
```
where threshold is the maximum value for |z_{i,j}| for which the possibility that y_{i,j} = 0 is not dismiseed, see section 6 of the paper. This will store the results in "ilp_evaluation.csv"
5. To generate the data for the proof of concept evaluation on the chipwhisperer, do:
```
mkdir poc_data
./build/generate_data poc_data
```

This will store the public key, private key, the y-coefficients and the generated signatures and the respective inputs to the `polyz_unpack` function in 
```
poc_data
```
Now, change to the project `chipwhisperer_evaluation`. 
Run the `./setup_chipwhisperer.sh` script. This will copy the `unpack`
target in the right place in the chipwhisperer repo.

To collect training data, first generate the inputs for the unpack function:
```
cd machine_learning/generate_data
make
./main_xof | tee xof_data.txt # Let it run for a while
cd ..
```
Then run:
```
python3 cw_collect_training_data.py
```
while the chipwhisperer with the STMF32F4 target is connected to your laptop. This will collect the training data:
Then run:
```
python3 train_classifier.py
```
To deduce the hyperparameters of the 4 machine-learning models that predict the zero-coefficients. The scripts also trains the models and evaluates the results on a test set.
Then, run the polyz_unpack function with the inputs generated in the earlier step and predict the coefficients like so:
```
python3 unpack_send_xof.py
```
This will list the cofficients predicted to be zero in the file `predicted_zero.txt`.
As the last step, run the `./build/execute_attack` binary to retrieve the secret key (and test if the the attack worked):
```
./build/execute_attack <path to poc_data> <path to predicted_zero.txt>
```
This will read the public key, the signatures and the coefficients that are predicted to be zero
and launch attack as described in section 5. 
