#ifndef UISOUND_HPP
#define UISOUND_HPP
#include <stdint.h>
namespace ui {
class Sound {
public:
    static void sync();
    static unsigned volume();
    static bool enabled();
    static unsigned tone();
    static unsigned feedback();
    static void setVolume(unsigned value);
    static void toggle();
    static void cycleTone();
    static void cycleFeedback();
    static void test();
    static void touch();
private:
    static void apply();
};
}
#endif
