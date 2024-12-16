#include "pch.h"
#include "FirstTouchTrainer.h"

BAKKESMOD_PLUGIN(FirstTouchTrainer, "First Touch Trainer", plugin_version, PLUGINTYPE_FREEPLAY)

std::shared_ptr<CVarManagerWrapper> _globalCvarManager;
float sessionStartTime = 0.0f; // Store start time
bool sessionRunning = false;
std::shared_ptr<bool> bPerformanceMode;

// Helper Functions
float GetMagnitude(float x, float y) {
    return std::sqrt((x * x) + (y * y));
}

std::string ToStringPrecision(float value, int precision) {
    std::ostringstream out;
    out << std::fixed << std::setprecision(precision) << value;
    return out.str();
}

// Plugin Lifecycle
void FirstTouchTrainer::onLoad() {
    _globalCvarManager = cvarManager;

    InitializeVariables();
    RegisterCvars();
    RegisterEvents();
    RegisterRenderCallbacks();
}

void FirstTouchTrainer::onUnload() {
    // Perform cleanup if necessary
}

// Initialization
void FirstTouchTrainer::InitializeVariables() {
    index = std::make_shared<int>(0);
    calculationIndex = std::make_shared<int>(0);

    gDrawnColor = std::make_shared<LinearColor>();

    Vector2 screenSize = gameWrapper->GetScreenSize();
    bScreenSizeX = std::make_shared<float>(screenSize.X);
    bScreenSizeY = std::make_shared<float>(screenSize.Y);

    bEnabled = std::make_shared<bool>(false);
    bXYZEnabled = std::make_shared<bool>(false);

    tXPos = std::make_shared<int>(0);
    tYPos = std::make_shared<int>(0);

    cGoodColor = std::make_shared<LinearColor>();
    cAlrightColor = std::make_shared<LinearColor>();
    cBadColor = std::make_shared<LinearColor>();

    tTextSize = std::make_shared<int>(0);
    tDropShadow = std::make_shared<bool>(false);

    sSessionTimerEnabled = std::make_shared<bool>(false);
    sSessionTimerX = std::make_shared<int>(0);
    sSessionTimerY = std::make_shared<int>(0);
    sSessionTimerColor = std::make_shared<LinearColor>();

    zTouchZoneEnabled = std::make_shared<bool>(false);
    zTouchZoneCircleEnabled = std::make_shared<bool>(false);
    zTouchZoneCircleRadius = std::make_shared<float>(0);
    zTouchZoneCircleThicc = std::make_shared<int>(2); // Direct value for thickness
    zTouchZoneSphereEnabled = std::make_shared<bool>(false);
    zTouchZoneSphereRadius = std::make_shared<float>(0);
    zTouchZoneSphereColor = std::make_shared<LinearColor>();
    zTouchZoneMatchColor = std::make_shared<bool>(false);
    zTouchZoneSphereMatchColor = std::make_shared<bool>(false);
    zTouchZoneColor = std::make_shared<LinearColor>();

    bPerformanceMode = std::make_shared<bool>(false);
}

void FirstTouchTrainer::RegisterCvars() {
    cvarManager->registerCvar("FTT_Enable", "0", "Show First Touch Trainer", true, true, 0, true, 1, true).bindTo(bEnabled);
    cvarManager->registerCvar("FTT_XYZEnabled", "0", "Calculate speed differential with XYZ Velocity", true, true, 0, true, 1, true).bindTo(bXYZEnabled);

    float speedDefaultX = *bScreenSizeX * 0.25f;
    float speedDefaultY = *bScreenSizeY * 0.25f;
    cvarManager->registerCvar("FTT_X_Position", std::to_string(speedDefaultX), "Change Text X Position", true).bindTo(tXPos);
    cvarManager->registerCvar("FTT_Y_Position", std::to_string(speedDefaultY), "Change Text Y Position", true).bindTo(tYPos);

    cvarManager->registerCvar("FTT_Good_Range", "#00FF00", "Good Range Color", true).bindTo(cGoodColor);
    cvarManager->registerCvar("FTT_Alright_Range", "#FFFF00", "Alright Range Color", true).bindTo(cAlrightColor);
    cvarManager->registerCvar("FTT_Bad_Range", "#FF0000", "Bad Range Color", true).bindTo(cBadColor);

    cvarManager->registerCvar("FTT_Text_Size", "3", "Change Text Size", true, true, 1, true, 10, true).bindTo(tTextSize);
    cvarManager->registerCvar("FTT_Shadow", "1", "Enable text drop shadows", true, true, 0, true, 1, true).bindTo(tDropShadow);

    cvarManager->registerCvar("FTT_SessionTimer", "0", "Enable session timer", true, true, 0, true, 1, true).bindTo(sSessionTimerEnabled);
    cvarManager->registerCvar("FTT_SessionTimerColor", "#FFFFFF", "Session Timer Color", true).bindTo(sSessionTimerColor);

    cvarManager->registerCvar("FTT_TouchZoneEnabled", "0", "Enable Touch Zone", true, true, 0, true, 1, true).bindTo(zTouchZoneEnabled);
    cvarManager->registerCvar("FTT_TouchZoneColor", "#FF0000FF", "Touch Zone Color", true).bindTo(zTouchZoneColor);

    cvarManager->registerCvar("FTT_PerformanceMode", "0", "Enable performance mode (reduces visual effects)", true).bindTo(bPerformanceMode);
}

