#include "SolarcApp.h"
#include "spdlog/spdlog.h"
#include "Utility/CompileTimeUtil.h"
#include <nlohmann/json.hpp>
using json = nlohmann::json;

void SolarcApp::Initialize()
{
	if (!m_Instance)
		m_Instance = std::make_unique<SolarcApp>();
	else
		throw std::runtime_error("SolarcApp already initialized");
}

void SolarcApp::Initialize(const std::string& configDataPath)
{
	if (!m_Instance)
		m_Instance = std::make_unique<SolarcApp>(configDataPath);
	else
		throw std::runtime_error("SolarcApp already initialized");
}

SolarcApp::SolarcApp()
{
	m_WindowFactory = GetWindowFactory();

	WindowsMetaData windowsMetaData;

	windowsMetaData.width = 1920;
	windowsMetaData.height = 1080;
	windowsMetaData.name = "Default SOLARC Window";

	windowsMetaData.posX = 0;
	windowsMetaData.posY = 0;
	windowsMetaData.extendedStyle = 0;
	windowsMetaData.style = WS_OVERLAPPEDWINDOW;

	m_Window = m_WindowFactory->Create(windowsMetaData);
}

SolarcApp::SolarcApp(const std::string& configDataPath)
{
	std::ifstream f(configDataPath);


	assert(f,"Failed to open Solarc config file.");

	json configData;

	try
	{
		configData = json::parse(f);
	}
	catch (const json::parse_error& e)
	{
		std::cerr << "JSON Parse Error: " << e.what() << std::endl;
		std::cerr << "At byte position: " << e.byte << std::endl;
	}

	m_WindowFactory = GetWindowFactory();

	WindowsMetaData windowsMetaData;

	const json& windowData = configData["WindowData"];

	windowsMetaData.width = windowData["Width"].get<int>();
	windowsMetaData.height = windowData["Height"].get<int>();
	windowsMetaData.name = windowData["Name"].get<std::string>();
	
	windowsMetaData.posX = 0;
	windowsMetaData.posY = 0;
	windowsMetaData.extendedStyle = 0;
	windowsMetaData.style = WS_OVERLAPPEDWINDOW;

	m_Window = m_WindowFactory->Create(windowsMetaData);
}

void SolarcApp::Run()
{
	while(m_IsRunning)
	{
		m_Window->Update();
	}
}
