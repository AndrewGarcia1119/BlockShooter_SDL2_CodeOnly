#include "Main.hpp"
#include "globalMethodsAndVars.hpp"
#include <cmath>
#include <memory>
#include <intrin.h>
#include <thread>
#include <mutex>
#include <condition_variable>
#include "properties.hpp"
#include "ShapeComponent.hpp"


/** Thread for keyboard input.
*/
void inputThread() {
    /*while (gameRunning) {
        readWASDInput();
        
    }*/
}

/** Main method, calls init to initialize renderer and creates a movable shape.
* starts game loop
*/
int main(int argc, char* argv[])
{
    //static TimeManager *timeManager = new TimeManager();
    initGlobal();
    setUpInputEventHandler();
    //Call the initialization functions
    initSDL();

    //start up game
    onGameStart();
    int64_t currentTime = 0;
    int64_t elapsedTime = 0;
    int64_t lastTime = timeManager->getTime();
    bool firstFrame = true;
    readInput();
    //std::thread in(inputThread);
    in = std::thread(inputThread);
    timeManager->setMaxFrameRate(60);

    while (gameRunning)
    {

        ///Temporary: I put input in main thread and it works better
        readInput();
        ///





      //  gameTimeManager->pause();
        currentTime = timeManager->getTime(); //in nanoseconds
        elapsedTime = currentTime - lastTime; //time since last frame
        int64_t nsPerFrame = 1000000000 / timeManager->maxFrameRate; // finds the number of nanoseconds per frame
        if (elapsedTime < nsPerFrame) { //if the last frame did not take the correct number of nanoseconds
            
            SDL_Delay( (nsPerFrame - elapsedTime) / 1000000 ); //delay to reach time limit
           // std::cout << "Delayed ";
            currentTime = timeManager->getTime();
            timeManager->deltaTime = (currentTime - lastTime) / 1000000.;// save deltatime for real time in miliseconds
            gameTimeManager->deltaTime = (static_cast<double>(currentTime) - lastTime) / gameTimeManager->getTic(); // save deltatime for game loop in seconds
            lastTime = currentTime;
        }
        else {
            if (elapsedTime > nsPerFrame * 2) {
                elapsedTime = nsPerFrame;
            }
            timeManager->deltaTime = elapsedTime / 1000000.;
            gameTimeManager->deltaTime = static_cast<double>(elapsedTime) / gameTimeManager->getTic();
            lastTime = currentTime;
        }
       
        //std::cout << gameTimeManager->deltaTime << std::endl;
        
        //Prep the scene
        prepareScene();

        //Process input
        doInput();

        //modify the game world here
        onGameUpdate();
        /////////
        //now do physics
        eventManager->eventCaller();
        physicsManager->doPhysics(gameTimeManager->deltaTime);
        physicsManager->updateColliders();
        /////////

        //render all shapes
        shapeComponent->renderShapes();
        //

        //Present the resulting scene
        presentScene();

 
    }
    //exit(0);
    onGameEnd();
    in.join();
    return 0;
}


