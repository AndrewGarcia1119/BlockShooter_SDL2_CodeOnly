#pragma once
#include "structs.hpp"
#include "init.hpp"
#include "draw.hpp"
#include "input.hpp"
#include "TimeManager.hpp"
#include "game.hpp"
#include "PhysicsManager.hpp"
#include <memory>

//borrowed from moodle sample code


// SDL render and window context
extern App* app;

int main(int argc, char* argv[]);