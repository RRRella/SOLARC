#pragma once
#include "Preprocessor/API.h"

#include "Event/EventQueue.h"

using OwnedEventCallBack  = std::shared_ptr<std::function<void(Event&)>>;
using SharedEventCallBack = std::weak_ptr<std::function<void(Event&)>>;

class ENERF_CORE_API EventComponent
{
public:
	EventComponent(unsigned int numConsumers)
	{
		m_Consumers.resize(numConsumers);
	};

	~EventComponent() = default;

	void SetProducerCallBack(SharedEventCallBack callBack) noexcept
	{
		m_Producers.emplace_back(callBack);
	}

	SharedEventCallBack GetConsumerCallBack(size_t id) noexcept { return m_Consumers[id]; }

protected:
	
	std::list<SharedEventCallBack> m_Producers;

	std::vector<OwnedEventCallBack> m_Consumers;
};