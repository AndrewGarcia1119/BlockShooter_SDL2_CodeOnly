#pragma once
#include "structs.hpp"
#include "defs.hpp"
#include <SDL.h>
#include <iostream>

//borrowed from moodle sample code

// SDL render and window context
extern App* app;

// Initialize SDL rendering window
void initSDL(void);