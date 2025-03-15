#pragma once
#include "Preprocessor/API.h"
//#include "Window"

class ENERF_CORE_API ENerfApp
{
public:

	static ENerfApp& Get() noexcept
	{
		if (!m_Instance)
			m_Instance = std::make_unique<ENerfApp>();

		return *m_Instance;
	}

	//Special functions
	ENerfApp();
	~ENerfApp() = default;

	ENerfApp(ENerfApp&& other) = delete;
	ENerfApp& operator=(ENerfApp&& other) = delete;

	ENerfApp(const ENerfApp& other) = delete;
	ENerfApp& operator=(const ENerfApp& other) = delete;
	//

	void Run();

private:

	inline static std::unique_ptr<ENerfApp> m_Instance = nullptr;

	//std::unique_ptr<Window> m_Window;

	bool m_IsRunning = true;
};