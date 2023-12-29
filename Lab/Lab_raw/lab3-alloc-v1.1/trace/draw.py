from cProfile import label
from calendar import day_abbr
import csv
import numpy as np
from fileinput import filename
from traceback import print_tb
import matplotlib.pyplot as plt
from numpy import array
import pandas as pd
filename_f = 'first_fit_mem_util.csv'
filename_b = "best_fit_mem_util.csv"
filename_m = 'mem_util.csv'
data_f=pd.read_csv(filename_f,sep=',')
data_b=pd.read_csv(filename_b,sep=',')
# data=pd.read_csv(filename_m,sep=',')

data_f=np.array(data_f)
data_b=np.array(data_b)
# data = np.array(data)

xf=data_f[:,0]
yf=data_f[:,1]
xb=data_b[:,0]
yb=data_b[:,1]
# x=data[:,0]
# y=data[:,1]

plt.plot(xf,yf,'r--',label='first fit',linewidth=0.5)
plt.plot(xb,yb,'y-',label='best fit',linewidth=1)
plt.xlabel("time(s)")
plt.ylabel('percent')
plt.legend()
plt.savefig("./fig.png")
plt.show()

