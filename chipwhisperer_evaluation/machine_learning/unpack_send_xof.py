#!/usr/bin/env python
# coding: utf-8
import collections
from itertools import count
import struct
import tqdm
import subprocess
import chipwhisperer as cw
import pickle
import collections
import random
import os
import train_classifier
import hashlib
import helpers
import binascii
import sys
import os
import signal
import json
import time


terminate_next_iter = False

def signal_handling(signum,frame):           
    global terminate_next_iter                         
    terminate_next_iter = True  

start_time = None

def LengthOfFile(f):
    """ Get the length of the file for a regular file (not a device file)"""
    currentPos=f.tell()
    f.seek(0, 2)          # move to end of file
    length = f.tell()     # get current position
    f.seek(currentPos, 0) # go back to where we started
    return length

def BytesRemaining(f,f_len):
    """ Get number of bytes left to read, where f_len is the length of the file (probably from f_len=LengthOfFile(f) )"""
    currentPos=f.tell()
    return f_len-currentPos

def BytesRemainingAndSize(f):
    """ Get number of bytes left to read for a regular file (not a device file), returns a tuple of the bytes remaining and the total length of the file
        If your code is going to be doing this alot then use LengthOfFile and  BytesRemaining instead of this function
    """
    global start_time
    if start_time is None:
        start_time = time.time()
    currentPos=f.tell()
    l=LengthOfFile(f)
    print("Bytes reamining", l-currentPos, "percent:", ((l-currentPos)/l)*100)
    if currentPos>0:
        time_taken = time.time()-start_time
        estimated_time_left = ((time_taken / (currentPos))*(l-currentPos))/3600
        if l == 0:
            seconds_per_percent = 0
        else:
            seconds_per_percent = time_taken/(((l-currentPos)/l)*100)
        print("ETA",estimated_time_left, "hours ", seconds_per_percent, "s/percent" )
    return l-currentPos,l

