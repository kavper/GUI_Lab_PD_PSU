#ifndef SCREENSETTINGSVIEW_HPP
#define SCREENSETTINGSVIEW_HPP

#include <gui_generated/screensettings_screen/ScreenSettingsViewBase.hpp>
#include <gui/screensettings_screen/ScreenSettingsPresenter.hpp>

class ScreenSettingsView : public ScreenSettingsViewBase
{
public:
    void setupTheme();
    ScreenSettingsView();
    virtual ~ScreenSettingsView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
    virtual void allOff();
protected:
};

#endif // SCREENSETTINGSVIEW_HPP
