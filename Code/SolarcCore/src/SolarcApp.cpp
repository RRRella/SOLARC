#include "SolarcApp.h"
#include "spdlog/spdlog.h"
#include "Utility/CompileTimeUtil.h"
#include "Window/WindowContextPlatform.h"

// --- SolarcState base ---
SolarcApp::SolarcState::SolarcState(SolarcContext& ctxRef, SOLARC_STATE_TYPE type)
    : m_SolarctCtxRef(ctxRef), m_Type(type)
{
}

// --- Concrete States (Update implementations) ---
void SolarcApp::SolarcStateInitialize::Update()
{
    // Perform the initialization logic that was previously in Initialize() and constructor
    SolarcApp& app = SolarcApp::Get();
    const std::string& configDataPath = app.m_ConfigDataPath;

    toml::value configData;
    try
    {
        configData = toml::parse(configDataPath);
    }
    catch (const toml::syntax_error& e)
    {
        std::cerr << "TOML Parse Error in '" << configDataPath << "':\n" << e.what() << std::endl;
        std::exit(EXIT_FAILURE);
    }

    // Parse window data
    app.ParseWindowData(configData);

    // Parse threading data
    app.ParseMTData(configData);

    // Parse startup data
    app.ParseStartupData(configData);

    //We're done , emit InitializeCompleteEvent to state machine
    auto event = std::make_shared<InitializeCompleteEvent>();
    DispatchEvent(event);
}

void SolarcApp::SolarcStateStaging::Update()
{
    // In a real implementation, this would show a UI for project selection
    // For now, we'll just use a hardcoded project path or skip to loading
    // Since we're skipping the actual UI, let's just emit the event with the current project path
    auto event = std::make_shared<StagingCompleteEvent>(m_ProjectToOpen);
    DispatchEvent(event);
}

void SolarcApp::SolarcStateLoading::Update()
{
    // Skip loading and emit LoadingCompleteEvent immediately
    auto event = std::make_shared<LoadingCompleteEvent>();
    DispatchEvent(event);
}

void SolarcApp::SolarcStateRunning::Update()
{
    // Create the main window if it doesn't exist
    if (!m_MainWindow) {
        SolarcApp& app = SolarcApp::Get();
        SolarcApp::SolarcContext& ctx = app.m_Ctx;

        // Create the main window using the parsed configuration
        m_MainWindow = ctx.windowCtx->CreateWindow(
            app.m_WindowName,
            app.m_WindowWidth,
            app.m_WindowHeight
        );

        m_MainWindow->Show();
    }

    m_MainWindow->Update();

    // Check if the main window is still valid (not destroyed)
    if (m_MainWindow && !m_MainWindow->IsVisible()) {
        // Window closed, emit RunningCompleteEvent with SHUTDOWN action
        auto event = std::make_shared<RunningCompleteEvent>(PostRunAction::SHUTDOWN);
        DispatchEvent(event);
    }
}

void SolarcApp::SolarcStateCleanup::Update()
{
    //We're done , emit CleanupCompleteEvent to state machine
    auto event = std::make_shared<CleanupCompleteEvent>();
    DispatchEvent(event);
}

// --- State Constructors ---
SolarcApp::SolarcStateInitialize::SolarcStateInitialize(SolarcContext& ctx)
    : SolarcState(ctx, SOLARC_STATE_TYPE::INITIALIZE) {
}

SolarcApp::SolarcStateStaging::SolarcStateStaging(SolarcContext& ctx, std::string& projectToOpen)
    : SolarcState(ctx, SOLARC_STATE_TYPE::STAGING), m_ProjectToOpen(projectToOpen) {
}

SolarcApp::SolarcStateLoading::SolarcStateLoading(SolarcContext& ctx, const std::string& projectToOpen)
    : SolarcState(ctx, SOLARC_STATE_TYPE::LOADING), m_ProjectToOpen(projectToOpen) {
}

SolarcApp::SolarcStateRunning::SolarcStateRunning(SolarcContext& ctx)
    : SolarcState(ctx, SOLARC_STATE_TYPE::RUNNING) {
}