def capture_value(scope, target, classifier, in_dir):
    input_file = open("f{in_dir}/y_polys.bin", "rb")
    count_wrong = 0
    count_right = 0
    count_wrong_non_zero = 0
    overall_zero = 0
    overall_non_zero = 0
    file_length = LengthOfFile(input_file)
    #input_file.seek(LengthOfFile(input_file)-388685018, 0)
    count_poly = 0
    #os.remove("predicted_zero.txt")
    signal.signal(signal.SIGINT,signal_handling)
    try:
        with open("next_poly.json") as fp:
            start_at_poly = json.load(fp)["count_poly"]
    except FileNotFoundError:
        start_at_poly = 0
    count_poly = start_at_poly
    input_file.seek(count_poly*(64+2+4*256*4+4*256*1))
    while BytesRemainingAndSize(input_file)[0]>0:
        rhoprime, start_nonce, y_poly, could_be_zero = helpers.read_from_y_poly_file(input_file)
        if count_poly < start_at_poly:
            count_poly += 1
            continue
        #if y_poly[1][0] != 0:
        #    continue
        #print(rhoprime, start_nonce)        #target.reset_comms()
        #msg = binascii.unhexlify("0499B659E12E2BCF8B9425B758DBC7B77427F8C047C09DCF876D45DD09A5F09CDDBA3C3C862D1C9553BC4C970D79467D514E3E4807CA8CCB78A2DEB12CAFFD4B")#os.urandom(64)
        #rhoprime = msg
        print("Setting rhoprime", rhoprime)
        #target.send_cmd(0x02, 0x01, rhoprime)
        #target.simpleserial_wait_ack()
        #Do the following:
        #Extend rhoprime per xof into stream of 512 bytes
        #Send 8 bytes per polynomial iteration
        #Run on iteration of the unpack algorithm and predict -- skip if no polynomial


        print("Done")
        
        for poly_idx in range(4):
            #print("Found some poly", y_poly[x][0], "x", x)
            #print(y_poly)
            #try:

            nonce = start_nonce*4+poly_idx
            nonce_bytes = struct.pack("<H", nonce)
            buf = helpers.xof_shake256(rhoprime, nonce)
            for unpack_loop_idx in range(0, 256, 4):
                if not any([h!=0 for h in could_be_zero[poly_idx][unpack_loop_idx+0:unpack_loop_idx+4]]):
                    for i in range(4):
                        if y_poly[poly_idx][unpack_loop_idx+i] == 0:
                            pass #raise Exception("Could be zero is wrong")
                    buf = buf[9:]
                    continue

                #msg = os.urandom(64) # #os.urandom(4)b"a"*64
                target.flush()
                scope.arm()
                msg = buf
                print("Now sending msg", msg)
                target.send_cmd(0x01,0x01,msg[:9])
                buf = buf[9:]
                capture = scope.capture()
                print("Catpure timeout", capture)
                res = target.simpleserial_read("r",4)
                print("res", res)
                res = struct.unpack("<i", res)
                print("res", res, "coeff", y_poly[poly_idx][unpack_loop_idx+0])
                #return_value = target.simpleserial_wait_ack(timeout=10000000)
                print("Trig count", scope.adc.trig_count)
                traces = scope.get_last_trace()
                #return_value = target.read_cmd(cmd=None,pay_len=4,timeout=100000) #simpleserial_read("r", 4,timeout=100000)
                #print(return_value)
                #print(len(scope.get_last_trace_segmented()))
                #continue
                #print(struct.unpack("<i",return_value))
                #print("trig count", scope.adc.trig_count)
                #print(return_value)
                #traces = scope.get_last_trace_segmented()
                #if return_value is None:
                #    continue
                #print("traces len", len(traces))
                res = classifier.predict(traces[:548].reshape(1,-1))
                print(res)
                #res = res[0]
                print(res)
                print("Classifier done", file=sys.stderr)
                print("Result", res, file=sys.stderr)
                #res = [0]

                for coeff_idx in range(4):
                    if could_be_zero[poly_idx][unpack_loop_idx+coeff_idx]==0 and y_poly[poly_idx][unpack_loop_idx+coeff_idx] == 0:
                        raise Exception("could be zero is wrong!")
                    if could_be_zero[poly_idx][unpack_loop_idx+coeff_idx]==0:
                        continue
                    

                    prediction = res[coeff_idx][0]
                    print(prediction)
                    if prediction < 0.95:
                        prediction = 1
                    else:
                        prediction = 0
                    if prediction == 0:
                        with open("predicted_zero.txt", "a+") as fp:
                            fp.write(f"{count_poly};{poly_idx};{unpack_loop_idx+coeff_idx}\n")

                    if prediction == 0 and y_poly[poly_idx][unpack_loop_idx+coeff_idx] != 0:
                        print(f"Wrong at idx {coeff_idx}")
                        count_wrong += 1
                    elif prediction == 0 and y_poly[poly_idx][unpack_loop_idx+coeff_idx] == 0:
                        print(f" Right at {coeff_idx}")
                        count_right += 1
                    elif prediction == 1 and y_poly[poly_idx][unpack_loop_idx+coeff_idx] == 0:
                        print(f"Wrongly predicted non-zero at idx {coeff_idx}")
                        count_wrong_non_zero += 1
                    if y_poly[poly_idx][unpack_loop_idx+coeff_idx] == 0:
                        overall_zero += 1
                    else:
                        overall_non_zero += 1
                    print(f"Wrong: {count_wrong}/{overall_non_zero}")
                    print(f"Right: {count_right}/{overall_zero}")
                    print(f"count_wrong_non_zero {count_wrong_non_zero}")

            #except Exception as e:
            #    print("Exception", e)
            #    print("Res", res)
            #    raise e#continue
            #exit(0)
        count_poly += 1
        if terminate_next_iter == True:
            print(f"Interrupted, next poly: {count_poly}")
            with open("next_poly.json", "w") as fp:
                json.dump({"count_poly":count_poly},fp)
            exit(0)

        #exit(0)
    return scope.get_last_trace()[:scope.adc.trig_count]

def main():
    classifier = train_classifier.PolyClassifier.from_saved()
    subprocess.run('cd ../chipwhisperer/victims/unpack_xof/ && make CFLAGS_LAST=\'-fomit-frame-pointer -fwrapv\' OPT=3 PLATFORM="CW308_STM32F4" CRYPTO_TARGET=NONE', shell=True)
    scope = cw.scope()
    target = cw.target(scope, cw.targets.SimpleSerial2, flush_on_err=True)
    scope.default_setup()
    scope.adc.fifo_fill_mode = "normal"
    cw.program_target(scope, cw.programmers.STM32FProgrammer, "../chipwhisperer/victims/unpack_xof/unpack-CW308_STM32F4.hex")
    capture_value(scope, target, classifier)

        


if __name__ == "__main__":
    main()
