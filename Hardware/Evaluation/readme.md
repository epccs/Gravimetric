# Description

This shows the setup and method used for evaluation of Gravimetric.

# Table of References


# Table Of Contents:

1. ^0 Mockup


## ^0 Mockup

Parts are not soldered, they are just resting in place, the headers are in separate BOM's and not included unless specified (they are expensive but in some cases worth the cost).

![MockupWithVertRPiZero](./17341^0_MockupWithVertRPiZero.jpg "Mockup With Vert R-Pi Zero")

With Horizontal Raspberry Pi Zero

![MockupWithHorzRPiZero](./17341^0_MockupWithHorzRPiZero.jpg "Mockup With Horz R-Pi Zero")

With Horizontal Raspberry Pi 3B+ (note the POL is missing)

![MockupWithRPi3B+](./17341^0_MockupWithRPi3B+_over.jpg "Mockup With R-Pi 3B+")

side view

![MockupWithRPi3B+_side](./17341^0_MockupWithRPi3B+_side.jpg "Mockup With R-Pi 3B+ Side")

The fit is surprisingly good. The edge of the ethernet connector is over the Gravimetric board just enough. The ethernet connector is resting on the plastic DIN rail clip. I suspect that an additional DIN clip could have some hook and loop (Velcro) placed on it and the top of the Ethernet header to fasten the board. One thing to note is that the screw head holding the Gravimetric board to the DIN rail clip needs some filing, so its edge does not go over the side of the Gravimetric board.

The computing power of a R-Pi 3 B+ is significant. It has a quad core Cortex-A53 (ARMv8) 64-bit SoC running at 1.4GHz. Unfortunately it needs 2.5A at 5V, and I have yet to find an option I like for the POL, I want it to turn 7V thru 36V into 5V@3A.

The R-Pi has a lot of approval certificates now days, so it is a choice for areas that need that sort of stuff.

https://www.raspberrypi.org/documentation/hardware/raspberrypi/conformity.md