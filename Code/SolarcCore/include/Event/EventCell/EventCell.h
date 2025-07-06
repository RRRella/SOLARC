#pragma once
#include "Preprocessor/API.h"

#include "Event/EventComponent.h"

#include "Utility/UUID.h"

struct SOLARC_CORE_API RegisteredDispatcher
{
	RegisteredDispatcher(SharedEventCallBack inCallBack, UUID inComponentID)
		:callBack(inCallBack), componentID(inComponentID), isRegistered(true){}

	SharedEventCallBack callBack;
	UUID componentID;
	bool isRegistered = false;
};

struct SOLARC_CORE_API RegisteredCatcher
{
	RegisteredCatcher(WeakEventCallBack inCallBack, UUID inComponentID)
		:callBack(inCallBack), componentID(inComponentID), isRegistered(true) {}

	WeakEventCallBack callBack;
	UUID componentID;
	bool isRegistered = false;
};

using SharedRegDispatcher = std::shared_ptr<RegisteredDispatcher>;
using SharedRegCatcher = std::shared_ptr<RegisteredCatcher>;


enum class SOLARC_CORE_API EVENT_ROLE_TYPE : short
{
	DISPATCHER,
	CATCHER
};

struct SOLARC_CORE_API EventRegisterInfo
{
	EVENT_ROLE_TYPE roleTypeInCell;
	uint8_t roleIDInCell;
	UUID eventCellID;
	UUID eventComponentID;
};

class SOLARC_CORE_API EventCell
{
public:
	EventCell(uint8_t numDispatcherRoles , uint8_t numCatcherRoles)
	{
		m_Dispatchers.resize(numDispatcherRoles);
		m_RegisteredDispatchers.resize(numDispatcherRoles);
		m_RegisteredCatchers.resize(numCatcherRoles);

		GeenerateUUID(m_UUID);
	}

	virtual ~EventCell() = default;

	virtual void Communicate() = 0;

	UUID GetID() const noexcept { return m_UUID; }

	EventRegisterInfo RegisterDispatcher(uint8_t roleID , EventComponent* eventComponent) noexcept
	{
		m_RegisteredDispatchers[roleID] = std::make_shared<RegisteredDispatcher>
											(
												std::make_shared<std::function<void(Event&)>>(m_Dispatchers[roleID]) , eventComponent->GetID()
											);

		eventComponent->SetProducerCallBack(m_RegisteredDispatchers[roleID]->callBack);

		return EventRegisterInfo{ EVENT_ROLE_TYPE::DISPATCHER , roleID, m_UUID ,  eventComponent->GetID()};
	}

	EventRegisterInfo RegisterCatcher(uint8_t roleID, EventComponent* eventComponent, uint8_t callBackIndex) noexcept
	{
		m_RegisteredCatchers[roleID] = std::make_shared<RegisteredCatcher>
										(
											eventComponent->GetConsumerCallBack(callBackIndex) , eventComponent->GetID()
										);

		return EventRegisterInfo{ EVENT_ROLE_TYPE::CATCHER , roleID, m_UUID ,  eventComponent->GetID() };
	}

	void UnregisterDispatcher(uint8_t roleID)
	{
		if (m_RegisteredDispatchers[roleID])
		{
			if (m_RegisteredDispatchers[roleID]->isRegistered)
			{
				m_RegisteredDispatchers[roleID]->isRegistered = false;
				m_RegisteredDispatchers[roleID].reset();
			}
		}
	}

	void UnregisterCatcher(uint8_t roleID)
	{
		if (m_RegisteredCatchers[roleID])
		{
			if (m_RegisteredCatchers[roleID]->isRegistered)
			{
				m_RegisteredCatchers[roleID]->isRegistered = false;
				m_RegisteredCatchers[roleID].reset();
			}
		}
	}

	UUID QueryRoleComponentID(EVENT_ROLE_TYPE roleType , uint8_t roleID)
	{
		switch (roleType)
		{
			case EVENT_ROLE_TYPE::DISPATCHER:
			{
				if (m_RegisteredDispatchers[roleID])
				{
					if (m_RegisteredDispatchers[roleID]->isRegistered)
						return m_RegisteredDispatchers[roleID]->componentID;
				}

				return UUID{};
			}
			case EVENT_ROLE_TYPE::CATCHER:
			{
				if (m_RegisteredCatchers[roleID])
				{
					if (m_RegisteredCatchers[roleID]->isRegistered)
						return m_RegisteredCatchers[roleID]->componentID;
				}

				return UUID{};
			}
		}
	}

	bool IsRegistered(EVENT_ROLE_TYPE roleType, uint8_t roleID) const noexcept
	{
		switch (roleType)
		{
			case EVENT_ROLE_TYPE::DISPATCHER:
			{
				if(m_RegisteredDispatchers[roleID])
					return m_RegisteredDispatchers[roleID]->isRegistered;

				return false;
			}
			case EVENT_ROLE_TYPE::CATCHER:
			{
				if (m_RegisteredCatchers[roleID])
					return m_RegisteredCatchers[roleID]->isRegistered;

				return false;
			}
		}
	}


protected:
	std::vector<SharedRegDispatcher> m_RegisteredDispatchers;
	std::vector<SharedRegCatcher>   m_RegisteredCatchers;

	std::vector<std::function<void(Event&)>> m_Dispatchers;

	UUID m_UUID;
};