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
print( DataFile.paaGetTextHeader(1) )
print( DataFile.paaGetTextHeader(2) )
print( DataFile.paaGetTextHeader(3) )
print( DataFile.paaGetTextHeader(4) )


ps = DataFile.paaGetPulseSize()
pc = DataFile.paaGetPulseCount()
tl = DataFile.paaGetThresholdLevel()

print("Points per pulse: ", ps)
print("Pulses in this file: ", pc)
print("threshold level: ", tl)
print()

pulse_data_t = list(range( ps ))

while(1):
    try:
        pulse_number = int(input('Pulse number to plot: '))
    except ValueError:
        print("Not a number")
        sys.exit(0)
    except KeyboardInterrupt:
        print()
        sys.exit()
    if( ( pulse_number < 0 ) or (pulse_number > (pc-1) )):
        print("Error: Bad pulse number")
        sys.exit(0)
    pulse_data_v = DataFile.paaGetPulseRP(pulse_number)
    # For paa files generated from other sources than
    # pacq on a RedPitaya, use paaGetPulseRaw() instead.
    #
    #pulse_data_v = DataFile.paaGetPulseRaw(pulse_number)
    #
    
    plt.plot(pulse_data_t, pulse_data_v)
        
    ax = plt.gca()
    ax.add_line( mlines.Line2D([0,ps-1], [tl,tl] ) )
    
    plt.show()
