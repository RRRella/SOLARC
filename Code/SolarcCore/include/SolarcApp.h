#pragma once
#include "Preprocessor/API.h"
#include "Window.h"
#include <nlohmann/json.hpp>
using json = nlohmann::json;

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

	static SolarcApp& Get()
	{
		if (!m_Instance)
			throw std::runtime_error("SolarcApp not initialized. Call Initialize() first.");

		return *m_Instance;
	}

	uint8_t GetThreadCountFor(const std::string& systemComponent) { return m_ThreadCounts[systemComponent]; }

	void Run();

private:
	void ParseWindowData(const json& configData);
	void ParseMTData(const json& configData);

	inline static std::unique_ptr<SolarcApp> m_Instance = nullptr;

	std::unordered_map<std::string, uint8_t> m_ThreadCounts;

	//temp
	std::unique_ptr<Window> m_Window;

	bool m_IsRunning = true;
};