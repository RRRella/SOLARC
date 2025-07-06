#pragma once
#include "Preprocessor/API.h"

#include "Event/EventQueue.h"

#include "Utility/UUID.h"

using SharedEventCallBack  = std::shared_ptr<std::function<void(Event&)>>;
using WeakEventCallBack = std::weak_ptr<std::function<void(Event&)>>;

class SOLARC_CORE_API EventComponent
{
public:
	explicit EventComponent(const uint64_t numConsumers)
	{
		m_Consumers.resize(numConsumers);

		GeenerateUUID(m_UUID);
	};

	~EventComponent() = default;

	void SetProducerCallBack(WeakEventCallBack callBack) noexcept
	{
		m_Producers.emplace_back(callBack);
	}

	uint64_t GetEventConsumersCount() const noexcept { return m_Consumers.size(); }

	UUID GetID() const noexcept { return m_UUID; }

	WeakEventCallBack GetConsumerCallBack(size_t id) noexcept { return m_Consumers[id]; }

protected:
	
	std::list<WeakEventCallBack> m_Producers;

	std::vector<SharedEventCallBack> m_Consumers;

	UUID m_UUID = {};
};