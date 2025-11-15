#pragma once
#include "Preprocessor/API.h"
#include "Window/Window.h"
#include "Window/WindowContext.h"
#include "Event/ObserverBus.h"
#include "Event/ApplicationEvent.h"
#include "toml.hpp"

class SOLARC_CORE_API SolarcApp
{
public:
    SolarcApp(const std::string& configDataPath);
    ~SolarcApp() = default;

    SolarcApp(SolarcApp&& other) = delete;
    SolarcApp& operator=(SolarcApp&& other) = delete;
    SolarcApp(const SolarcApp& other) = delete;
    SolarcApp& operator=(const SolarcApp& other) = delete;

    static void Initialize(const std::string& configDataPath);
    static SolarcApp& Get();

    void SetInitialProject(const std::string& projectPath);

    uint8_t GetThreadCountFor(const std::string& systemComponent) { return m_ThreadCounts[systemComponent]; }

    void Run();
    void RequestQuit() { m_IsRunning = false; }

private:
    struct SolarcContext
    {
        std::unique_ptr<WindowContext> windowCtx;
    };

    enum class SOLARC_STATE_TYPE
    {
        INITIALIZE,
        STAGING,
        LOADING,
        RUNNING,
        CLEANUP
    };

    class SolarcState;
    class SolarcStateInitialize;
    class SolarcStateStaging;
    class SolarcStateLoading;
    class SolarcStateRunning;
    class SolarcStateCleanup;

    class SolarcStateMachine : public EventListener<ApplicationEvent>
    {
    public:
        SolarcStateMachine(SolarcContext& solarcCtx);
        void Update();
        void OnEvent(const std::shared_ptr<const ApplicationEvent>& e) override;

        void SetInitialProject(const std::string& path) { m_InitialProjectPath = path; }

    private:
        using TransitionFunc = std::function<void(SolarcStateMachine&, std::shared_ptr<const ApplicationEvent>)>;
        void BuildTransitionTable();

        ObserverBus<ApplicationEvent> m_Bus;
        SolarcContext& m_SolarctCtxRef;
        std::unique_ptr<SolarcState> m_State;
        std::string m_InitialProjectPath; // set once at startup
        std::string m_ProjectToOpen;      // for runtime changes (e.g., from staging)
        std::unordered_map<ApplicationEvent::TYPE, TransitionFunc> m_TransitionTable;
    };

    void ParseWindowData(const toml::value& configData);
    void ParseMTData(const toml::value& configData);
    void ParseStartupData(const toml::value& configData);
    void PerformInitializationLogic();

    inline static std::unique_ptr<SolarcApp> m_Instance = nullptr;

    SolarcContext m_Ctx;
    std::unique_ptr<SolarcStateMachine> m_StateMachine;
    std::unordered_map<std::string, uint8_t> m_ThreadCounts;
    bool m_IsRunning = true;
    std::string m_ConfigDataPath;

    // Store parsed window configuration
    int m_WindowWidth = 1920;
    int m_WindowHeight = 1080;
    bool m_WindowFullscreen = false;
    std::string m_WindowName = "Solarc Window";
};

// --- Base State ---
class SolarcApp::SolarcState : public EventProducer<ApplicationEvent>
{
public:
    SolarcState(SolarcContext& ctxRef, SOLARC_STATE_TYPE type);
    virtual ~SolarcState() = default;
    virtual void Update() = 0;
    SOLARC_STATE_TYPE GetType() const { return m_Type; }

protected:
    SolarcContext& m_SolarctCtxRef;

private:
    SOLARC_STATE_TYPE m_Type;
};

// --- Concrete States ---
class SolarcApp::SolarcStateInitialize : public SolarcState
{
public:
    SolarcStateInitialize(SolarcContext& ctx);
    void Update() override;
};

class SolarcApp::SolarcStateStaging : public SolarcState
{
public:
    SolarcStateStaging(SolarcContext& ctx, std::string& projectToOpen);
    void Update() override;

private:
    std::string& m_ProjectToOpen;
};

class SolarcApp::SolarcStateLoading : public SolarcState
{
public:
    SolarcStateLoading(SolarcContext& ctx, const std::string& projectToOpen);
    void Update() override;

private:
    std::string m_ProjectToOpen;
};

class SolarcApp::SolarcStateRunning : public SolarcState
{
public:
    SolarcStateRunning(SolarcContext& ctx);
    void Update() override;

private:
    std::shared_ptr<Window> m_MainWindow;
};

class SolarcApp::SolarcStateCleanup : public SolarcState
{
public:
    SolarcStateCleanup(SolarcContext& ctx);
    void Update() override;
};