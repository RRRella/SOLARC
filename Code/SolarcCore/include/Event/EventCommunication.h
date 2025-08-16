#pragma once
#include "Preprocessor/API.h"
#include "Utility/UUID.h"
#include "Event/EventQueue.h"

enum class EVENT_ROLE_TYPE
{
	PRODUCER,
	CONSUMER
};

class SOLARC_CORE_API EventCell
{
public:
	EventCell()
	{
		GeenerateUUID(m_UUID);
	}

	virtual ~EventCell() = default;

	virtual void Communicate() = 0;

	UUID GetID() const noexcept { return m_UUID; }

	friend class EventComponent;

protected:
	UUID m_UUID;

	class SOLARC_CORE_API EventRegisterBlock
	{
	public:
		virtual ~EventRegisterBlock() = default;

		EventRegisterBlock(const EventRegisterBlock& other) = delete;
		EventRegisterBlock& operator=(const EventRegisterBlock& other) = delete;

		EventRegisterBlock(EventRegisterBlock&& other) = default;
		EventRegisterBlock& operator=(EventRegisterBlock&& other) = default;

		bool IsRegistered() const { std::lock_guard lock(m_Mtx); return m_IsRegistered; }
		EVENT_ROLE_TYPE GetRoleType() const { return m_RoleType; }
		UUID GetEventComponentID() const;
		virtual void Unregister(/*UUID type is temproary*/const UUID& key) = 0;

	protected:

		EventRegisterBlock(const EVENT_ROLE_TYPE& type) :
			m_RoleType(type)
		{
		};

		EVENT_ROLE_TYPE m_RoleType;
		UUID m_EventComponentID;
		bool m_IsRegistered = false;

		mutable std::mutex m_Mtx;
	};

	class SOLARC_CORE_API ProducerRegisterBlock : public EventRegisterBlock
	{
	public:
		ProducerRegisterBlock() : EventRegisterBlock(EVENT_ROLE_TYPE::PRODUCER) {}
		virtual ~ProducerRegisterBlock() = default;

		ProducerRegisterBlock(const ProducerRegisterBlock& other) = delete;
		ProducerRegisterBlock& operator=(const ProducerRegisterBlock& other) = delete;

		ProducerRegisterBlock(ProducerRegisterBlock&& other) = default;
		ProducerRegisterBlock& operator=(ProducerRegisterBlock&& other) = default;

		std::shared_ptr<const Event> GetEvent();
		void Register(EventComponent& eventComponent, EventCell* eventCell);
		void Unregister(const UUID& eventComponentID) override;

	private:

		std::shared_ptr<ProducerEventQueue> m_EventQueue;
	};

	class SOLARC_CORE_API ConsumerRegisterBlock : public EventRegisterBlock
	{
	public:
		ConsumerRegisterBlock() : EventRegisterBlock(EVENT_ROLE_TYPE::CONSUMER) {}
		virtual ~ConsumerRegisterBlock() = default;

		ConsumerRegisterBlock(const ConsumerRegisterBlock& other) = delete;
		ConsumerRegisterBlock& operator=(const ConsumerRegisterBlock& other) = delete;

		ConsumerRegisterBlock(ConsumerRegisterBlock&& other) = default;
		ConsumerRegisterBlock& operator=(ConsumerRegisterBlock&& other) = default;

		void PushEvent(std::shared_ptr<const Event> e);

		void Register(EventComponent& eventComponent);
		void Unregister(const UUID& eventComponentID) override;

	private:
		
		std::weak_ptr<ConsumerEventQueue> m_EventQueue;
	};

};

class SOLARC_CORE_API EventComponent
{
public:
	//For now I'm gonna stick with the friend mechanism
	//Maybe I will change the design later
	friend EventCell::ProducerRegisterBlock;
	friend EventCell::ConsumerRegisterBlock;
	//////////////

	virtual ~EventComponent() = default;

	UUID GetEventCommunicationID() const noexcept { return m_UUID; }

protected:
	EventComponent()
	{
		GeenerateUUID(m_UUID);
	};

	UUID m_UUID = {};


	std::list<std::weak_ptr<ProducerEventQueue>> m_ProducerQueues;
	std::shared_ptr<ConsumerEventQueue> m_EventQueue;


private:

	//This mutex and function are temproary
	//I need to make a thread safe doubly linked list later
	//And create producer queues with that
	mutable std::mutex m_Mtx;
	void RegisterProducerRole(std::weak_ptr<ProducerEventQueue> queue) noexcept
	{
		std::lock_guard lock(m_Mtx);
		m_ProducerQueues.emplace_back(queue);
	}
};
