# Description

This is a list of Test preformed on each Gravimetric board after assembly.

# Table of References


# Table Of Contents:

1. Basics
2. Assembly check
3. IC Solder Test
4. TBD

## Basics

These tests are for an assembled RPUicp board 17341^0 which may be referred to as a Unit Under Test (UUT). If the UUT fails and can be reworked then do so, otherwise it needs to be scraped. 

**Warning: never use a soldering iron to rework ceramic capacitors due to the thermal shock.**
    
Items used for test.

TBD


## Assembly check

After assembly check the circuit carefully to make sure all parts are soldered and correct, the device marking is labeled on the schematic and assembly drawing.
    

## IC Solder Test

U3 and U4 are not yet populated. Check that a diode drop to 0V is present from a circuit board pad that is connected to each of the pins of U1 and U3 by measuring with a DMM's diode test set up for reversed polarity. Consult the schematic to determine which pins can be skipped (e.g. ground, power rail, ...). This board is not set up for In-Circuit Testing (ICT), I recomend automated optical inspection and then functional testing.

