#include "SolarcApp.h"
#include "Utility/CompileTimeUtil.h"
#include <thread>
#include <algorithm>
#include <cmath>
#include "Logging/LogMacros.h"
#include "Rendering/RHI/RHI.h"

#undef min
#undef max

// ============================================================================
// SolarcApp - Singleton and Initialization
// ============================================================================

void SolarcApp::Initialize(const std::string& configDataPath)
{
    assert(!m_Instance && "SolarcApp already initialized");
    m_Instance = std::make_unique<SolarcApp>(configDataPath);
}

SolarcApp& SolarcApp::Get()
{
    assert(m_Instance && "SolarcApp not initialized. Call Initialize() first.");
    return *m_Instance;
}

SolarcApp::SolarcApp(const std::string& configDataPath)
    : m_ConfigDataPath(configDataPath)
{
    // Note: We don't parse config here anymore - that happens in INITIALIZE state

    // Create the WindowContext with the platform

    m_Ctx.windowCtx = &WindowContext::Get();

    m_StateMachine = std::make_unique<SolarcStateMachine>(m_Ctx , configDataPath);

    // JobSystem will be created after parsing thread counts in INITIALIZE state
    // State machine will be created after JobSystem is ready
}

SolarcApp::~SolarcApp()
{
    // Destroy in reverse order of creation
    m_StateMachine.reset();
    m_Ctx.windowCtx->Shutdown();
    m_JobSystem.reset();
}

void SolarcApp::SetInitialProject(const std::string& projectPath)
{
    m_InitialProjectPath = projectPath;
}

uint8_t SolarcApp::GetThreadCountFor(const std::string& systemComponent)
{
    auto it = m_ThreadCounts.find(systemComponent);
    if (it != m_ThreadCounts.end()) {
        return it->second;
    }
    return 0; // Return 0 if not found (JobSystem will use default)
}

void SolarcApp::Run()
{
    while (m_IsRunning)
    {
        if (m_Ctx.windowCtx)
            m_Ctx.windowCtx->PollEvents();

        if (m_StateMachine)
            m_StateMachine->Update();
    }
}

// ============================================================================
// Configuration Parsing
// ============================================================================

void SolarcApp::ParseWindowData(const toml::value& configData)
{
    if (!configData.contains("window"))
    {
        SOLARC_APP_WARN("Missing [window] table in config. Using defaults.");
        return;
    }

    const auto& window = toml::find(configData, "window");
    if (!window.is_table())
    {
        SOLARC_APP_ERROR("[window] must be a table");
        return;
    }

    m_WindowWidth = toml::find_or(window, "width", 1920);
    m_WindowHeight = toml::find_or(window, "height", 1080);
    m_WindowFullscreen = toml::find_or(window, "fullscreen", false);
    m_WindowName = toml::find_or(window, "Name", "Solarc Window");
}

void SolarcApp::ParseMTData(const toml::value& configData)
{
    if (!configData.contains("threading"))
    {
        SOLARC_APP_WARN("Missing [threading] table in config. Using defaults.");
        return;
    }

    const auto& threading = toml::find(configData, "threading");
    if (!threading.is_table())
    {
        SOLARC_APP_ERROR("[threading] must be a table");
        return;
    }

    const auto hw_threads = std::thread::hardware_concurrency();
    unsigned int totalFactor = 0;
    std::vector<std::pair<std::string, int>> factors;

    for (const auto& [key, val] : toml::get<toml::table>(threading))
    {
        if (!val.is_integer())
        {
            SOLARC_APP_ERROR("Thread count factor for '{}' must be an integer", key);
            continue;
        }
        int factor = toml::get<int>(val);
        if (factor < 0)
        {
            SOLARC_APP_ERROR("Factor for '{}' must be non-negative", key);
            continue;
        }
        factors.emplace_back(key, factor);
        totalFactor += factor;
    }

    if (totalFactor == 0) return;
    if (totalFactor != 100)
        SOLARC_APP_WARN("Total thread distribution factor is {} (should be 100). Normalizing.", totalFactor); std::vector<std::pair<std::string, uint8_t>> allocations;
    std::vector<std::pair<size_t, double>> remainders;
    unsigned int allocated = 0;
    size_t idx = 0;

    for (const auto& [key, factor] : factors)
    {
        double exactAlloc = (static_cast<double>(factor) * hw_threads) / totalFactor;
        uint8_t baseAlloc = static_cast<uint8_t>(std::floor(exactAlloc));
        allocations.emplace_back(key, baseAlloc);
        allocated += baseAlloc;
        double rem = exactAlloc - baseAlloc;
        remainders.emplace_back(idx++, rem);
    }

    unsigned int leftover = hw_threads - allocated;
    std::sort(remainders.begin(), remainders.end(),
        [](const auto& a, const auto& b) {
            if (a.second == b.second) return a.first < b.first;
            return a.second > b.second;
        });

    for (unsigned int i = 0; i < std::min(leftover, static_cast<unsigned int>(remainders.size())); ++i)
    {
        allocations[remainders[i].first].second += 1;
    }

    for (auto& [key, count] : allocations)
    {
        m_ThreadCounts[key] = count;
        spdlog::info("Thread allocation: {} = {} threads", key, count);
    }
}

