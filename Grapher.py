#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Mon Apr 23 17:15:43 2018

@author: sms1198
"""

import matplotlib.pyplot as plt


fp = open("CWND.csv")
congw = []
time = []


for n in fp:
    n = n.strip().split(',')
    time.append(int(n[0]))
    congw.append(int(n[1]))



fig = plt.figure(figsize=(30, 8))
ax = fig.add_subplot(111)
ax.plot(time,congw)

plt.xlabel('Time (milli sec, unix epoch time)', fontsize=20)
plt.ylabel('Congestion Window', fontsize=20)

plt.savefig("Congestion Window Graph with time.pdf" , dpi=350)

plt.show()
fp.close()