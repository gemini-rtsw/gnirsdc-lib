#!/usr/bin/python3.9

import libgnirsioc
import time

libgnirsioc.setGlobalDebug(True)


c = libgnirsioc.controllerInterface(".",".")

#	int setExposure(double fowlserSamples, double adcSamples, double exposureTime, int coadds, int skip_frames);

c.setExposure(10, 1, 20, 1, 5)
c.startExposureBlock(0,0,False)

