#!/usr/bin/env python
# -*- coding: utf-8 -*-
import numpy as np
import numpy.polynomial.polynomial as poly
from numpy import genfromtxt
import matplotlib.pyplot as plt

my_data = genfromtxt('highest.csv', delimiter=',')
data = my_data[1:].transpose()
trade_count = data[0]
pl = data[1]
x=data[2:]


x_new = np.array(np.random.rand(100), 'float')
x_size =len(x)
colors=['blue', 'orange', 'red', 'yellow', 'cyan', 'purple','green','black','#ffbbfe','#cc02a0']
fig1 = plt.figure()

#x1.scatter(pl, x[0], facecolors='None')
for i in range(len(x)):
    ax1 = fig1.add_subplot(x_size, 1, i+1)
    coefs = poly.polyfit( x[i], pl, 3)
    ffit = poly.Polynomial(coefs)
    val = ffit(x_new)
    #ax1.hist(ffit(x_new), 1, facecolor='yellow')
    ax1.scatter( x_new, val, facecolors='yellow')
    ax1.plot(x_new, ffit(x_new), '-')
plt.show()
