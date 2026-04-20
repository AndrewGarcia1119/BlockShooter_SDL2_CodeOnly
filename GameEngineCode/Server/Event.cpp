#include "Event.hpp"

Event::Event(EventType eventType, int64_t eventSourceUUID, EventParams parameters)
{
	this->eventType = eventType;
	this->eventSourceUUID = eventSourceUUID;
	this->parameters = parameters;
}
