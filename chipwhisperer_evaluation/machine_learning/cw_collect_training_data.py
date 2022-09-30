#!/usr/bin/env python
# coding: utf-8
import collections
import struct
from numpy.core.numeric import tensordot
import tqdm
import subprocess
import chipwhisperer as cw
import pickle
import collections
import random
import hashlib
import numpy
import helpers



def randombytes(seed):
    buf = bytearray(8)
    for i in range(8):
        buf[i] = (numpy.uint8(seed >> 8  * i))
    m = hashlib.shake_128()
    m.update(bytes(buf))
    return m.digest(64)

def capture_value(scope, target, seed):
    while True:
        
        msg = helpers.xof_shake256(randombytes(seed), 0)[:9]
        scope.arm()
        target.flush()
        target.send_cmd(0x01,0x01,msg)
        print("Sending msg", msg)
        capture = scope.capture()
        return_value = target.simpleserial_read("r", 4)
        #print("trig count", scope.adc.trig_count)
        print(return_value)
        #return_value = target.simpleserial_wait_ack(timeout=1000000)
        #print(return_value)
        target.flush()
        if return_value is None:
            continue
        print("Trig count", scope.adc.trig_count)
        return scope.get_last_trace()[:scope.adc.trig_count]

def main():
    reverse_dict = collections.defaultdict(lambda: collections.defaultdict(list))
    output_dict = {}
    count = 0
    with open("generate_data/xof_data.txt" ) as fp:
        for line in tqdm.tqdm(fp):
            try:
                seed = int(line.split()[0])
                int_array = [int(x) for x in line.split()[1:]]
                if len(int_array) != 4:
                    continue
                output_dict[seed] = int_array
            except IndexError as e:
                continue
            for i in range(4):
                try:
                    coeff = int(line.split()[i+1])
                except IndexError as e:
                    break
                reverse_dict[i][coeff].append(seed)
    subprocess.run('cd ../chipwhisperer/victims/unpack_xof/ && make  CFLAGS_LAST=\'-fomit-frame-pointer -fwrapv\' OPT=3   PLATFORM="CW308_STM32F4" CRYPTO_TARGET=NONE', shell=True)
    scope = cw.scope()
    target = cw.target(scope, cw.targets.SimpleSerial2, flush_on_err=False)
    scope.default_setup()
    scope.adc.fifo_fill_mode = "normal"
    #scope.adc.samples = 20000
    print(scope)
    cw.program_target(scope, cw.programmers.STM32FProgrammer, "../chipwhisperer/victims/unpack_xof/unpack-CW308_STM32F4.hex")
    captures = []
    values = []
    for _ in tqdm.tqdm(range(3000)):
        for i in range(4):
            for label in [0]: 
                print(reverse_dict[i][label])
                input_seed = random.sample(reverse_dict[i][label],1)[0]
                capture = capture_value(scope, target, input_seed)
                captures.append(capture)
                values.append(output_dict[input_seed])
    count_other_values = 0
    for y_coeff in tqdm.tqdm(range(-92,93)):
        for i in range(4):  
            for seed in set(reverse_dict[i][y_coeff]):
                captures.append(capture_value(scope, target, seed))
                values.append(output_dict[seed])
                count_other_values += 1
    print("Other values captured", count_other_values)
    with open("captures_multiclass_xof.pickle", "wb") as fp:
        pickle.dump(captures, fp)
    with open("values_multiclass_xof.pickle", "wb") as fp:
        pickle.dump(values, fp)
        


if __name__ == "__main__":
    main()
