#include "Window.h"

void Window::Update()
{
	if (m_Platform->PeekNextMessage())
	{
		m_Platform->ProcessMessage();
	}

	m_Platform->Update();
}