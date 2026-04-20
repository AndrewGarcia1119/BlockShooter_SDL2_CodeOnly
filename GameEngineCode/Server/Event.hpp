#pragma once
#include <variant>
#include <map>
#include "properties.hpp"


class Event {
public:
	EventType eventType;
	int64_t eventSourceUUID;
	//EventParams is defined in properties.hpp. It is a typedef of a list of variants
	EventParams parameters;
	int64_t priority;
	Event(EventType eventType, int64_t eventSourceUUID = -1, EventParams parameters = EventParams());
};