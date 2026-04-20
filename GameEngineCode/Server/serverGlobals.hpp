#pragma once
#include "TimeManager.hpp"
#include "structs.hpp"
#include "EventManager.hpp"
#include <mutex>



extern TimeManager* timeManager;
extern TimeManager* serverTimeManager;
extern std::mutex m;

void initGlobal();