void SolarcApp::ParseStartupData(const toml::value& configData)
{
    if (!configData.contains("startup"))
    {
        SOLARC_APP_WARN("Missing [startup] table in config.");
        return;
    }

    const auto& startup = toml::find(configData, "startup");
    if (!startup.is_table())
    {
        SOLARC_APP_ERROR("[startup] must be a table");
        return;
    }

    if (startup.contains("project_to_open"))
    {
        std::string projectPath = toml::find<std::string>(startup, "project_to_open");
        SetInitialProject(projectPath);
    }
}

void SolarcApp::ParseRenderingData(const toml::value& configData)
{
    if (!configData.contains("rendering"))
    {
        SOLARC_APP_DEBUG("No [rendering] section in config, using defaults");
        return;
    }

    const auto& rendering = toml::find(configData, "rendering");
    if (!rendering.is_table())
    {
        SOLARC_APP_ERROR("[rendering] must be a table");
        return;
    }

    // Parse VSync setting
    bool vsync = toml::find_or(rendering, "vsync", true);
    SetVSyncPreference(vsync);
    SOLARC_APP_INFO("Config: VSync = {}", vsync ? "enabled" : "disabled");
}

// ============================================================================
// State Machine
// ============================================================================

SolarcApp::SolarcStateMachine::SolarcStateMachine(SolarcContext& solarcCtx,
    const std::string& initialProjectPath)
    : m_SolarctCtxRef(solarcCtx)
    , m_InitialProjectPath(initialProjectPath)
{
    m_CurrentState = std::make_unique<SolarcStateInitialize>(m_SolarctCtxRef);
    m_CurrentState->OnEnter();
}

void SolarcApp::SolarcStateMachine::Update()
{
    if (!m_CurrentState) return;

    // Update current state
    StateTransitionData result = m_CurrentState->Update();

    // Handle transition immediately (same frame)
    if (result.transition != StateTransition::NONE)
    {
        TransitionTo(result.transition, result.projectPath);
    }
}

void SolarcApp::SolarcStateMachine::TransitionTo(StateTransition transition,
    const std::string& data)
{
    // Exit current state
    if (m_CurrentState)
    {
        SOLARC_APP_INFO("Exiting state: {}", static_cast<int>(m_CurrentState->GetType()));
        m_CurrentState->OnExit();
    }

    // Create new state based on transition
    switch (transition)
    {
    case StateTransition::TO_STAGING:
        m_CurrentState = std::make_unique<SolarcStateStaging>(m_SolarctCtxRef, data);
        break;

    case StateTransition::TO_LOADING:
        m_CurrentState = std::make_unique<SolarcStateLoading>(m_SolarctCtxRef, data);
        break;

    case StateTransition::TO_RUNNING:
        m_CurrentState = std::make_unique<SolarcStateRunning>(m_SolarctCtxRef);
        break;

    case StateTransition::TO_CLEANUP:
        m_CurrentState = std::make_unique<SolarcStateCleanup>(m_SolarctCtxRef);
        break;

    case StateTransition::QUIT:
        m_CurrentState.reset();
        SolarcApp::Get().RequestQuit();
        SOLARC_APP_INFO("Application shutdown requested");
        return;

    default:
        SOLARC_APP_ERROR("Unknown state transition: {}", static_cast<int>(transition));
        return;
    }// Enter new state
    if (m_CurrentState)
    {
        SOLARC_APP_INFO("Entering state: {}", static_cast<int>(m_CurrentState->GetType()));
        m_CurrentState->OnEnter();
    }
}

// ============================================================================
// Base State
// ============================================================================

SolarcApp::SolarcState::SolarcState(SolarcContext& ctxRef, SOLARC_STATE_TYPE type)
    : m_SolarctCtxRef(ctxRef)
    , m_Type(type)
{
}

// ============================================================================
// Concrete States
// ============================================================================

// --- INITIALIZE State ---

