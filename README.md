# Davis Wind Rain Gauge I2C interface
Davis wind vane, anemometer and rain gauge I2C slave interface with the rp2040 (raspberry pi pico).

While hooking my Davis wind vane, anemometer and rain gauge to another-weather-station-project built with a raspberry pi zero, I stumbled into these issues:
* wind vane is analog, so an ADC is needed
* anemometer and rain gauge needs interrupt-driven GPIOs to properly handle the "clicks" from the sensor, and the raspberry hasn't a decent IRQ handling

So what? Since I2C is already used for other sensors (temp, pm25, whatever) why no "convert" the Davis stuff to I2C ? 
Here enters the pico, which is also an execuse to play with the pico-sdk.

## Low power stuff
Since the whole system will be running from battery, I tried to reduce the power consumption of the pico, which normally draws ~23mA.
By reducing clock speeds to 12Mhz (who needs 133Mhz???) for both ADC and CPU this little gadget now consumes a little less than 6mA which is ok for me,
without having to dig into sleep modes.

Initial experiments with sleep gave me some issues with I2C because the I2C interrupts where disabled. At same time unmasking them caused the pico 
to not really sleep because of the I2C bus activity (there're other sensors on it). So the solution was to add another wire to signal the pico to wake up,
perform I2C exchanges and send back to sleep... too complicated, that's why I choose to lower the clocks and disable not needed ones.

## Notes
Please note that I'm neither a C/C++ dev nor an embedded developer, just playing around.

## Sources, external links, etc
[Davis anemometer on arduino](http://cactus.io/hookups/weather/anemometer/davis/hookup-arduino-to-davis-anemometer) Davis wind sensor schematics, example arduino code

[RP2040 Datasheet](https://datasheets.raspberrypi.com/rp2040/rp2040-datasheet.pdf)

[Pico SDK docs](https://www.raspberrypi.com/documentation/pico-sdk)

[Pico Examples](https://github.com/raspberrypi/pico-examples) Examples for several things, including I2C slave, sleep, etc

[Pico Extras](https://github.com/raspberrypi/pico-extras) Libs not yet in SDK, like sleep, clocks stuff, etc

[Davis derived variables application note](https://support.davisinstruments.com/article/igpcv664kz-app-notes-derived-variables-in-davis-weather-products) Several derived variables, including how they calculate rain rate
