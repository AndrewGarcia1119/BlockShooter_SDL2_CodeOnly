#pragma once
#include <SDL.h>
#include "structs.hpp"
#include "properties.hpp"
//#include "globalMethodsAndVars.hpp"

class PhysicsManager { //objects must contain their current and last positon, and acceleration
public:
	PhysicsManager();

	Coord *gravity;
	void doPhysics(float deltaTIme);
	void sideScroll(int direction); //called on collision with a side boundary
	//void controllable(int64_t UUID, int64_t deltaT);
	bool resolveCollision(Property type, int64_t firstUUID, int64_t secondUUID, Coord *pos, Coord *safeZone, int direction); //pos coord will be removed
	void updateColliders();
	void applyImpulse(int64_t UUID, Coord impulse);
	Coord *add(Coord one, Coord two);
	Coord* subtract(Coord one, Coord two);
	void scalarMult(Coord *vec, float sca);
	float dotProd(Coord one, Coord two);
	int64_t managerUUID;
	EventHandler handler;
private:
	//I am copying some of the physics function stuff here in case this whole property thing doesn't work out
	void updatePosition2(int64_t UUID, float deltaTime); //??
	void accelerate2(int64_t UUID, Coord vec, float deltaTime);
	void update2(float f, int64_t UUID);
	//void handleCollision(EventType eventType, int64_t firstUUID, EventParams otherParams);
};
