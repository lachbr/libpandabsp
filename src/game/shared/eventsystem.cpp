#include "eventsystem.h"
#include "eventHandler.h"

IMPLEMENT_CLASS( EventSystem )

const char *EventSystem::get_name() const
{
	return "EventSystem";
}

bool EventSystem::initialize()
{
	return true;
}

void EventSystem::shutdown()
{
}

void EventSystem::update( double frametime )
{
	EventHandler::get_global_event_handler()->process_events();
}
