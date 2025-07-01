#pragma once
#include "Preprocessor/API.h"
#include "Window.h"

class ENERF_CORE_API ENerfApp
{
public:

	ENerfApp();
	ENerfApp(const std::string& configDataPath);
	~ENerfApp() = default;

	ENerfApp(ENerfApp&& other) = delete;
	ENerfApp& operator=(ENerfApp&& other) = delete;

	ENerfApp(const ENerfApp& other) = delete;
	ENerfApp& operator=(const ENerfApp& other) = delete;

	static void Initialize();
	static void Initialize(const std::string& configDataPath);

	static ENerfApp& Get()
	{
		if (!m_Instance)
			throw std::runtime_error("ENerfApp not initialized. Call Initialize() first.");

		return *m_Instance;
	}

	void Run();

private:

	inline static std::unique_ptr<ENerfApp> m_Instance = nullptr;

	std::unique_ptr<WindowFactory> m_WindowFactory;

	//temp
	std::unique_ptr<Window> m_Window;

	bool m_IsRunning = true;
};