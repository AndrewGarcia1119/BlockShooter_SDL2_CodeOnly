#pragma once
#include "serverGlobals.hpp"
#include <zmq.hpp>
#include <zmq_addon.hpp>
#include <SDL.h>
#include <thread>
#include <iostream>
#include "EventManager.hpp"



void onServerStart();
void onServerUpdate();
void onServerEnd();