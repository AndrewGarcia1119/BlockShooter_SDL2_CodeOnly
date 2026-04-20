#include "properties.hpp"
//#include <map>


std::map<Property, std::map<int64_t, Attributes>> propertyMap;

void initProperties() {
    propertyMap.insert({ POSITION, std::map<int64_t, Attributes>() });
    propertyMap.insert({ PHYSICS, std::map<int64_t, Attributes>() });
    propertyMap.insert({ RENDER, std::map<int64_t, Attributes>() });
    propertyMap.insert({ CONTROL, std::map<int64_t, Attributes>() });
    propertyMap.insert({ COLLISION, std::map<int64_t, Attributes>() });
    propertyMap.insert({ DEATH, std::map<int64_t, Attributes>() });
    propertyMap.insert({ SIDE, std::map<int64_t, Attributes>() });
    propertyMap.insert({ EVENTHANDLER, std::map<int64_t, Attributes>() });
    propertyMap.insert({ HEALTH, std::map<int64_t, Attributes>() });
}

bool addComponent(int64_t UUID, Property property)
{
    if (propertyMap.at(property).find(UUID) == propertyMap.at(property).end()) {
        //Add UUID to property map
        propertyMap.at(property).insert({ UUID, Attributes() });
        //...
        return true;
    }
    return false;
}

void destroy(int64_t *UUID)
{
    for (auto const& [prop, propMap] : propertyMap) {
        if (propertyMap.at(prop).find(*UUID) != propertyMap.at(prop).end()) {
            propertyMap.at(prop).erase(*UUID);
        }
    }
    
        *UUID = 0;
}
