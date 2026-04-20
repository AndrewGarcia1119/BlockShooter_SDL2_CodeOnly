#pragma once
#include "Event.hpp"
#include <vector>
#include <list>
#include <thread>
#include <queue>


class EventManager {
public:
	EventManager();
	void registerEvent(std::list<EventType> events, EventHandler* handler);
	void deregisterEvent(std::list<EventType> events, EventHandler* handler);
	void raise(Event event);
	void eventCaller();
private:
	class Compare {
	public:
		bool operator() (Event e1, Event e2) {
			if (e1.priority > e2.priority) {
				return true;
			}
			else {
				return false;
			}
		}
	};
	std::map < EventType, std::list<EventHandler* >> handlers;
	std::priority_queue<Event, std::vector<Event>, Compare> raised_events;
};

extern EventManager* eventManager;