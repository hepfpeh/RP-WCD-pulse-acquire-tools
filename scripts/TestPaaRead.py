#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Tue Mar 19 21:12:49 2019

@author: hepf
"""
import sys
import matplotlib.pyplot as plt
import matplotlib.lines as mlines
from paa01 import paaFile

if( len(sys.argv) < 2 ):
    print("No filename given")
    sys.exit()
    
DataFile = paaFile( sys.argv[1] )

print( DataFile.paaGetTextHeader(0) )

ps = DataFile.paaGetPulseSize()

pulse_data_t = list(range( ps ))
pulse_data_v = DataFile.paaGetPulse(10)
plt.plot(pulse_data_t, pulse_data_v)

tl = DataFile.paaGetThresholdLevel()
if( tl < 0):
    tly = 16384+tl
else:
    tly = tl
    
ax = plt.gca()
ax.add_line( mlines.Line2D([0,ps-1], [tly,tly] ) )

plt.show()