SolarcApp::SolarcStateCleanup::SolarcStateCleanup(SolarcContext& ctx)
    : SolarcState(ctx, SOLARC_STATE_TYPE::CLEANUP) {
}

// --- SolarcApp ---
void SolarcApp::Initialize(const std::string& configDataPath)
{
    assert(!m_Instance, "SolarcApp already initialized");
    m_Instance = std::make_unique<SolarcApp>(configDataPath);
}

SolarcApp& SolarcApp::Get()
{
    assert(m_Instance, "SolarcApp not initialized. Call Initialize() first.");
    return *m_Instance;
}

void SolarcApp::SetInitialProject(const std::string& projectPath)
{
    if (m_StateMachine)
        m_StateMachine->SetInitialProject(projectPath);
    else
        m_StateMachine->SetInitialProject(projectPath); // will be read in constructor logic
}

SolarcApp::SolarcApp(const std::string& configDataPath)
    : m_ConfigDataPath(configDataPath) // Store the path for later use
{
    // Create the WindowContext with the platform
    m_Ctx.windowCtx = std::make_unique<WindowContext>(WindowContextPlatform::CreateWindowContextPlatform());
    m_StateMachine = std::make_unique<SolarcStateMachine>(m_Ctx);
}

void SolarcApp::Run()
{
    while (m_IsRunning)
    {
        if (m_Ctx.windowCtx)
            m_Ctx.windowCtx->PollEvents();

        m_StateMachine->Update();
    }
}

void SolarcApp::ParseWindowData(const toml::value& configData)
{
    if (!configData.contains("window"))
    {
        spdlog::warn("Missing [window] table in config. Using defaults.");
        return;
    }

    const auto& window = toml::find(configData, "window");
    if (!window.is_table())
    {
        spdlog::error("[window] must be a table");
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
        spdlog::warn("Missing [threading] table in config. Using defaults.");
        return;
    }

    const auto& threading = toml::find(configData, "threading");
    if (!threading.is_table())
    {
        spdlog::error("[threading] must be a table");
        return;
    }

    const auto hw_threads = std::thread::hardware_concurrency();
    unsigned int totalFactor = 0;
    std::vector<std::pair<std::string, int>> factors;

    for (const auto& [key, val] : toml::get<toml::table>(threading))
    {
        if (!val.is_integer())
        {
            spdlog::error("Thread count factor for '{}' must be an integer", key);
            continue;
        }
        int factor = toml::get<int>(val);
        if (factor < 0)
        {
            spdlog::error("Factor for '{}' must be non-negative", key);
            continue;
        }
        factors.emplace_back(key, factor);
        totalFactor += factor;
    }

    if (totalFactor == 0) return;
    if (totalFactor != 100)
        spdlog::warn("Total thread distribution factor is {} (should be 100). Normalizing.", totalFactor);

    std::vector<std::pair<std::string, uint8_t>> allocations;
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
    }
}

void SolarcApp::ParseStartupData(const toml::value& configData)
{
    if (!configData.contains("startup"))
    {
        spdlog::warn("Missing [startup] table in config.");
        return;
    }

    const auto& startup = toml::find(configData, "startup");
    if (!startup.is_table())
    {
        spdlog::error("[startup] must be a table");
        return;
    }

    if (startup.contains("project_to_open"))
    {
        std::string projectPath = toml::find<std::string>(startup, "project_to_open");
        SetInitialProject(projectPath);
    }
}

void SolarcApp::PerformInitializationLogic()
{
    // This function can be used if additional initialization logic is needed
    // Currently, all initialization logic is in the SolarcStateInitialize::Update method
}

// --- SolarcStateMachine ---
SolarcApp::SolarcStateMachine::SolarcStateMachine(SolarcContext& solarcCtx)
    : m_SolarctCtxRef(solarcCtx)
{
    m_State = std::make_unique<SolarcStateInitialize>(m_SolarctCtxRef);
    m_Bus.RegisterListener(this);
    m_Bus.RegisterProducer(m_State.get());
    BuildTransitionTable();
}

