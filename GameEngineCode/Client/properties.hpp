#pragma once
#include <map>
#include <variant>
#include <vector>
#include "structs.hpp"
#include <mutex>

#define CONTROL_COUNT 10

extern int64_t uuidVal;

static int64_t getNewUUID() {
    return uuidVal++;
}


//this struct should be moved to structs, but for easy access and testing, it will be here
typedef struct PA{
    Coord positionPrev;
    Coord acceleration;
    float jumpHeight;

    PA(Coord pp, Coord a, float jh) : positionPrev(pp), acceleration(a), jumpHeight(jh) {};
    PA(Coord pp, Coord a) : positionPrev(pp), acceleration(a), jumpHeight(10.f) {};
    PA() : positionPrev(Coord()), acceleration(Coord()), jumpHeight(10.f) {};

} PhysicsAttribute;


typedef struct CL{
    SDL_FRect rectCollider;

} ColliderAttribute;

typedef struct SA {
    SDL_FRect shape;
    int r, g, b;

    SA(float _w, float _h, int _r, int _g, int _b) : shape(SDL_FRect()), r(_r), g(_g), b(_b) {
        shape.w = _w;
        shape.h = _h;
    }
} ShapeAttribute;

typedef struct CONT {
    bool controllable;
} ControlAttribute;

typedef struct DEATH {
    bool deathZone;
    DEATH(bool dz) : deathZone(dz) {}
} DeathAttribute;

typedef struct SIDE {
    bool sideBoundary;
    SIDE(bool sb) : sideBoundary(sb) {}
} SideAttribute;


//////////////////// I moved event type outside EVENT struct so that it was easier to use in other files (specifically event.hpp file)
enum EventType {
    InputEvent, CollisionEvent, DeathEvent, SpawnEvent
};
////////////////////

typedef std::vector<std::variant<int64_t, Coord*, SDL_FRect*, SDL_FRect, Uint8, bool>> EventParams;

typedef struct EVENT {
    int64_t assignedUUID; //this is the UUID of the object the handler is attached to. This is to make handling collisions a bit easier
    void (*eventHandleMethod)(EventType eventType, int64_t sourceUUID, EventParams params);
} EventHandler;

typedef struct HEALTH {
    int hitPoints;
    HEALTH(int hp) : hitPoints(hp){}
    HEALTH() : hitPoints(3) {}
} HealthAttribute;

typedef std::variant<Coord, PhysicsAttribute, ShapeAttribute, ControlAttribute, ColliderAttribute, DeathAttribute, SideAttribute, EventHandler, HealthAttribute> Attributes;

enum Property {
    POSITION,
    PHYSICS,
    RENDER,
    CONTROL,
    COLLISION,
    DEATH,
    SIDE,
    EVENTHANDLER,
    HEALTH
};

extern std::map<Property, std::map<int64_t, Attributes>> propertyMap;

void initProperties();
bool addComponent(int64_t UUID, Property property);
void destroy(int64_t *UUID);

