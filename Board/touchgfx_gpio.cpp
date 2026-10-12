/* DK instrumentation pins are assigned to panel LEDs on the custom PCB. */
#include <touchgfx/hal/GPIO.hpp>
using namespace touchgfx;
void GPIO::init() {}
void GPIO::set(GPIO_ID) {}
void GPIO::clear(GPIO_ID) {}
void GPIO::toggle(GPIO_ID) {}
bool GPIO::get(GPIO_ID) { return false; }
