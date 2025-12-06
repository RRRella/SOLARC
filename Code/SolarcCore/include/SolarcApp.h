#pragma once
#include "Preprocessor/API.h"
#include "Window/Window.h"
#include "Window/WindowContext.h"
#include "MT/JobSystem.h"
#include "toml.hpp"
#include <memory>
#include <string>
#include <unordered_map>

class SOLARC_CORE_API SolarcApp
{
public:
    SolarcApp(const std::string& configDataPath);
    ~SolarcApp();

    SolarcApp(SolarcApp&& other) = delete;
    SolarcApp& operator=(SolarcApp&& other) = delete;
    SolarcApp(const SolarcApp& other) = delete;
    SolarcApp& operator=(const SolarcApp& other) = delete;

    static void Initialize(const std::string& configDataPath);
    static SolarcApp& Get();

    void SetInitialProject(const std::string& projectPath);

    uint8_t GetThreadCountFor(const std::string& systemComponent);

    void Run();
    void RequestQuit() { m_IsRunning = false; }

private:
    friend class SolarcState;
    friend class SolarcStateInitialize;
    friend class SolarcStateLoading;

    struct SolarcContext
    {
        WindowContext* windowCtx;
        JobSystem* jobSystem = nullptr; // Non-owning pointer
    };

    enum class SOLARC_STATE_TYPE
    {
        INITIALIZE,
        STAGING,
        LOADING,
        RUNNING,
        CLEANUP
    };

    enum class StateTransition
    {
        NONE,
        TO_STAGING,
        TO_LOADING,
        TO_RUNNING,
        TO_CLEANUP,
        QUIT
    };

    struct StateTransitionData
    {
        StateTransition transition = StateTransition::NONE;
        std::string projectPath;
    };

    // Forward declarations
    class SolarcState;
    class SolarcStateInitialize;
    class SolarcStateStaging;
    class SolarcStateLoading;
    class SolarcStateRunning;
    class SolarcStateCleanup;
    class SolarcStateMachine;

    void ParseWindowData(const toml::value& configData);
    void ParseMTData(const toml::value& configData);
    void ParseStartupData(const toml::value& configData);

    inline static std::unique_ptr<SolarcApp> m_Instance = nullptr;

    SolarcContext m_Ctx;
    std::unique_ptr<JobSystem> m_JobSystem;
    std::unique_ptr<SolarcStateMachine> m_StateMachine;
    std::unordered_map<std::string, uint8_t> m_ThreadCounts;
    bool m_IsRunning = true;
    std::string m_ConfigDataPath;

    // Store parsed window configuration
    int m_WindowWidth = 1920;
    int m_WindowHeight = 1080;
    bool m_WindowFullscreen = false;
    std::string m_WindowName = "Solarc Window";

    // Initial project path (set before state machine starts)
    std::string m_InitialProjectPath;
};

// ============================================================================
// State Machine
// ============================================================================

class SolarcApp::SolarcStateMachine
{
public:
    SolarcStateMachine(SolarcContext& solarcCtx, const std::string& initialProjectPath);
    void Update();

private:
    void TransitionTo(StateTransition transition, const std::string& data);

    SolarcContext& m_SolarctCtxRef;
    std::unique_ptr<SolarcState> m_CurrentState;
    std::string m_InitialProjectPath;
};

// ============================================================================
// Base State
// ============================================================================

class SolarcApp::SolarcState
{
public:
    SolarcState(SolarcContext& ctxRef, SOLARC_STATE_TYPE type);
    virtual ~SolarcState() = default;

    // Returns what should happen next
    virtual StateTransitionData Update() = 0;

    // Called once when entering this state
    virtual void OnEnter() {}

    // Called once when leaving this state
    virtual void OnExit() {}

    SOLARC_STATE_TYPE GetType() const { return m_Type; }

protected:
    SolarcContext& m_SolarctCtxRef;

private:
    SOLARC_STATE_TYPE m_Type;
};

// ============================================================================
// Concrete States
// ============================================================================

class SolarcApp::SolarcStateInitialize : public SolarcState
{
public:
    SolarcStateInitialize(SolarcContext & ctx);
    StateTransitionData Update() override;
};

class SolarcApp::SolarcStateStaging : public SolarcState
{
public:
    SolarcStateStaging(SolarcContext& ctx, const std::string& projectToOpen);
    StateTransitionData Update() override;

private:
    std::string m_ProjectToOpen;
};

class SolarcApp::SolarcStateLoading : public SolarcState
{
public:
    SolarcStateLoading(SolarcContext& ctx, const std::string& projectToOpen);
    void OnEnter() override;
    StateTransitionData Update() override;

private:
    std::string m_ProjectPath;
    JobHandle m_LoadingJob;
};

class SolarcApp::SolarcStateRunning : public SolarcState
{
public:
    SolarcStateRunning(SolarcContext& ctx);
    void OnEnter() override;
    StateTransitionData Update() override;
    void OnExit() override;

private:
    std::shared_ptr<Window> m_MainWindow;
};

class SolarcApp::SolarcStateCleanup : public SolarcState
{
public:
    SolarcStateCleanup(SolarcContext& ctx);
    StateTransitionData Update() override;
};