#!/usr/bin/env python3

import numpy as np
import struct

#------------------------------------------------------------------------------

def numpy2bin(a, filename):
    (rows, cols) = a.shape

    fp = open(filename, 'wb')
    header = struct.pack('4sfQQQQQQQ',
                         b'PSBD', # magic
                         1.0,     # version
                         64,      # hsize
                         4*cols,  # rlength
                         cols,    # vdim   
                         0,       # format 
                         0,       # tmp1
                         0,       # tmp2
                         0)       # tmp3
    fp.write(header)

    for r in range(rows):
        for c in range(cols):
            f = struct.pack('f', a[r, c])
            fp.write(f)

    fp.close()

#------------------------------------------------------------------------------

if __name__ == '__main__':
    a = np.array([[0, 1], [2, 3], [4,5]])
    numpy2bin(a, "out.bin")
    
