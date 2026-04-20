#include "ShapeComponent.hpp"

void ShapeComponent::renderShapes()
{
	Coord pos;
	for (auto const& [UUID, Att] : propertyMap.at(RENDER)) {
		if (propertyMap.at(POSITION).find(UUID) != propertyMap.at(POSITION).end()) {
			ShapeAttribute sa = std::get<ShapeAttribute>(Att);
			pos = std::get<Coord>(propertyMap.at(POSITION).at(UUID));
			sa.shape.x = pos.x;
			sa.shape.y = pos.y;
			SDL_SetRenderDrawColor(app->renderer, sa.r, sa.g, sa.b, SDL_ALPHA_OPAQUE);
			SDL_RenderDrawRectF(app->renderer, &(sa.shape));
			SDL_RenderFillRectF(app->renderer, &(sa.shape));
		}
	}
}

int ShapeComponent::getRColor(int64_t UUID)
{
	return std::get<ShapeAttribute>(propertyMap.at(RENDER).at(UUID)).r;
}

int ShapeComponent::getGColor(int64_t UUID)
{
	return std::get<ShapeAttribute>(propertyMap.at(RENDER).at(UUID)).g;
}

int ShapeComponent::getBColor(int64_t UUID)
{
	return std::get<ShapeAttribute>(propertyMap.at(RENDER).at(UUID)).b;
}
