#pragma once
#include "Preprocessor/API.h"

class SOLARC_CORE_API WindowPlatform
{
public:
	virtual bool PeekNextMessage() = 0;
	virtual void ProcessMessage() = 0;
	virtual void Update() = 0;

};