#pragma once
#include "bakkesmod/plugin/bakkesmodplugin.h"
#include "bakkesmod/plugin/pluginwindow.h"
#include "bakkesmod/plugin/pluginsettingswindow.h"
#include "bakkesmod/wrappers/GameWrapper.h"
#include "bakkesmod/wrappers/WrapperStructs.h"
#include "bakkesmod/wrappers/canvaswrapper.h"
#include "bakkesmod/wrappers/GuiManagerWrapper.h"
#include "bakkesmod/wrappers/PluginManagerWrapper.h"
#include "bakkesmod/wrappers/CVarManagerWrapper.h"
#include <memory>
#include <string>
#include <sstream>
#include <iomanip>

class FirstTouchTrainer : public BakkesMod::Plugin::BakkesModPlugin
{
public:
    virtual void onLoad();
    virtual void onUnload();

    void InitializeVariables();
    void RegisterCvars();
    void RegisterEvents();
    void RegisterRenderCallbacks();

    void RenderFTT(CanvasWrapper canvas);
    void RenderSessionTimer(CanvasWrapper canvas);
    void RenderTouchZone(CanvasWrapper canvas);

    struct FirstTouchResult {
        float velocityDifference;
        float ballPosX;
        float ballPosY;
        float ballPosZ;
        float zVelDif;
    };

    FirstTouchResult firstTouchTrainer();
    int checkConditions();
    void OnFreeplayLoad(std::string eventName);
    void OnFreeplayDestroy(std::string eventName);

private:
    std::shared_ptr<CVarManagerWrapper> _globalCvarManager;
    float sessionStartTime;
    bool sessionRunning;
    std::shared_ptr<bool> bPerformanceMode;

    std::shared_ptr<int> index;
    std::shared_ptr<int> calculationIndex;
    std::shared_ptr<LinearColor> gDrawnColor;
    std::shared_ptr<float> bScreenSizeX;
    std::shared_ptr<float> bScreenSizeY;
    std::shared_ptr<bool> bEnabled;
    std::shared_ptr<bool> bXYZEnabled;
    std::shared_ptr<int> tXPos;
    std::shared_ptr<int> tYPos;
    std::shared_ptr<LinearColor> cGoodColor;
    std::shared_ptr<LinearColor> cAlrightColor;
    std::shared_ptr<LinearColor> cBadColor;
    std::shared_ptr<int> tTextSize;
    std::shared_ptr<bool> tDropShadow;
    std::shared_ptr<bool> sSessionTimerEnabled;
    std::shared_ptr<int> sSessionTimerX;
    std::shared_ptr<int> sSessionTimerY;
    std::shared_ptr<LinearColor> sSessionTimerColor;
    std::shared_ptr<bool> zTouchZoneEnabled;
    std::shared_ptr<bool> zTouchZoneCircleEnabled;
    std::shared_ptr<float> zTouchZoneCircleRadius;
    std::shared_ptr<int> zTouchZoneCircleThicc;
    std::shared_ptr<bool> zTouchZoneSphereEnabled;
    std::shared_ptr<float> zTouchZoneSphereRadius;
    std::shared_ptr<LinearColor> zTouchZoneSphereColor;
    std::shared_ptr<bool> zTouchZoneMatchColor;
    std::shared_ptr<bool> zTouchZoneSphereMatchColor;
    std::shared_ptr<LinearColor> zTouchZoneColor;
};