SolarcApp::SolarcStateInitialize::SolarcStateInitialize(SolarcContext& ctx)
    : SolarcState(ctx, SOLARC_STATE_TYPE::INITIALIZE)
{
}

SolarcApp::StateTransitionData SolarcApp::SolarcStateInitialize::Update()
{
    SOLARC_APP_INFO("Initializing application...");

    SolarcApp& app = SolarcApp::Get();
    const std::string& configDataPath = app.m_ConfigDataPath;

    // Parse configuration file
    toml::value configData;
    try
    {
        configData = toml::parse(configDataPath);
    }
    catch (const toml::syntax_error& e)
    {
        SOLARC_APP_ERROR("TOML Parse Error in '{}': {}", configDataPath, e.what());
        std::exit(EXIT_FAILURE);
    }

    // Parse window data
    app.ParseWindowData(configData);

    // Parse threading data
    app.ParseMTData(configData);

    // Parse startup data (sets initial project path)
    app.ParseStartupData(configData);

    // Parse rendering data
    app.ParseRenderingData(configData);

    // Create JobSystem now that we have thread counts
    size_t numWorkers = app.GetThreadCountFor("job_system");
    if (numWorkers == 0) {
        numWorkers = std::max(1u, std::thread::hardware_concurrency() - 1);
        SOLARC_APP_INFO("Using default job system thread count: {}", numWorkers);
    }

    app.m_JobSystem = std::make_unique<JobSystem>(numWorkers);
    app.m_Ctx.jobSystem = app.m_JobSystem.get();

    SOLARC_APP_INFO("JobSystem created with {} worker threads", numWorkers);

    // Decide next state based on whether we have an initial project
    if (!app.m_InitialProjectPath.empty())
    {
        SOLARC_APP_INFO("Initial project specified: {}", app.m_InitialProjectPath);
        return { StateTransition::TO_LOADING, app.m_InitialProjectPath };
    }
    else
    {
        SOLARC_APP_INFO("No initial project, going to staging");
        return { StateTransition::TO_STAGING, "" };
    }
}

// --- STAGING State ---

SolarcApp::SolarcStateStaging::SolarcStateStaging(SolarcContext& ctx,
    const std::string& projectToOpen)
    : SolarcState(ctx, SOLARC_STATE_TYPE::STAGING)
    , m_ProjectToOpen(projectToOpen)
{
}

SolarcApp::StateTransitionData SolarcApp::SolarcStateStaging::Update()
{
    SOLARC_APP_INFO("In staging state (project selection)");

    // TODO: In a real implementation, this would show a UI for project selection
    // For now, we'll just transition to loading with an empty/default project

    if (m_ProjectToOpen.empty())
    {
        // Use a default project path or just go to running with no project
        m_ProjectToOpen = "default_project";
    }

    return { StateTransition::TO_LOADING, m_ProjectToOpen };
}

// --- LOADING State ---

SolarcApp::SolarcStateLoading::SolarcStateLoading(SolarcContext& ctx,
    const std::string& projectToOpen)
    : SolarcState(ctx, SOLARC_STATE_TYPE::LOADING)
    , m_ProjectPath(projectToOpen)
{
}

void SolarcApp::SolarcStateLoading::OnEnter()
{
    SOLARC_APP_INFO("Loading project: {}", m_ProjectPath);

    // Kick off async asset loading
    auto& jobSys = *m_SolarctCtxRef.jobSystem;

    m_LoadingJob = jobSys.Schedule([projectPath = m_ProjectPath]() {// Simulate loading project configuration and assets
        SOLARC_APP_INFO("Loading project data for: {}", projectPath);

        // TODO: Real implementation would:
        // - Parse project file
        // - Load scene hierarchy
        // - Queue asset loads (textures, models, etc.)
        // - Initialize renderer with project settings

        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        SOLARC_APP_INFO("Project loading complete: {}", projectPath);

        }, {}, "Load Project");
}

SolarcApp::StateTransitionData SolarcApp::SolarcStateLoading::Update()
{
    // Check if loading is complete
    if (m_LoadingJob.IsComplete())
    {
        SOLARC_APP_INFO("Assets loaded, transitioning to running state");
        return { StateTransition::TO_RUNNING, "" };
    }

    // Still loading - could update loading UI here
    // TODO: Show loading progress bar

    return { StateTransition::NONE, "" };
}

// --- RUNNING State ---

SolarcApp::SolarcStateRunning::SolarcStateRunning(SolarcContext& ctx)
    : SolarcState(ctx, SOLARC_STATE_TYPE::RUNNING)
{
}

