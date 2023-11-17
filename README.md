# Arduino-Apollo3ADC
An Arduino library to utilize more of the Apollo3 ADC functionality then available via the base Arduino API. 
This is a hardware specific library used with boards built on the Ambiq Apoll3 Blue MCU.

Boards that use this chip:
  * [Sparkfun Artemis](https://www.sparkfun.com/artemis)
  * [Apollo3 Blue Evaluation Board](https://www.ambiq.top/en/apollo3-blue-soc-eval-board)

If you use PlatformIO you will need to install the [platform-apollo3blue](https://github.com/nigelb/platform-apollo3blue) platform.


This library allows you to:

* Change the reference used by the ADC
* Change the clock used by the ADC
* Set the precision of the ADC to one of: 8 bit, 10 bit, 12 bit, 14 bit.
* Utilise the builtin hardware averaging, to average any of the following number of samples: 1, 2, 4, 8, 16, 32, 64, 128

See the [Hardware Averaging](examples/HardwareAveraging/HardwareAveraging.ino) example.
