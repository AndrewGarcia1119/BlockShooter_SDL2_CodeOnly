#pragma once
#include "properties.hpp"
#include "draw.hpp"



class ShapeComponent {
public: 
	void renderShapes();
	int getRColor(int64_t UUID);
	int getGColor(int64_t UUID);
	int getBColor(int64_t UUID);
};