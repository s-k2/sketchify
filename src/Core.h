#ifndef ROUGH_CORE_H
#define ROUGH_CORE_H

#include <array>

namespace Rough {

typedef std::array<double, 2> Point;
typedef std::array<Point, 2> Line;

struct RoughOptions {
	double maxRandomnessOffset;
	double roughness;
	double bowing;
	double disableMultiStroke;
	double disableMultiStrokeFill;
	double curveTightness;
	double curveFitting;
	double curveStepCount;
	double hachureAngle;
	double hachureGap;
	double strokeWidth;
};

}

#endif /* ROUGH_CORE_H */
