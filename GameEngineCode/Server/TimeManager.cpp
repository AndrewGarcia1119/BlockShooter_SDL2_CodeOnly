#include "TimeManager.hpp"
#include <intrin.h>
#include <chrono>

//empty for now, but more will be added here when more features are planned for the time manager
using namespace std::chrono;

/** Time manager constructor
*	@param anchor the timeline this TimeManager is anchored to. Will be null for real time TimeManager
*	@param tic size of the time interval
*/
TimeManager::TimeManager(TimeManager* anchor, int64_t tic)
{
	this->anchor = anchor;
	changeTic(tic);
	if (!anchor) {
		time_point<high_resolution_clock> start_time_point = high_resolution_clock::now();

		//I put it all in one line so that there is a smaller rounding error from going straight to the count rather storing the milliseconds in a variable, then getting the count from the variable.
		//I am also using nanoseconds rather than milliseconds
		start_time = duration_cast<nanoseconds>(start_time_point.time_since_epoch()).count();
	}
	else {
		start_time = anchor->getTime();
	}
	elapsed_paused_time = 0;
	last_paused_time = -1;
	paused = false;
	deltaTime = 0;
	timeScale = 1;
	maxFrameRate = 60;
}

TimeManager::TimeManager() : TimeManager(NULL, 1) {}

int64_t TimeManager::getTime()
{
	if (anchor) {
		return (anchor->getTime() - start_time) / tic;
	}
	else {
		return (duration_cast<nanoseconds>(high_resolution_clock::now().time_since_epoch()).count() - start_time) / tic;
	}
}

void TimeManager::pause()
{
	paused = true;
	last_paused_time = this->getTime();
}

void TimeManager::unpause()
{
	paused = false;
	elapsed_paused_time = this->getTime() - this->last_paused_time;
}

void TimeManager::changeTic(int tic)
{
	this->tic = tic;
}

bool TimeManager::isPaused()
{
	return paused;
}

void TimeManager::setMaxFrameRate(int64_t frameRate)
{
	maxFrameRate = frameRate;
}

int64_t TimeManager::getTic()
{
	return tic;
}

void TimeManager::setTimeScale(double newScale)
{
	double prevScale = this->timeScale;
	this->timeScale = newScale;
	//only change tic size if a new scale was selected
	if (!(prevScale == newScale)) {
		//change tic back to baseline
		if (prevScale == .5) {
			changeTic(getTic() / 2);
		}
		else if (prevScale == 2) {
			changeTic(getTic() * 2);
		}

		//edit tic size based on selected scale
		if (newScale == .5) {
			changeTic(getTic() * 2);
		}
		else if (newScale == 2) {
			changeTic(getTic() / 2);
		}
	}
}

