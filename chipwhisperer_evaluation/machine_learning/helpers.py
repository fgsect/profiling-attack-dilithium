import struct
import binascii
import collections
import scipy
import scipy.stats
import numpy

TRACE_LENGTH = 316

K = 39
min_coeff = -2
max_coeff = +2

from spongeshaker.sha3 import sha3_256, shake256

def xof_shake256(rhoprime, nonce):
    t = bytearray(2)
    t[0] = nonce
    t[1] = (numpy.uint8(nonce >> 8))
    md = shake256()
    md.update(rhoprime)
    md.update(t)
    return md.extract(576)

def randombytes(seed):
    buf = bytearray(8)
    for i in range(8):
        buf[i] = (numpy.uint8(seed >> 8  * i))
    m = hashlib.shake_128()
    m.update(bytes(buf))
    return m.digest(64)

def calculate_convolution(pmf1, pmf2):
    ret_pmf = collections.defaultdict(int,{x:0 for x in range(K*min_coeff, K*max_coeff+1)})
    for z in range(K*min_coeff, K*max_coeff+1):
        for k in range(K*min_coeff, K*max_coeff+1):
            ret_pmf[z] += pmf1[k] * pmf2[z-k]
    return ret_pmf

def y_cs_pmf_convolution(y_pmf, cs_pmf):
    ret_pmf = collections.defaultdict(int,{x:0 for x in range(K*min_coeff, K*max_coeff+1)})
    for z in range(-131072-78, 131072+79):
        for k in range(-78, +79):
            ret_pmf[z] += y_pmf[k] * cs_pmf[z-k]
    return ret_pmf

def calculate_y_given_z_prob():
    basic_pmf = collections.defaultdict(int, {x:scipy.stats.randint.pmf(x, min_coeff, max_coeff+1) for x in range(K*min_coeff, K*max_coeff+1)})
    current_pmf = basic_pmf
    for _ in range(K-1):
        current_pmf = calculate_convolution(current_pmf, basic_pmf)
    cs_pmf = current_pmf
    y_pmf = {x: 1/(92*2) for x in range(-92,+93)}
    z_pmf = y_cs_pmf_convolution(y_pmf, cs_pmf) # prob(z=i | y!=0)
    y_zero_given_z = collections.defaultdict(int)
    for i in range(-14,15):
        y_zero_given_z[i] = (cs_pmf[i]*y_pmf[0])/z_pmf[i]
    final_pmf = y_zero_given_z
    #final_pmf = {k:v for (k,v) in final_pmf.items() if v != 0}
    return final_pmf

#y_zero_given_z_pmf = calculate_y_given_z_prob()

def read_from_z_file(file_handle):
    z_polys = []
    for _ in range(4):
        current_poly = []
        for _ in range(256):
            coefficient_bytes = file_handle.read(4)
            coefficient = struct.unpack("<i", coefficient_bytes)[0] #signed integer
            current_poly.append(coefficient)
        z_polys.append(current_poly)
    return z_polys

def read_from_y_poly_file(file_handle):
    #Performs the following reads:
    #64+2+4*256*4+4*256*1
    rhoprime = file_handle.read(64)
    nonce_bytes = file_handle.read(2)
    nonce = struct.unpack("<H",nonce_bytes)[0] #unsigned short+
    print(nonce_bytes)
    y_polys = []
    for _ in range(4):
        current_poly = []
        for _ in range(256):
            coefficient_bytes = file_handle.read(4)
            coefficient = struct.unpack("<i", coefficient_bytes)[0] #signed integer
            current_poly.append(coefficient)
        y_polys.append(current_poly)
    could_be_zero = [] # Could the coefficient at position i,j be zero?
    for _ in range(4):
        current_poly_zero = []
        for _ in range(256):
            coefficient_bytes = file_handle.read(1)
            coefficient_could_be_zero = struct.unpack("<B", coefficient_bytes)[0]
            current_poly_zero.append(coefficient_could_be_zero)
        could_be_zero.append(current_poly_zero)
    return (rhoprime, nonce, y_polys, could_be_zero)




def get_hamming_weight(x):
    return sum( [x&(1<<i)>0 for i in range(32)] )


def get_num_traces_segmented(num_traces, coeff, scope, target, verbose=False):
    all_captures = []
    done = False
    count = 0 
    max_fifo_size = 24400#scope.adc.oa.hwMaxSamples
    #print("max fifo size", max_fifo_size)
    segments_to_capture = int(round(max_fifo_size / 316 + 1))
    while not done:
        #print("Segments to capture", segments_to_capture)
        scope.arm()
        msg = struct.pack("<h", segments_to_capture)
        msg += struct.pack("<i", coeff)
        #print("Sending", binascii.hexlify(msg))
        target.simpleserial_write("p",msg)
        scope.capture_segmented()
        target.simpleserial_wait_ack(timeout=100000)
        traces = scope.get_last_trace_segmented()
        target.flush()
        #print("Traces", traces)
        #print("Len traces", len(traces))
        #print("Len traces 1", len(traces[0]))
        all_captures += list(traces)
        #print("len all_captures", len(all_captures))
        #target.flush()
        if len(all_captures) >= num_traces:
            done = True
    return all_captures

def main():
    rhoprime = binascii.unhexlify("BC9DD52C5A93479D87476CFCA466B6A6823458D873EDAC24BCB2EC87A3A08AE9EEE0872BE80019D7EE0A8AF12F963F051AAC551EBBACF12836E11DC50AA1354A")
    nonce = 0
    print(binascii.hexlify(xof_shake256(rhoprime, nonce)))

if __name__ == "__main__":
    main()
