#pragma once

//code borrowed from timeline slides.

#include <mutex>
#include <SDL.h>

//This class handles all time related features of the game engine
class TimeManager {
    public:
		double deltaTime; //difference in time between every frame
		double timeScale;
		TimeManager(TimeManager* anchor, int64_t tic);
		TimeManager(); //optional, may not be included
		int64_t getTime(); //this can be game or system time implementation
		void pause();
		void unpause();
		void changeTic(int tic); //optional
		bool isPaused(); //optional
		int64_t maxFrameRate; //maximum frame rate
		void setMaxFrameRate(int64_t frameRate);
		int64_t getTic();
		void setTimeScale(double newScale);
		
private:
	std::mutex m; //if tics can change size and the game is multithreaded
	int64_t start_time; //the time of the *anchor when created
	int64_t elapsed_paused_time;
	int64_t last_paused_time;
	int64_t tic; //units of anchor timeline per step
	bool paused;
	TimeManager* anchor; //for most general game time, system library pointer
	
};