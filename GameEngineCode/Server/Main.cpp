#include <zmq.hpp>
#include <SDL.h>
#include <thread>
#include <iostream>
#include "server.hpp"
#include "EventManager.hpp"

EventManager* eventManager;

int main(int argc, char* argv[])
{
    initGlobal();
    initProperties();
    int64_t currentTime = 0;
    int64_t elapsedTime = 0;
    int64_t lastTime = timeManager->getTime();
    eventManager = new EventManager();
    onServerStart();

    while (true) {
        //...get deltaTime
        m.lock();
        currentTime = timeManager->getTime(); //in nanoseconds
        m.unlock();
        elapsedTime = currentTime - lastTime; //time since last frame
        int64_t nsPerFrame = 1000000000 / timeManager->maxFrameRate; // finds the number of nanoseconds per frame
        if (elapsedTime < nsPerFrame) { //if the last frame did not take the correct number of nanoseconds

            SDL_Delay((nsPerFrame - elapsedTime) / 1000000); //delay to reach time limit
           // m.lock();
            currentTime = timeManager->getTime();
            timeManager->deltaTime = (currentTime - lastTime) / 1000000.;// save deltatime for real time in miliseconds

            serverTimeManager->deltaTime = (static_cast<double>(currentTime) - lastTime) / serverTimeManager->getTic(); // save deltatime for game loop in seconds
            //m.unlock();
            lastTime = currentTime;
        }
        else {
            //m.lock();
            timeManager->deltaTime = elapsedTime / 1000000.;
            serverTimeManager->deltaTime = static_cast<double>(elapsedTime) / serverTimeManager->getTic();
            //m.unlock();
            lastTime = currentTime;
        }
        //...
        onServerUpdate();
        eventManager->eventCaller();
    }
    onServerEnd();
    
    return 0;
}

