#ifndef ROUGH_POINTS_ON_PATH_H
#define ROUGH_POINTS_ON_PATH_H

#include <vector>
#include <string>

#include "../Core.h" // TODO: bad recursive dependency... but where else should this type be declared?

namespace Rough {
namespace PointsOnPath {

extern std::vector<std::vector<Point>> pointsOnPath(const std::string &path, double tolerance, double distance);

} /* PointsOnPath */
} /* Rough */
 
#endif /* ROUGH_POINTS_ON_PATH_H */
