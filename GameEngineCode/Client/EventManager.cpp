#include "EventManager.hpp"
#include <iostream>
#include <queue>
#include "Event.hpp"
#include "globalMethodsAndVars.hpp"

#define MAX_EVENTS_PER_UPDATE 100

//values that can be added to an event's raise time to get its priority
#define HIGH_PRIORITY 0
#define MEDIUM_PRIORITY 2
#define LOW_PRIORITY 4

EventManager::EventManager()
{
	handlers = std::map<EventType, std::list<EventHandler*>>();
	handlers.insert({ InputEvent, std::list<EventHandler*>() });
	handlers.insert({ CollisionEvent, std::list<EventHandler*>() });
	handlers.insert({ DeathEvent, std::list<EventHandler*>() });
	handlers.insert({ SpawnEvent, std::list<EventHandler*>() });



	//We have not tried making a thread as a variable in a class, so this may not work. We may need to move this thread outside the EventManager class if this does not work.
	//found this at https://stackoverflow.com/questions/17472827/create-thread-inside-class-with-function-from-same-class
	//eventCallThread = std::thread(&EventManager::eventCaller, this); 
}

void EventManager::registerEvent(std::list<EventType> events, EventHandler* handler)
{
	for (auto eventType : events) {
		handlers.at(eventType).push_back(handler);
		//std::cout << "num handlers: " << handlers.at(eventType).size() << std::endl;
	}
}

void EventManager::deregisterEvent(std::list<EventType> events, EventHandler* handler)
{
	for (auto eventType : events) {
		handlers.at(eventType).remove(handler);
	}
}

void EventManager::raise(Event event)
{
	int64_t raiseTime = gameTimeManager->getTime();
	if (event.eventType == InputEvent) {
		event.priority = raiseTime + HIGH_PRIORITY;
	}
	else if (event.eventType == CollisionEvent) {
		event.priority = raiseTime + MEDIUM_PRIORITY;
	}
	else if (event.eventType == DeathEvent) {
		event.priority = raiseTime + LOW_PRIORITY;
	}
	else if (event.eventType == SpawnEvent) {
		event.priority = raiseTime + LOW_PRIORITY;
	}
	raised_events.push(event);
}

void EventManager::eventCaller()
{
		//std::cout << "num events: " << raised_events.size() << std::endl;
		for (int i = 0; i < MAX_EVENTS_PER_UPDATE; i++) {
			if (raised_events.empty()) break;
			//std::cout << "attempting to run event handler" << std::endl;
			for (auto handler : handlers.at(raised_events.top().eventType)) {
				//std::cout << "attempting to run event handler" << std::endl;
				if (handler->eventHandleMethod)
					handler->eventHandleMethod(raised_events.top().eventType, raised_events.top().eventSourceUUID, raised_events.top().parameters);
			}
			raised_events.pop();
			
		}
		
}
