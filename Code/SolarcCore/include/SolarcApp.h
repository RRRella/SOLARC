#pragma once
#include "Preprocessor/API.h"
#include "Window.h"

class SOLARC_CORE_API SolarcApp
{
public:

	SolarcApp();
	SolarcApp(const std::string& configDataPath);
	~SolarcApp() = default;

	SolarcApp(SolarcApp&& other) = delete;
	SolarcApp& operator=(SolarcApp&& other) = delete;

	SolarcApp(const SolarcApp& other) = delete;
	SolarcApp& operator=(const SolarcApp& other) = delete;

	static void Initialize();
	static void Initialize(const std::string& configDataPath);

	static SolarcApp& Get()
	{
		if (!m_Instance)
			throw std::runtime_error("SolarcApp not initialized. Call Initialize() first.");

		return *m_Instance;
	}

	void Run();

private:

	inline static std::unique_ptr<SolarcApp> m_Instance = nullptr;

	std::unique_ptr<WindowFactory> m_WindowFactory;

	//temp
	std::unique_ptr<Window> m_Window;

	bool m_IsRunning = true;
};