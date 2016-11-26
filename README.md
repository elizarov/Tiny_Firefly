Tiny Firefly
============

Tiny Firefly with ATtiny13a, LED and battery.

Parts and tools
---------------

* ATTINY13A-PU chip (8-pin DIP case)
  http://www.mouser.com/ProductDetail/Atmel/ATTINY13A-PU

* LED medium size ~2.0V ~10mA, any vendor, any color, for example:
  Kingbright L-53SRD-E (red), 
  Kingbright L-53SYD (yellow), or  
  Kingbright L-7113GD (green) 

* CR2032 3V coin-cell battery
  https://www.sparkfun.com/products/338

* CR2032 battery holder, like this one
  https://www.sparkfun.com/products/783

* Some kind of AVR programmer. Tiny AVR Programmer is recommended
  https://www.sparkfun.com/products/11801

* Breadboard for testing. Small one is recomended
  https://www.sparkfun.com/products/12043
  https://www.adafruit.com/product/65

* Soldering tools (solder iron, solder, small pliers and cutters/snippers)

How to
------

* Download AVRDUDE programmer at
  http://savannah.nongnu.org/projects/avrdude
  You can also use AVRDUDE that is coming as a part of Arduio installation.

* Verify your chip / set fuse

```
avrdude -c usbtiny -p t13 -v -B 250
```

Should report and the end

```
avrdude.exe: safemode: Fuses OK (E:FF, H:FF, L:7B)
                                               ^^
```

* Set fuses (TBD)

* Upload code

```
avrdude -c usbtiny -p t13 -B 250 -U flash:w:Release/Tiny_Firefly.hex:i
```

* Verify with protoboard (TBD)

* Solder (TBD)
