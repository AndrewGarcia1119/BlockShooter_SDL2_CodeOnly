#include "input.hpp"
#include "structs.hpp"
#include "EventManager.hpp"
#include "iostream"



/*
* This function is boilerplate, and the contents currently read the events thrown by SDL and look
* for the SDL quit event, terminating the program. This file is where code to read the keyboard 
* state and determine which keys are pressed should sit.
*/

int controls[CONTROL_COUNT] = { SDL_SCANCODE_W, SDL_SCANCODE_A, SDL_SCANCODE_S, SDL_SCANCODE_D, SDL_SCANCODE_P, SDL_SCANCODE_J, SDL_SCANCODE_K, SDL_SCANCODE_L, SDL_SCANCODE_BACKSPACE, SDL_SCANCODE_SPACE };
const Uint8* inputState;
bool pauseHeld;
std::thread in;
Uint8 eventControls[CONTROL_COUNT];
int mousePosX;
int mousePosY;
bool leftMouseButtonState;
bool leftMouseButtonDownEvent;
int64_t inputEventHandlerUUID;
EventHandler inputHandler;



void setUpInputEventHandler()
{
    inputEventHandlerUUID = getNewUUID();
    addComponent(inputEventHandlerUUID, EVENTHANDLER);
    propertyMap.at(EVENTHANDLER).at(inputEventHandlerUUID) = EventHandler();
    inputHandler = std::get<EventHandler>(propertyMap.at(EVENTHANDLER).at(inputEventHandlerUUID));
    inputHandler.eventHandleMethod = onInputChanged;
    std::list<EventType> list = std::list<EventType>();
    list.push_back(InputEvent);
    eventManager->registerEvent(list, &inputHandler);
    for (int i = 0; i < CONTROL_COUNT; i++) {
        eventControls[i] = 0;
    }
}

//borrowed from moodle sample code , however, it is not currently checking realtime input as this will likely be saved for refactoring or event inputs
void doInput(void)
{
    SDL_Event event;

    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
        case SDL_QUIT:
            quitGame();
            break;
       /* case SDL_MOUSEBUTTONDOWN:
            Event e = Event(InputEvent);
            for (int i = 0; i < CONTROL_COUNT; i++) {
                e.parameters.push_back(inputState[controls[i]]);
            }
            eventManager->raise(e);
            break;*/
        default:
            break;
        }
    }

}

/** Reads input from keyboard and calculates total object movement
*/
void readInput(/*Coord* totalMovement, Object* shape*/)
{

    inputState = SDL_GetKeyboardState(NULL);
    leftMouseButtonState = SDL_GetMouseState(&mousePosX, &mousePosY) & SDL_BUTTON(1);
    if (eventControls[0] != inputState[controls[0]] || eventControls[1] != inputState[controls[1]] || eventControls[2] != inputState[controls[2]]
        || eventControls[3] != inputState[controls[3]] || eventControls[4] != inputState[controls[4]] || eventControls[5] != inputState[controls[5]]
        || eventControls[6] != inputState[controls[6]] || eventControls[7] != inputState[controls[7]] || eventControls[8] != inputState[controls[8]]
        || eventControls[9] != inputState[controls[9]] || leftMouseButtonDownEvent != leftMouseButtonState) {
        Event e = Event(InputEvent);
        for (int i = 0; i < CONTROL_COUNT; i++) {
            e.parameters.push_back(inputState[controls[i]]);
        }
        e.parameters.push_back(leftMouseButtonState);
        eventManager->raise(e);//TODO: continue
    }
}

void onInputChanged(EventType eventType, int64_t sourceUUID, EventParams parameters)
{
    if (!std::holds_alternative<Uint8>(parameters.at(1))) {
        return;
    }
    eventControls[0] = std::get<Uint8>(parameters.at(0));
    eventControls[1] = std::get<Uint8>(parameters.at(1));
    eventControls[2] = std::get<Uint8>(parameters.at(2));
    eventControls[3] = std::get<Uint8>(parameters.at(3));
    eventControls[4] = std::get<Uint8>(parameters.at(4));
    eventControls[5] = std::get<Uint8>(parameters.at(5));
    eventControls[6] = std::get<Uint8>(parameters.at(6));
    eventControls[7] = std::get<Uint8>(parameters.at(7));
    eventControls[8] = std::get<Uint8>(parameters.at(8));
    eventControls[9] = std::get<Uint8>(parameters.at(9));
    leftMouseButtonDownEvent = std::get<bool>(parameters.at(10));
    /*for (int i = 0; i < CONTROL_COUNT; i++) {
        eventControls[i] = std::get<Uint8>(parameters.at(i));
    }*/
}



bool isKeyPressed(Key key)
{
    //inputState = SDL_GetKeyboardState(NULL);
    if (key == W) {
        return eventControls[0] == 1;
    }
    else if (key == A) {
        return eventControls[1] == 1;
    }
    else if (key == S) {
        return eventControls[2] == 1;
    }
    else if (key == D) {
        return eventControls[3] == 1;
    }
    else if (key == P) {
        return eventControls[4] == 1;
    }
    else if (key == J) {
        return eventControls[5] == 1;
    }
    else if (key == K) {
        return eventControls[6] == 1;
    }
    else if (key == L) {
        return eventControls[7] == 1;
    }
    else if (key == BACKSPACE) {
        return eventControls[8] == 1;
    }
    else if (key == SPACE_BAR) {
        return eventControls[9] == 1;
    }
    else if (key == LEFT_CLICK) {
        return leftMouseButtonDownEvent;
    }

    return false;
}

void setPauseHeld(bool held)
{
    pauseHeld = held;
}

Coord getMousePos()
{
    return Coord(mousePosX, mousePosY);
}

bool getPauseHeld()
{
    return pauseHeld;
}