void FirstTouchTrainer::RegisterEvents() {
    gameWrapper->HookEvent("Function TAGame.Mutator_Freeplay_TA.Init", bind(&FirstTouchTrainer::OnFreeplayLoad, this, std::placeholders::_1));
    gameWrapper->HookEvent("Function TAGame.GameEvent_Soccar_TA.Destroyed", bind(&FirstTouchTrainer::OnFreeplayDestroy, this, std::placeholders::_1));
}

void FirstTouchTrainer::RegisterRenderCallbacks() {
    gameWrapper->RegisterDrawable([this](CanvasWrapper canvas) {
        if (*bEnabled) {
            RenderFTT(canvas);
        }
        if (*sSessionTimerEnabled) {
            RenderSessionTimer(canvas);
        }
        if (*zTouchZoneEnabled || *zTouchZoneSphereEnabled) {
            RenderTouchZone(canvas);
        }
    });
}

// Logic
struct FirstTouchResult {
    float velocityDifference;
    float ballPosX;
    float ballPosY;
    float ballPosZ;
    float zVelDif;
};

FirstTouchResult FirstTouchTrainer::firstTouchTrainer() {
    if (checkConditions() == 0) {
        return {0.0f, 0.0f, 0.0f, 93.14f, 0.0f};
    }

    ServerWrapper server = gameWrapper->GetCurrentGameState();
    if (server.IsNull()) {
        return {0.0f, 0.0f, 0.0f, 93.14f, 0.0f};
    }

    BallWrapper ball = server.GetBall();
    if (ball.IsNull()) {
        return {0.0f, 0.0f, 0.0f, 93.14f, 0.0f};
    }

    CarWrapper car = gameWrapper->GetLocalCar();
    if (car.IsNull()) {
        return {0.0f, 0.0f, 0.0f, 93.14f, 0.0f};
    }

    Vector ballPos = ball.GetCurrentRBLocation();
    Vector carVel = car.GetVelocity();
    Vector ballVel = ball.GetVelocity();

    float carXYMag = GetMagnitude(carVel.X, carVel.Y);
    float ballXYMag = GetMagnitude(ballVel.X, ballVel.Y);

    float velocityDifference = carXYMag - ballXYMag;
    if (-1 <= velocityDifference && velocityDifference <= 1) {
        velocityDifference = 0;
    }

    float zVelDif = std::abs(carVel.Z) - std::abs(ballVel.Z);
    if (-1 <= zVelDif && zVelDif <= 1) {
        zVelDif = 0;
    }

    return {velocityDifference, ballPos.X, ballPos.Y, ballPos.Z, zVelDif};
}

int FirstTouchTrainer::checkConditions() {
    if (!*bEnabled && !*zTouchZoneEnabled && !*zTouchZoneSphereEnabled) {
        return 0;
    }
    if (gameWrapper->IsInOnlineGame()) {
        return 0;
    }
    return 1;
}

void FirstTouchTrainer::OnFreeplayLoad(std::string eventName) {
    sessionStartTime = gameWrapper->GetGameTime(); // Capture start time
    sessionRunning = true;
}

void FirstTouchTrainer::OnFreeplayDestroy(std::string eventName) {
    sessionRunning = false;
}

void FirstTouchTrainer::RenderFTT(CanvasWrapper canvas) {
    if (*bEnabled) {
        canvas.SetColor(*cGoodColor);
        canvas.DrawString("First Touch Trainer Enabled", *tXPos, *tYPos, *tTextSize, *tTextSize, *tDropShadow);
    }
}

void FirstTouchTrainer::RenderSessionTimer(CanvasWrapper canvas) {
    if (*sSessionTimerEnabled && sessionRunning) {
        float elapsedTime = gameWrapper->GetGameTime() - sessionStartTime;
        int minutes = static_cast<int>(elapsedTime) / 60;
        int seconds = static_cast<int>(elapsedTime) % 60;
        std::string timerText = ToStringPrecision(minutes, 2) + ":" + ToStringPrecision(seconds, 2);
        canvas.SetColor(*sSessionTimerColor);
        canvas.DrawString("Session Timer: " + timerText, *sSessionTimerX, *sSessionTimerY, *tTextSize, *tTextSize, *tDropShadow);
    }
}

void FirstTouchTrainer::RenderTouchZone(CanvasWrapper canvas) {
    if (*zTouchZoneEnabled) {
        canvas.SetColor(*zTouchZoneColor);
        if (*zTouchZoneCircleEnabled) {
            canvas.DrawCircle(*tXPos, *tYPos, *zTouchZoneCircleRadius, *zTouchZoneCircleThicc);
        }
        if (*zTouchZoneSphereEnabled) {
            // Convert 2D position to 3D position based on camera view if needed
            Vector carPos = gameWrapper->GetLocalCar().GetLocation();
            canvas.DrawSphere(carPos.X, carPos.Y, carPos.Z, *zTouchZoneSphereRadius);
        }
    }
}