# Graviton

## Overview

This firmware is not done; it is aimed at helping to understand the gravimetric calibration method for liquid metering devices. I will deal with integers on the MCU so for fun I am going to call this Graviton. It will be an interactive command-line program that demonstrates control options of this board to do Gravimetric calibration, the program source documents how that is achieved. 

Water is diverted into a tank for a time determined by the ICP3 (START) and ICP4 (STOP) events. The weight of the collected water is measured. The pulses from a flow meter are input with ICP1 between the START and STOP time to calculate a meter constant (the meters K factor). 

A program running on a host computer will take the integers (e.g., pulse counts, timing counts, timing events) and calculate floating-point values (e.g., the meter K factor). The AVR's single time-domain keeps the relation between events, counts, and control simple; it is accurate to within one crystal clock. A high-speed MCU that runs with a clock derived from a PPL and input pins in their time domain is very likely less precise. In this occasion, the simple AVR time-domain means a more repeatable measurement with accuracy that is wholly dependent on the crystal accuracy without any wordsmithing (e.g., no Double-Timing Pulse Interpolation.) Once the precision data is collected, it can be passed to the sausage factory that is a modern computer for processing and networking. An FPGA could do this also, but there is several MCU's that have surfactant capture hardware to do the task.


## Scale

There is a range of industrial scales that have RS232; for the most part, they will print an output when a button is pushed or take a command (e.g., 'S' is often a request for weighing after stable). Use a [shifter] to convert RS232 to CMOS logic for the AVR. The Avery Weight-Tronix [BSQ Bench] is the calibrated gold option.

[shifter]: https://www.sparkfun.com/products/449
[BSQ Bench]: https://www.averyweigh-tronix.com/globalassets/products/bench-scales/bsq/bsq_spec_501488v3.pdf

Another option is to build a scale with load cells and an IC like the NAU7802 or HX711, but the truth is if it is not calibrated, then this exercise is absurd.

For development I will input data from another serial device (e.g., USBuart or a ICSP /w an R-Pi).


## Partial Pulse

An incomplete pulse time will be found between the Start and Stop events. Keep track of the first and last pulse from the Flowmeter and use that to figure out how much is needed to finish the time between Start and Stop. Now that I have seen the workings, it seems entirely straightforward, but I guess that is the nature of things.

```
Start = ICP3
Stop = ICP4
FirstFTafterICP3 = ICP1[0]
LastFTbeforICP4 = ICP1[Last-1]
# the Partial Pulse
Partial = (Stop - Start) - (LastFTbeforICP4 - FirstFTafterICP3)
# the average pulse is a floating point
FTrate = 1.0 * (LastFTbeforICP4 - FirstFTafterICP3) / Last
Weight = read_stable_scale()
WtPerFT = Weight / (Last + (FTrate / Partial) )
```


## Volume

Convert the weight per flow pulse into volume. 

```
# the nominal value is in g/cm3
density_of_water = 0.997239
density_of_air = 0.001119
```

Type II Electronic balance weights density is approximately 7.84g/cm3. This application involves the calibration of a Class II balance with an ASTM Class 4, 5 and 6 weight(s) manufactured in accordance with ASTM E 617-97 specification and tolerances.

```
density_correction = (1+ density_of_air*( (1/density_of_water) - (1/7.84)))
water_mass_collected = water_weight * density_correction
volume_start_to_stop = water_mass_collected / density_of_water
```

The instrument to be proven may have other correction factors that need to be applied to the volume, for example a volume prover will need corrections for both temperature and pressure.

