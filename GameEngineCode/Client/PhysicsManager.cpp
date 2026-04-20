#include "PhysicsManager.hpp"
#include "input.hpp"
#include "properties.hpp"
#include "EventManager.hpp"
#include "Event.hpp"
#include <iostream>;
#include <zmq.hpp>



/** adapted from Verlet Integration Physics Engine created by Jean Tampon
* https://github.com/johnBuffer/VerletSFML-Multithread
*/




/** Called each frame to implement physics on objects with a physics component
*/
void PhysicsManager::doPhysics(float deltaTime)
{
	//gets both key and value of each entry in map at physics entry
	for (auto const& [UUID, Att] :propertyMap.at(PHYSICS)) {
		update2(deltaTime, UUID);
	}
}

/** Called on collision with a boundary object, transitions scene by 
	moving all objects in the given direction
*/
void PhysicsManager::sideScroll(int direction)
{
	globalOffset->x -= 10 * direction;
	for (auto const& [UUID, Att] : propertyMap.at(POSITION)) { //for all objects with a position
		bool scroll = false;
		try {
			auto& contAtt = propertyMap.at(CONTROL).at(UUID); //get control attribute
		}
		catch (...) {
			scroll = true;
		}

		try {
			auto& sideAtt = propertyMap.at(SIDE).at(UUID);
			scroll = false;
		}
		catch (...) {
			scroll = true;
		}
		if (scroll) { //if not a controllable object
			Coord pos = std::get<Coord>(Att); //position
			pos.x -= 10 * direction; //move in direction
			propertyMap.at(POSITION).at(UUID) = pos; //reassign postion
		}
	}
}



/** If an object has the collision property, will resolve collision by updating position with a pointer to the object position.
*/
bool PhysicsManager::resolveCollision(Property type, int64_t firstUUID, int64_t secondUUID, Coord *tempUpdateCoord, Coord *safeZone, int direction) {
	SDL_FRect* overlap = new SDL_FRect();
	SDL_FRect collider1;
	SDL_FRect collider2;
	bool collided = false;

	auto& firstAttributes = propertyMap.at(COLLISION).at(firstUUID); //check for first obj collider property
	auto& secondAttributes = propertyMap.at(COLLISION).at(secondUUID); //check for second obj collider property

	if (std::holds_alternative<ColliderAttribute>(firstAttributes) && std::holds_alternative<ColliderAttribute>(secondAttributes)) { //checks if attribute is collider
		//get collider from attribute
		collider1 = std::get<ColliderAttribute>(firstAttributes).rectCollider;
		collider2 = std::get<ColliderAttribute>(secondAttributes).rectCollider;

		collided = SDL_HasIntersectionF(&collider1, &collider2);

	}

	if (!collided) {
		return false;
	}
	if (type == COLLISION) {
		SDL_IntersectFRect(&collider1, &collider2, overlap);
		Event e = Event(CollisionEvent, firstUUID);
		e.parameters.push_back(tempUpdateCoord);
		e.parameters.push_back(secondUUID);
		e.parameters.push_back(overlap);
		e.parameters.push_back(collider1);
		e.parameters.push_back(collider2);
		eventManager->raise(e);
	}
	else if (type == DEATH) {
		Event e = Event(DeathEvent, firstUUID);
		e.parameters.push_back(safeZone);
		eventManager->raise(e);
	}
	else if (type == SIDE) {
		sideScroll(direction);
	}
	
	return true;
}

/** 
*/
void PhysicsManager::updateColliders()
{
	Coord posProperty;
	for (auto const& [UUID, Att] : propertyMap.at(COLLISION)) {
		posProperty = std::get<Coord>(propertyMap.at(POSITION).at(UUID));
		ColliderAttribute ca = std::get<ColliderAttribute>(Att);
		ca.rectCollider.x = posProperty.x;
		ca.rectCollider.y = posProperty.y;
		propertyMap.at(COLLISION).at(UUID) = ca;
	}
	
}

// math

Coord* PhysicsManager::add(Coord one, Coord two)
{
	return new Coord(one.x + two.x, one.y + two.y);
}

/** subtracts seconds coord from first coord
*/
Coord* PhysicsManager::subtract(Coord one, Coord two)
{
	return new Coord(one.x - two.x, one.y - two.y);
}

