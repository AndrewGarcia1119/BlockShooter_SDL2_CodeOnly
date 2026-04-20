#pragma once

#include "TimeManager.hpp"
#include "PhysicsManager.hpp"
#include "ShapeComponent.hpp"
#include "EventManager.hpp"

extern TimeManager* timeManager;
extern PhysicsManager* physicsManager;
extern TimeManager* gameTimeManager;
extern ShapeComponent* shapeComponent;
extern bool gameRunning;
extern Coord* globalOffset;

void initGlobal();
void quitGame();
