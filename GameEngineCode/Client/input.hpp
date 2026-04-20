#pragma once 
#include <SDL.h>
#include "defs.hpp"
#include "globalMethodsAndVars.hpp"
#include <thread>


//borrowed from moodle sample code

extern std::thread in;

enum Key {
	W, A, S, D, P, J, K, L, BACKSPACE, SPACE_BAR, LEFT_CLICK
};

void setUpInputEventHandler();

void doInput(void);

void readInput(/*Coord* totalMovement, Object* shape*/);

void onInputChanged(EventType eventType, int64_t sourceUUID, EventParams parameters);

bool isKeyPressed(Key key);

/// <summary>
/// Gets the mouse position of where the mouse was last clicked
/// </summary>
/// <returns> the mouse position </returns>
Coord getMousePos();

bool getPauseHeld();

void setPauseHeld(bool held);