void SolarcApp::SolarcStateMachine::BuildTransitionTable()
{
    using ET = ApplicationEvent::TYPE;

    auto reg = [this](ET type, TransitionFunc func) {
        m_TransitionTable[type] = func;
        };

    reg(ET::INITIALIZE_COMPLETE,
        [this](SolarcStateMachine& sm, std::shared_ptr<const ApplicationEvent> e) {
            sm.m_Bus.UnregisterProducer(sm.m_State.get());
            if (!sm.m_InitialProjectPath.empty()) {
                sm.m_State = std::make_unique<SolarcStateLoading>(sm.m_SolarctCtxRef, sm.m_InitialProjectPath);
            }
            else {
                sm.m_State = std::make_unique<SolarcStateStaging>(sm.m_SolarctCtxRef, sm.m_ProjectToOpen);
            }
            sm.m_Bus.RegisterProducer(sm.m_State.get());
        });

    reg(ET::STAGING_COMPLETE,
        [this](SolarcStateMachine& sm, std::shared_ptr<const ApplicationEvent> e) {
            sm.m_Bus.UnregisterProducer(sm.m_State.get());

            // Extract project path from the event
            auto stagingEvent = std::static_pointer_cast<const StagingCompleteEvent>(e);
            sm.m_State = std::make_unique<SolarcStateLoading>(sm.m_SolarctCtxRef, stagingEvent->GetProjectPath());
            sm.m_Bus.RegisterProducer(sm.m_State.get());
        });

    reg(ET::LOADING_COMPLETE,
        [](SolarcStateMachine& sm, std::shared_ptr<const ApplicationEvent> e) {
            sm.m_Bus.UnregisterProducer(sm.m_State.get());
            sm.m_State = std::make_unique<SolarcStateRunning>(sm.m_SolarctCtxRef);
            sm.m_Bus.RegisterProducer(sm.m_State.get());
        });

    reg(ET::RUNNING_COMPLETE,
        [this](SolarcStateMachine& sm, std::shared_ptr<const ApplicationEvent> e) {
            auto runningEvent = std::static_pointer_cast<const RunningCompleteEvent>(e);
            sm.m_Bus.UnregisterProducer(sm.m_State.get());

            switch (runningEvent->GetAction()) {
            case PostRunAction::SHUTDOWN:
                sm.m_State = std::make_unique<SolarcStateCleanup>(sm.m_SolarctCtxRef);
                break;
            case PostRunAction::OPEN_NEW_PROJECT:
                sm.m_ProjectToOpen.clear();
                sm.m_State = std::make_unique<SolarcStateStaging>(sm.m_SolarctCtxRef, sm.m_ProjectToOpen);
                break;
            case PostRunAction::RESTART:
                sm.m_InitialProjectPath.clear();
                sm.m_ProjectToOpen.clear();
                sm.m_State = std::make_unique<SolarcStateInitialize>(sm.m_SolarctCtxRef);
                break;
            }
            sm.m_Bus.RegisterProducer(sm.m_State.get());
        });

    reg(ET::CLEANUP_COMPLETE,
        [](SolarcStateMachine& sm, std::shared_ptr<const ApplicationEvent> e) {
            sm.m_Bus.UnregisterProducer(sm.m_State.get());
            sm.m_State.reset();
            SolarcApp::Get().RequestQuit();
        });
}

void SolarcApp::SolarcStateMachine::Update()
{
    m_Bus.Communicate();

    auto event = m_EventQueue.TryNext();
    if (event.has_value())
        OnEvent(event.value());

    if (m_State)
        m_State->Update();
}

void SolarcApp::SolarcStateMachine::OnEvent(const std::shared_ptr<const ApplicationEvent>& e)
{
    auto it = m_TransitionTable.find(e->GetApplicationEventType());
    if (it != m_TransitionTable.end())
        it->second(*this, e);
}