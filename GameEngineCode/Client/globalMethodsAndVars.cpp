#include "globalMethodsAndVars.hpp"
#include "PhysicsManager.hpp"

//global time manager
TimeManager* timeManager;

PhysicsManager* physicsManager;

TimeManager* gameTimeManager;

ShapeComponent* shapeComponent;

//eventManager extern var is declared in EventManager.hpp rather than globalMethodsAndVars.hpp so that eventManager could be used in physicsManager.
EventManager* eventManager;

Coord* globalOffset;

bool gameRunning;

int64_t uuidVal;


//initializes all global variables
void initGlobal()
{
	initProperties();
	uuidVal = 0;
	timeManager = new TimeManager();
	eventManager = new EventManager();
	physicsManager = new PhysicsManager();
	shapeComponent = new ShapeComponent();
	
	gameRunning = true;
	globalOffset = new Coordinate(0, 0);
	//tic is approximatley 1/60 second when time is stored in nanoseconds
	gameTimeManager =  new TimeManager(timeManager, 10000000);
}


void quitGame() {
	gameRunning = false;
}

