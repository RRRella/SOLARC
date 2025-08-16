#include "SolarcApp.h"
#include "spdlog/spdlog.h"
#include "Utility/CompileTimeUtil.h"

void SolarcApp::Initialize(const std::string& configDataPath)
{
	if (!m_Instance)
		m_Instance = std::make_unique<SolarcApp>(configDataPath);
	else
		throw std::runtime_error("SolarcApp already initialized");
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

	ParseWindowData(configData);
	ParseMTData(configData);
}

void SolarcApp::Run()
{
	while(m_IsRunning)
	{
		m_Window->Update();
	}
}

void SolarcApp::ParseWindowData(const json& configData)
{
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

void SolarcApp::ParseMTData(const json& configData)
{
	const json& MTData = configData["MTData"];

	struct Order
	{
		std::string key;
		uint8_t value;
		double factor;
	};

	std::vector<Order> order;
	order.reserve(MTData.size());

	unsigned int ThreadCount = std::thread::hardware_concurrency();
	unsigned int totalFactor = 0;

	//Parse factors and calculate exact allocations
	struct AllocationInfo {
		size_t index;
		double remainder;
	};

	std::vector<AllocationInfo> remainders;
	remainders.reserve(MTData.size());

	unsigned int allocated = 0;
	size_t idx = 0;

	for (const auto& [key, factorJson] : MTData.items())
	{
		int factor = factorJson.get<int>();
		totalFactor += factor;

		double exactAlloc = (factor * ThreadCount) / 100.0;
		uint8_t baseAlloc = static_cast<uint8_t>(std::floor(exactAlloc));
		allocated += baseAlloc;

		order.push_back({ key, baseAlloc, static_cast<double>(factor) });

		double rem = exactAlloc - baseAlloc;
		remainders.push_back({ idx++, rem });
	}

	//Ensure total factor = 100
	assert(totalFactor == 100, "Thread distribution factor must equal 100");

	//Distribute leftovers based on largest remainders
	unsigned int leftover = ThreadCount - allocated;

	std::sort(remainders.begin(), remainders.end(),
		[](const AllocationInfo& a, const AllocationInfo& b) {
			if (a.remainder == b.remainder) return a.index < b.index;
			return a.remainder > b.remainder;
		});

	for (unsigned int i = 0; i < leftover; ++i) {
		order[remainders[i].index].value += 1;
	}

	for (auto& item : order)
		m_ThreadCounts[item.key] = item.value;
}
