#ifndef FRONTENDAPPLICATION_HPP
#define FRONTENDAPPLICATION_HPP

#include <gui_generated/common/FrontendApplicationBase.hpp>

class FrontendHeap;

using namespace touchgfx;

class FrontendApplication : public FrontendApplicationBase
{
public:
    FrontendApplication(Model& m, FrontendHeap& heap);
    virtual ~FrontendApplication() { }

    virtual void handleTickEvent()
    {
        model.tick();
        FrontendApplicationBase::handleTickEvent();
    }
    // Wipe transitions draw only the newly revealed strip. Telemetry must not
    // invalidate widgets outside it while the previous screen is still visible.
    bool isScreenTransitionActive() const
    {
        return currentTransition && !currentTransition->isDone();
    }
private:
};

#endif // FRONTENDAPPLICATION_HPP
