#pragma once

#include "eventpp/eventdispatcher.h"
#include <cstdint>

namespace ACTGame
{
enum class EventType : std::int32_t
{
    SceneCreated = 0,
    EntityCreated = 1,
};

struct EventParamHolder
{
    int Type;
};

struct EventPolicies
{
    static int getEvent(const EventParamHolder& e)
    {
        return e.Type;
    }
};

template <typename Arg>
struct EventHandler
{
    using handler_t = eventpp::EventDispatcher<int, void(Arg), EventPolicies>::Handle;
    handler_t Handler;
    EventType EventId;
};

class EventService
{
public:
    template <typename Arg>
    auto RegisterHandler(int id, std::function<void(Arg)> Handler)
    {
        //using ArgType = typename std::decay_t<decltype(Handler)>::argument_type;
        auto& dispatcher = GetDispatcher<Arg>();
        return dispatcher.appendListener(id, Handler);
    }

    template <typename Arg>
    void RemoveHandler(const EventHandler<Arg>& handler)
    {
        auto& dispatcher = GetDispatcher<Arg>();
        dispatcher.removeListener((int)handler.EventId, handler.Handler);
    }

    template <typename EventType>
    void Emit(int id, EventType data)
    {
        GetDispatcher<EventType>().dispatch(id, data);
    }

private:
    template <typename T>
    eventpp::EventDispatcher<int, void(T), EventPolicies>& GetDispatcher()
    {
        static eventpp::EventDispatcher<int, void(T), EventPolicies> instance;
        return instance;
    }
};
}