void SolarcApp::SolarcStateRunning::OnEnter()
{
    SOLARC_APP_INFO("Entering running state - creating main window");

    SolarcApp& app = SolarcApp::Get();

    // Create the main window using parsed configuration
    m_MainWindow = m_SolarctCtxRef.windowCtx->CreateWindow(
        app.m_WindowName,
        app.m_WindowWidth,
        app.m_WindowHeight
    );

    m_Bus.RegisterProducer(m_MainWindow.get());

    m_MainWindow->Show();
    m_MainWindow->Update();
    SOLARC_APP_INFO("Main window created and shown");

    // Initialize RHI with the main window
    try {
        if (m_MainWindow->IsVisible() && !m_MainWindow->IsMinimized())
        {
            SOLARC_APP_INFO("Initializing RHI...");
            RHI::Initialize(m_MainWindow);
            m_Bus.RegisterListener(&RHI::Get());
            SOLARC_APP_INFO("RHI initialized successfully");

            // Apply VSync preference if overridden
            if (app.m_VSyncOverride)
            {
                RHI::Get().SetVSync(app.m_VSyncEnabled);
                SOLARC_APP_INFO("Applied VSync preference: {}", app.m_VSyncEnabled ? "ON" : "OFF");
            }
        }
    }
    catch (const std::exception& e) {
        SOLARC_CRITICAL("Failed to initialize RHI: {}", e.what());
        throw; // Re-throw to trigger cleanup state
    }


}

SolarcApp::StateTransitionData SolarcApp::SolarcStateRunning::Update()
{
    // Update the window (processes its event queue)
    m_MainWindow->Update();

    if (!RHI::IsInitialized())
    {
        if (m_MainWindow->IsVisible() && !m_MainWindow->IsMinimized())
        {
            SOLARC_APP_INFO("Initializing RHI...");
            RHI::Initialize(m_MainWindow);
            m_Bus.RegisterListener(&RHI::Get());
            SOLARC_APP_INFO("RHI initialized successfully");


            SolarcApp& app = SolarcApp::Get();
            // Apply VSync preference if overridden
            if (app.m_VSyncOverride)
            {
                RHI::Get().SetVSync(app.m_VSyncEnabled);
                SOLARC_APP_INFO("Applied VSync preference: {}", app.m_VSyncEnabled ? "ON" : "OFF");
            }
        }

    }

    // Render frame if RHI is initialized and window is not minimized
    if (RHI::IsInitialized() && !m_MainWindow->IsMinimized())
    {
        auto& rhi = RHI::Get();

        // Begin frame
        rhi.BeginFrame();

        // Clear screen to dark blue
        rhi.Clear(0.1f, 0.2f, 0.3f, 1.0f);

        // TODO: Future rendering work goes here:
        // - Draw scene geometry
        // - Render UI
        // - Post-processing effects

        // End frame and present
        rhi.EndFrame();
        rhi.Present();
    }

    // Check if window was closed
    if (m_MainWindow->IsClosed())
    {
        SOLARC_APP_INFO("Main window closed by user");
        return { StateTransition::TO_CLEANUP, "" };
    }

    return { StateTransition::NONE, "" };
}

void SolarcApp::SolarcStateRunning::OnExit()
{
    SOLARC_APP_INFO("Exiting running state - cleaning up RHI and window");

    // Shutdown RHI BEFORE destroying the window
    if (RHI::IsInitialized())
    {
        SOLARC_APP_INFO("Shutting down RHI...");
        try {
            RHI::Shutdown();
            SOLARC_APP_INFO("RHI shutdown complete");
        }
        catch (const std::exception& e) {
            SOLARC_APP_ERROR("Exception during RHI shutdown: {}", e.what());
        }
    }

    // Now safe to destroy the window
    if (m_MainWindow)
    {
        m_MainWindow->Hide();
        m_MainWindow->Update();
        m_MainWindow.reset();
        SOLARC_APP_INFO("Main window destroyed");
    }
}

// --- CLEANUP State ---

SolarcApp::SolarcStateCleanup::SolarcStateCleanup(SolarcContext& ctx)
    : SolarcState(ctx, SOLARC_STATE_TYPE::CLEANUP)
{
}

SolarcApp::StateTransitionData SolarcApp::SolarcStateCleanup::Update()
{
    SOLARC_APP_INFO("Performing cleanup...");

    // TODO: Real cleanup:
    // - Save application state
    // - Release GPU resources
    // - Flush logs
    // - etc.

    SOLARC_APP_INFO("Cleanup complete");

    return { StateTransition::QUIT, "" };
}