void PhysicsManager::scalarMult(Coord* vec, float sca)
{
	vec->x *= sca;
	vec->y *= sca;
}

float PhysicsManager::dotProd(Coord one, Coord two)
{
	return (one.x * two.x) + (one.y * two.y);
}

void PhysicsManager::updatePosition2(int64_t UUID, float deltaTime)
{
	PhysicsAttribute pa = std::get<PhysicsAttribute>(propertyMap.at(PHYSICS).at(UUID));
	Coord posProperty = std::get<Coord>(propertyMap.at(POSITION).at(UUID));
	Coord* vel = PhysicsManager::subtract(posProperty, pa.positionPrev);
	
	vel->x = 0;
	if (vel->y < -pa.jumpHeight) { vel->y = -pa.jumpHeight; } //moving down

	pa.positionPrev = posProperty;

	//position = position + velocity * change in time
	PhysicsManager::scalarMult(&pa.acceleration, deltaTime * deltaTime); //update acceleration
	posProperty = *(PhysicsManager::add(*PhysicsManager::add(posProperty, *vel), pa.acceleration));

	//reset acceleration
	
	pa.acceleration.x = 0.0;
	pa.acceleration.y = 0.0;
	propertyMap.at(PHYSICS).at(UUID) = pa;
	propertyMap.at(POSITION).at(UUID) = posProperty;
}

void PhysicsManager::accelerate2(int64_t UUID, Coord vec, float deltaTime)
{
	PhysicsAttribute pa = std::get<PhysicsAttribute>(propertyMap.at(PHYSICS).at(UUID));
	pa.acceleration.x = 0;
	pa.acceleration.y = pa.acceleration.y + vec.y * deltaTime;
	propertyMap.at(PHYSICS).at(UUID) = pa;
	//obj->setAcceleration(0, obj->acceleration->y + vec.y * deltaTime);
}

void PhysicsManager::update2(float deltaTime, int64_t UUID)
{
	if (deltaTime != 0) {
		float idk = deltaTime / 100;
		//gotta be replaced with a global grav variable, physicsManager->gravity
		PhysicsManager::accelerate2(UUID, *gravity, idk);
		PhysicsManager::updatePosition2(UUID, idk);
	}
}

void handleCollision(EventType eventType, int64_t collidedUUID, EventParams otherParams)
{
	
	if (eventType == CollisionEvent) {
		Coord* tempUpdateCoord = std::get<Coord*>(otherParams.at(0));
		int64_t otherUUID = std::get<int64_t>(otherParams.at(1));
		SDL_FRect* overlap = std::get<SDL_FRect*>(otherParams.at(2));
		SDL_FRect collider1 = std::get<SDL_FRect>(otherParams.at(3));
		SDL_FRect collider2 = std::get<SDL_FRect>(otherParams.at(4));
		//SDL_FRect* overlap = new SDL_FRect();
		Coord movement;
		SDL_IntersectFRect(&collider1, &collider2, overlap);
		if (overlap->w < overlap->h) {
			if (collider1.x <= collider2.x) {
				movement.x = -(overlap->w);
			}
			else {
				movement.x = overlap->w;
			}
		}
		else {
			if (collider1.y <= collider2.y) {
				movement.y = -(overlap->h);
			}
			else {
				movement.y = overlap->h;
			}
		}
		//coord variable temporarily holds object changes while program is still object oriented
		tempUpdateCoord->x += movement.x;
		tempUpdateCoord->y += movement.y;
	}
	
}


PhysicsManager::PhysicsManager()
{
	
	managerUUID = getNewUUID();
	addComponent(managerUUID, EVENTHANDLER);
	propertyMap.at(EVENTHANDLER).at(managerUUID) = EventHandler();
	handler = std::get<EventHandler>(propertyMap.at(EVENTHANDLER).at(managerUUID));
	handler.eventHandleMethod = handleCollision;
	std::list<EventType> list = std::list<EventType>();
	list.push_back(CollisionEvent);
	//list.push_back(DeathEvent);
	eventManager->registerEvent(list, &handler);
	gravity = new Coord(0, 80000);
}