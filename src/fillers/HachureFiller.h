#ifndef ROUGH_HACHURE_FILLER_H
#define ROUGH_HACHURE_FILLER_H

#include "../Core.h"
#include "../Geometry.h"

namespace Rough {

class Renderer;

namespace Filler {

class HachureFiller
{
public:
	HachureFiller(Renderer &renderer);
	void fillPolygon(const std::vector<Point> &points, const RoughOptions &o, bool connectEnds = false);

private:
	void renderLines(const std::vector<Line> &lines, const RoughOptions &o);
	std::vector<Line> connectingLines(const std::vector<Point> &polygon, const std::vector<Line> &lines);
	bool midPointInPolygon(const std::vector<Point> &polygon, const Line &segment);
	std::vector<Line> splitOnIntersections(const std::vector<Point> &polygon, const Line &segment);

	Renderer &renderer;
};

} /* namespace Filler */
} /* namespace Rough */

#endif /* ROUGH_HACHURE_FILLER_H */
