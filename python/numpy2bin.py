#!/usr/bin/env python3

import argparse
import struct
import sys

import numpy as np

def check_compatibility(x):
    problems = []
    if not x.flags['C_CONTIGUOUS']:
        problems.append('Wrong memory layout (C_CONTIGUOUS != True)\n{}' .format(x.flags))
    if x.dtype != np.float32:
        problems.append('Wrong dtype: {} != float32'.format(x.dtype))
    if x.strides[1] != 4:
        problems.append('Wrong stride: {} != 4'.format(x.strides[1]))
    return problems

def print_problems(problems):
    print('Reasons:' if len(problems) > 1 else 'Reason:')
    for i, p in enumerate(problems):
        print('[{}] {}'.format(i+1, p))

def write_slow(fp, x):
    for r in range(x.shape[0]):
        for c in range(x.shape[1]):
            f = struct.pack('f', x[r, c])
            fp.write(f)

def write_fast(fp, x):
    fp.write(x.data)

def numpy2bin(x, filename, force_slow=False):
    (rows, cols) = x.shape

    fp = open(filename, 'wb')
    header = struct.pack('4sfQQQQQQQ',
                         b'PSBD', # magic
                         1.0,     # version
                         64,      # hsize
                         4*cols,  # rlength
                         cols,    # vdim   
                         1,       # format 
                         0,       # tmp1
                         0,       # tmp2
                         0)       # tmp3
    fp.write(header)

    problems = check_compatibility(x)
    if problems:
        print('WARNING: Numpy array not compatible with PicSOM format, trying casting...')
        print_problems(problems)

        x = x.astype(np.float32)
        problems = check_compatibility(x)

        if problems:
            print('WARNING: Casted array also not compatible. Falling back to slow method.')
            print_problems(problems)
        else:
            print('Casted array is OK!')

    if problems or force_slow:
        write_slow(fp, x)
    else:
        write_fast(fp, x)

    fp.close()
    print("Wrote {}".format(filename))

#------------------------------------------------------------------------------

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('numpy_file', type=str, help='Input file as .npy')
    parser.add_argument('bin_file', type=str, help='Out file as .bin')
    parser.add_argument('--force_slow', action='store_true')
    args = parser.parse_args()

    print('Reading {}'.format(args.numpy_file))
    x = np.load(args.numpy_file)
    numpy2bin(x, args.bin_file, args.force_slow)

