.. RPi_monitor documentation master file, created by
   sphinx-quickstart on Tue Feb 26 09:36:40 2019.
   You can adapt this file completely to your liking, but it should at least
   contain the root `toctree` directive.

Incuvers telemetric chamber documentation
=========================================


The Incuvers telemetric chambers have two basic components:
a bare-metal environmental control unit based on an Arduino,
and a high level programming interface based on a Raspberry-Pi.

If used as a regular `dumb` incubator, one does not need to use the Raspberry-Pi interface, however this will severely limit the capacities.
The Raspberry-Pi allows to implement time-based environmental programs, logging, time-lapse imaging, real-time imaging and even connects to the cloud via your local wifi or wired network for cloud-based controlls/monitoring (by logging on the Incuvers app with desktop/tablet/mobile phone).




Arduino
=======
.. toctree::
    :maxdepth: 2

    arduino.rst

Raspberry Pi
============
.. toctree::
    :maxdepth: 2

    rpi.rst
