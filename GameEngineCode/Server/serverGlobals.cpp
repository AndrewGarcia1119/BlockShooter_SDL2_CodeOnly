#include "serverGlobals.hpp"

//global time manager
TimeManager* timeManager;

TimeManager* serverTimeManager;

std::mutex m;
int64_t uuidVal;

//initializes all global variables
void initGlobal()
{
	timeManager = new TimeManager();
	uuidVal = 0;
	//tic is approximatley 1/60 second when time is stored in nanoseconds
	serverTimeManager = new TimeManager(timeManager, 10000000);
}
