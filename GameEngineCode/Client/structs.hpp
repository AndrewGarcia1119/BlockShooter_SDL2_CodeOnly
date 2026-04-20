#pragma once
#include <SDL.h>
#include <string>


//A struct to contain the pointers to the SDL Renderer and SDL window.
typedef struct {
	SDL_Renderer* renderer;
	SDL_Window* window;
} App;

//A struct that contains 2 values that can be used as a position, velocity or other forms of movement
typedef struct Coordinate{
	float x;
	float y;

	Coordinate(float _x, float _y) : x(_x), y(_y){}
	Coordinate() : x(0.f), y(0.f) {}

} Coord;

typedef struct SI {
	Coord pos;
	float w, h;
	int r, g, b;
	bool deathZone;

	SI(Coord _pos, float _w, float _h, int _r, int _g, int _b) : pos(_pos), w(_w), h(_h), r(_r), g(_g), b(_b), deathZone(false) {}
	SI(Coord _pos, float _w, float _h, bool dz) : pos(_pos), w(_w), h(_h), r(0), g(0), b(0), deathZone(dz) {}
} ShapeInfo;