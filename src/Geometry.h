
#ifndef ROUGH_GEOMETRY_H
#define ROUGH_GEOMETRY_H

#include <array>
#include <vector>
#include <math.h>
#include <ranges>
#include <utility>
#include <optional>
#include <limits>

#include "Core.h"

namespace Rough {
namespace Geometry {

struct Rectangle
{
	double x;
	double y;
	double width;
	double height;
};

inline bool doIntersect(Point p1, Point q1, Point p2, Point q2);
inline int orientation(Point p, Point q, Point r);
inline bool onSegment(Point p, Point q, Point r);

template <class RangeT>
inline void rotatePoints(RangeT &&points, Point center, double degrees)
{
	if(points.begin() != points.end()) {
		const double cx = center[0], cy = center[1];
		const double angle = (M_PI / 180) * degrees;
		const double cos = ::cos(angle);
		const double sin = ::sin(angle);
		for(auto &p : points) {
			double x = p[0], y = p[1];
			p[0] = ((x - cx) * cos) - ((y - cy) * sin) + cx;
			p[1] = ((x - cx) * sin) + ((y - cy) * cos) + cy;
		}
	}
}

inline void rotateLines(std::vector<Line> &lines, Point center, double degrees)
{
	std::vector<Point> points;
	rotatePoints(std::ranges::join_view(lines), center, degrees);
}

inline double lineLength(const Line &line)
{
	const Point p1 = line[0];
	const Point p2 = line[1];
	return sqrt(pow(p1[0] - p2[0], 2) + pow(p1[1] - p2[1], 2));
}

inline std::optional<Point> lineIntersection(Point a, Point b, Point c, Point d)
{
	const double a1 = b[1] - a[1];
	const double b1 = a[0] - b[0];
	const double c1 = a1 * (a[0]) + b1 * (a[1]);
	const double a2 = d[1] - c[1];
	const double b2 = c[0] - d[0];
	const double c2 = a2 * (c[0]) + b2 * (c[1]);
	const double determinant = a1 * b2 - a2 * b1;
	return determinant != 0.0 ? std::optional<Point>{ Point{(b2 * c1 - b1 * c2) / determinant, (a1 * c2 - a2 * c1) / determinant} } : std::nullopt; // TODO: compare against double?
}

inline bool isPointInPolygon(const std::vector<Point> points, double x, double y)
{
	const size_t vertices = points.size();

	// There must be at least 3 vertices in polygon
	if(vertices < 3) {
		return false;
	}
	const Point extreme{std::numeric_limits<double>::max(), y}; // TODO: is max double ok? was MAX_SAFE_INT in JS...
	const Point p{x, y};
	size_t count = 0;
	for(size_t i = 0; i < vertices; i++) {
		const Point current = points[i];
		const Point next = points[(i + 1) % vertices];
		if(doIntersect(current, next, p, extreme)) {
			if(orientation(current, p, next) == 0) {
				return onSegment(current, p, next);
			}
			count++;
		}
	}
	// true if count is off
	return count % 2 == 1;
}

// Check if q lies on the line segment pr
inline bool onSegment(Point p, Point q, Point r)
{
	return(
			q[0] <= std::max(p[0], r[0]) &&
			q[0] >= std::min(p[0], r[0]) &&
			q[1] <= std::max(p[1], r[1]) &&
			q[1] >= std::min(p[1], r[1])
	      );
}

// For the ordered points p, q, r, return
// 0 if p, q, r are collinear
// 1 if Clockwise
// 2 if counterclickwise
inline int orientation(Point p, Point q, Point r)
{
	const double val = (q[1] - p[1]) * (r[0] - q[0]) - (q[0] - p[0]) * (r[1] - q[1]);
	if(val == 0) {
		return 0;
	}
	return val > 0 ? 1 : 2;
}

// Check is p1q1 intersects with p2q2
inline bool doIntersect(Point p1, Point q1, Point p2, Point q2)
{
	const int o1 = orientation(p1, q1, p2);
	const int o2 = orientation(p1, q1, q2);
	const int o3 = orientation(p2, q2, p1);
	const int o4 = orientation(p2, q2, q1);

	if(o1 != o2 && o3 != o4) {
		return true;
	}

	// p1, q1 and p2 are colinear and p2 lies on segment p1q1
	if(o1 == 0 && onSegment(p1, p2, q1)) {
		return true;
	}

	// p1, q1 and p2 are colinear and q2 lies on segment p1q1
	if(o2 == 0 && onSegment(p1, q2, q1)) {
		return true;
	}

	// p2, q2 and p1 are colinear and p1 lies on segment p2q2
	if(o3 == 0 && onSegment(p2, p1, q2)) {
		return true;
	}

	// p2, q2 and q1 are colinear and q1 lies on segment p2q2
	if(o4 == 0 && onSegment(p2, q1, q2)) {
		return true;
	}

	return false;
}

} /* namespace Geometry */
} /* namespace Rough */

#endif /* ROUGH_GEOMETRY_H */
