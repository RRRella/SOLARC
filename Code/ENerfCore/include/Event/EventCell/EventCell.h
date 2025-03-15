#pragma once
#include "Preprocessor/API.h"

#include "Event/EventComponent.h"

struct ENERF_CORE_API EventConsumerRole
{
	uint64_t roleID;
	EventComponent* eventComponent;
	size_t callBackIndex;
};

struct ENERF_CORE_API EventProducerRole
{
	uint64_t roleID;
	EventComponent* eventComponent;
};

class ENERF_CORE_API EventCell
{
public:
	EventCell(size_t numProducerRoles , size_t numConsumerRoles)
	{
		m_Producers.resize(numProducerRoles);
		m_Consumers.resize(numConsumerRoles);
	}

	virtual ~EventCell() = default;

	virtual void Communicate() = 0;

	void RegisterProducerRole(EventProducerRole role) noexcept
	{
		role.eventComponent->SetProducerCallBack(m_Producers[role.roleID]);
	}
	void RegisterConsumerRole(EventConsumerRole role) noexcept
	{
		m_Consumers[role.roleID] = role.eventComponent->GetConsumerCallBack(role.callBackIndex);
	}

protected:
	std::vector<OwnedEventCallBack> m_Producers;
	std::vector<SharedEventCallBack> m_Consumers;
};