import numpy as np
import math
import time
def gelu(x):
    res = 0.5000*x*(1+ math.tanh(math.sqrt(2/math.pi)* (x+ 0.04471* x**3) ))
    return res
def bumbble_gelu(x):   

    seg4gelu = None
    if x <= -5.:
        seg4gelu = - 10e-5
    if -5 < x and x <= -1.97:
        seg4gelu = -0.5054031199708174 - 0.4222658115198386 * x  - 0.1180761295118195 * x**2 - 0.0110341340306157 * x ** 3  
    if -1.97 < x and x <= 3:
        seg4gelu =  0.0085263215410380 +  0.5 * x  +  0.360329269278962 * x**2 + .0 * x ** 3  -0.037688200365904 * x ** 4 + 0.0018067462606141  *x **6
    if x > 3:
        seg4gelu = x - 10e-5

    return seg4gelu

def our_gelu(x):
    seg5gelu = None
    if x <= -5.075:
        seg5gelu = 0
    if -5.075 < x and x <= -1.414: # poor
        seg5gelu = -0.568686678 -  0.529288810 * x  - 0.183509590* x**2 - 0.028070202 * x ** 3  -0.001597741 * x ** 4
    if -1.414 < x and x < 1.414: # good! 
        seg5gelu = 0.001193207 +  0.5 * x  + 0.385858026* x**2 + .0 * x ** 3  -0.045101361 * x ** 4
    if 1.414 <= x and x < 5.075: # bad
        seg5gelu = -0.438406187 + 1.340789252 * x - 0.087184212 * x ** 2 + 0.007334718 * x ** 3
    if x >= 5.075:
        seg5gelu =x 
    return seg5gelu


def Bolt_gelu(x):
    # x = truncate(x)

    c1 = 0.14439048359960427
    c2 = 0.7077117131613893
    c3 = 4.5702822654246535
    c4 = 8.15444702051307
    c5 = 16.382265425072532

    c1 = np.floor(c1 * 2**11)
    c2 = np.floor(c2 * 2**11)
    c3 = np.floor(c3 * 2**11)
    c4 = np.floor(c4 * 2**11)
    c5 = np.floor(c5 * 2**11)

    abs_x = np.abs(x)
    # y = truncate((truncate(c1 * abs_x, bits=12) - c2) * abs_x, bits=12) + c3
    # res = truncate((y + truncate(c1 * abs_x, bits=12) - c4) * y, bits=12) + c5 + 0.5 * x
    # res = truncate(res, bits=12)
    # res[x > 2.7] = x[x > 2.7]
    # res[x < -2.7] = 0

    temp_y = np.floor(c1 * abs_x / 2**11) - c2
    y = np.floor(temp_y * abs_x / 2**11) + c3
    temp_res = y + np.floor(c1 * abs_x / 2**11) - c4
    temp_res = temp_res * y
    res = np.floor(temp_res / 2**11) + c5 + x / 2

    res = np.floor(res)

    res[x > np.floor(2.7 * 2**11)] = x[x > np.floor(2.7 * 2**11)]
    res[x < np.floor(-2.7 * 2**11)] = 0
    return res



def truncate(x, bits=12):
    return np.floor(x * 2 ** bits) / 2 ** bits

def ULP_error(x, y, scale):
    fixed_x = x * 2 ** scale
    fixed_y = y * 2 ** scale
    
    return int((fixed_x - fixed_y) if (fixed_x - fixed_y)  > 0 else (fixed_y - fixed_x) )

def main():

    
    MAX_error = 0
    AVE_error = 0
    total_err = 0
    DIM = 128 * 3072
    
    time_a = 0
    time_b = 0
    
    random_vector = np.random.uniform(-6, 6, size=DIM)
    start = time.time()
    for i in random_vector:
        a= our_gelu(i)
    temp_a = time.time() - start
    
    start_1 = time.time()
    for i in random_vector:
        b = gelu(i), 12
    temp_b = time.time() - start_1
    
    for i in random_vector:
        c = Bolt_gelu(i)
    
    
    for i in random_vector:
        a= our_gelu(i)
        b = gelu(i)
        err = ULP_error(a, b, 12)
        total_err += err
        MAX_error = max(MAX_error, err)
        
    AVE_error = total_err / DIM  
      

    print('average ULP error',  AVE_error)
    print('Max ULP error', MAX_error)
    print('Number of test', DIM)
    print('time_our', temp_a)
    print('time_gelu', temp_b)
    
main()



