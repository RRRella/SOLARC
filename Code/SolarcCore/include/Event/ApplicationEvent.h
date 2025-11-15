#pragma once
#include "Preprocessor/API.h"
#include "Event/Event.h"

class SOLARC_CORE_API ApplicationEvent : public Event
{
public:
    enum class TYPE
    {
        INITIALIZE_COMPLETE,
        STAGING_COMPLETE,
        LOADING_COMPLETE,
        RUNNING_COMPLETE,
        CLEANUP_COMPLETE
    };

    ApplicationEvent(ApplicationEvent::TYPE type)
        : m_ApplicationEventType(type), Event(Event::TYPE::APPLICATION_EVENT)
    {
    }
    ~ApplicationEvent() = default;

    TYPE GetApplicationEventType() const { return m_ApplicationEventType; }

private:
    TYPE m_ApplicationEventType;
};

// --- Concrete Events ---

class InitializeCompleteEvent : public ApplicationEvent
{
public:
    InitializeCompleteEvent() : ApplicationEvent(ApplicationEvent::TYPE::INITIALIZE_COMPLETE) {}
};

class StagingCompleteEvent : public ApplicationEvent
{
public:
    StagingCompleteEvent(const std::string& projectPath = "")
        : ApplicationEvent(ApplicationEvent::TYPE::STAGING_COMPLETE), m_ProjectPath(projectPath) {
    }

    const std::string& GetProjectPath() const { return m_ProjectPath; }

private:
    std::string m_ProjectPath;
};

class LoadingCompleteEvent : public ApplicationEvent
{
public:
    LoadingCompleteEvent() : ApplicationEvent(ApplicationEvent::TYPE::LOADING_COMPLETE) {}
};

enum class PostRunAction
{
    SHUTDOWN,
    RESTART,
    OPEN_NEW_PROJECT
};

class RunningCompleteEvent : public ApplicationEvent
{
public:
    RunningCompleteEvent(PostRunAction action)
        : ApplicationEvent(ApplicationEvent::TYPE::RUNNING_COMPLETE)
        , m_Action(action)
    {
    }

    PostRunAction GetAction() const { return m_Action; }

private:
    PostRunAction m_Action;
};

class CleanupCompleteEvent : public ApplicationEvent
{
public:
    CleanupCompleteEvent() : ApplicationEvent(ApplicationEvent::TYPE::CLEANUP_COMPLETE) {}
};