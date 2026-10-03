#ifndef SCREENAPPEARANCEVIEW_HPP
#define SCREENAPPEARANCEVIEW_HPP

#include <gui_generated/screenappearance_screen/ScreenAppearanceViewBase.hpp>
#include <gui/screenappearance_screen/ScreenAppearancePresenter.hpp>

class ScreenAppearanceView : public ScreenAppearanceViewBase
{
public:
    void setupTheme();
    ScreenAppearanceView();
    virtual ~ScreenAppearanceView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
    virtual void allOff();
    virtual void setLightTheme();
    virtual void setDarkTheme();
    virtual void setBlueAccent();
    virtual void setTealAccent();
    virtual void setVioletAccent();
    virtual void setAmberAccent();
protected:
    void refreshThemeControls();
};

#endif // SCREENAPPEARANCEVIEW_HPP
