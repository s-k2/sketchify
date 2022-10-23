#ifndef ROUGH_POINTS_ON_CURVE_H
#define ROUGH_POINTS_ON_CURVE_H

#include <vector>
#include <string>

#include "../Core.h" // TODO: bad recursive dependency... but where else should this type be declared?

namespace Rough {
namespace PointsOnCurve {

extern std::vector<Point> pointsOnBezierCurves(const std::vector<Point> &points, double tolerance, double distance = 0.0);
extern std::vector<Point> simplify(const std::vector<Point> &points, double distance);

} /* PointsOnCurve */
} /* Rough */
 
#endif /* ROUGH_POINTS_ON_CURVE_H */
