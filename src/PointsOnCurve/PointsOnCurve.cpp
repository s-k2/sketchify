#include "PointsOnCurve.h"

#include <algorithm>
#include <math.h>

namespace Rough {
namespace PointsOnCurve {

static double distanceSq(Point p1, Point p2);
static Point lerp(Point a, Point b, double t);
static std::vector<Point> simplifyPoints(const std::vector<Point> &points, size_t start, size_t end, double epsilon);
static std::vector<Point> simplifyPoints(const std::vector<Point> &points, size_t start, size_t end, double epsilon, std::vector<Point> &newPoints);

// distance between 2 points
static double distance(Point p1, Point p2)
{
	return sqrt(distanceSq(p1, p2));
}

// distance between 2 points squared
static double distanceSq(Point p1, Point p2)
{
	return pow(p1[0] - p2[0], 2) + pow(p1[1] - p2[1], 2);
}

// Sistance squared from a point p to the line segment vw
static double distanceToSegmentSq(Point p, Point v, Point w)
{
	const double l2 = distanceSq(v, w);
	if(l2 == 0) {
		return distanceSq(p, v);
	}
	double t = ((p[0] - v[0]) * (w[0] - v[0]) + (p[1] - v[1]) * (w[1] - v[1])) / l2;
	t = std::max(0.0, std::min(1.0, t));
	return distanceSq(p, lerp(v, w, t));
}

static Point lerp(Point a, Point b, double t)
{
	return Point{
		a[0] + (b[0] - a[0]) * t,
		a[1] + (b[1] - a[1]) * t,
	};
}

// Adapted from https://seant23.wordpress.com/2010/11/12/offset-bezier-curves/
static double flatness(const std::vector<Point> &points, double offset)
{
	const Point &p1 = points[offset + 0];
	const Point &p2 = points[offset + 1];
	const Point &p3 = points[offset + 2];
	const Point &p4 = points[offset + 3];

	double ux = 3 * p2[0] - 2 * p1[0] - p4[0]; ux *= ux;
	double uy = 3 * p2[1] - 2 * p1[1] - p4[1]; uy *= uy;
	double vx = 3 * p3[0] - 2 * p4[0] - p1[0]; vx *= vx;
	double vy = 3 * p3[1] - 2 * p4[1] - p1[1]; vy *= vy;

	if(ux < vx) {
		ux = vx;
	}

	if(uy < vy) {
		uy = vy;
	}

	return ux + uy;
}

static std::vector<Point> getPointsOnBezierCurveWithSplitting(const std::vector<Point> &points, double offset, double tolerance, std::vector<Point> &newPoints)
{
	std::vector<Point> &outPoints = newPoints;
	if(flatness(points, offset) < tolerance) {
		const Point &p0 = points[offset + 0];
		if(!outPoints.empty()) {
			const double d = distance(outPoints.back(), p0);
			if(d > 1) {
				outPoints.push_back(p0);
			}
		} else {
			outPoints.push_back(p0);
		}
		outPoints.push_back(points[offset + 3]);
	} else {
		// subdivide
		const double t = .5;
		const Point p1 = points[offset + 0];
		const Point p2 = points[offset + 1];
		const Point p3 = points[offset + 2];
		const Point p4 = points[offset + 3];

		const Point q1 = lerp(p1, p2, t);
		const Point q2 = lerp(p2, p3, t);
		const Point q3 = lerp(p3, p4, t);

		const Point r1 = lerp(q1, q2, t);
		const Point r2 = lerp(q2, q3, t);

		const Point red = lerp(r1, r2, t);

		getPointsOnBezierCurveWithSplitting(std::vector<Point>{p1, q1, r1, red}, 0, tolerance, outPoints);
		getPointsOnBezierCurveWithSplitting(std::vector<Point>{red, r2, q3, p4}, 0, tolerance, outPoints);
	}
	return outPoints;
}

std::vector<Point> simplify(const std::vector<Point> &points, double distance)
{
	return simplifyPoints(points, 0, points.size(), distance);
}

static std::vector<Point> simplifyPoints(const std::vector<Point> &points, size_t start, size_t end, double epsilon)
{
	std::vector<Point> emptyNewPoints;
	return simplifyPoints(points, start, end, epsilon, emptyNewPoints);
}

// Ramer–Douglas–Peucker algorithm
// https://en.wikipedia.org/wiki/Ramer%E2%80%93Douglas%E2%80%93Peucker_algorithm
static std::vector<Point> simplifyPoints(const std::vector<Point> &points, size_t start, size_t end, double epsilon, std::vector<Point> &newPoints)
{
	std::vector<Point> &outPoints = newPoints;

	// find the most distance point from the endpoints
	const Point s = points[start];
	const Point e = points[end - 1];
	double maxDistSq = 0;
	size_t maxNdx = 1;
	for(size_t i = start + 1; i < end - 1; ++i) {
		const double distSq = distanceToSegmentSq(points[i], s, e);
		if(distSq > maxDistSq) {
			maxDistSq = distSq;
			maxNdx = i;
		}
	}

	// if that point is too far, split
	if(sqrt(maxDistSq) > epsilon) {
		simplifyPoints(points, start, maxNdx + 1, epsilon, outPoints);
		simplifyPoints(points, maxNdx, end, epsilon, outPoints);
	} else {
		if(outPoints.empty()) {
			outPoints.push_back(s);
		}
		outPoints.push_back(e);
	}

	return outPoints;
}

std::vector<Point> pointsOnBezierCurves(const std::vector<Point> &points, double tolerance, double distance)
{
	std::vector<Point> newPoints;
	const size_t numSegments = (points.size() - 1) / 3;
	for(size_t i = 0; i < numSegments; i++) {
		const size_t offset = i * 3;
		getPointsOnBezierCurveWithSplitting(points, offset, tolerance, newPoints);
	}
	if(distance && distance > 0) {
		return simplifyPoints(newPoints, 0, newPoints.size(), distance);
	}
	return newPoints;
}

} /* PointsOnCurve */
} /* Rough */
 
