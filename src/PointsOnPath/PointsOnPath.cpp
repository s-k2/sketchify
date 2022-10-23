#include "PointsOnPath.h"

#include <ranges>

#include "../PathDataParser/Absolutize.h"
#include "../PathDataParser/Normalize.h"
#include "../PathDataParser/Parser.h"
#include "../PointsOnCurve/PointsOnCurve.h"

namespace Rough {
namespace PointsOnPath {

std::vector<std::vector<Point>> pointsOnPath(const std::string &path, double tolerance, double distance)
{
	const std::vector<PathDataParser::Segment> segments = PathDataParser::parsePath(path);
	const std::vector<PathDataParser::Segment> normalized = PathDataParser::normalize(PathDataParser::absolutize(segments));

	std::vector<std::vector<Point>> sets;
	std::vector<Point> currentPoints;
	Point start{0, 0};
	std::vector<Point> pendingCurve;

	auto appendPendingCurve = [&]() {
		if(pendingCurve.size() >= 4) {
			auto pendingCurvePoints = PointsOnCurve::pointsOnBezierCurves(pendingCurve, tolerance);
			std::copy(pendingCurvePoints.begin(), pendingCurvePoints.end(), std::back_inserter(currentPoints));
		}
		pendingCurve.clear();
	};

	auto appendPendingPoints = [&]() {
		appendPendingCurve();
		if(!currentPoints.empty()) {
			sets.emplace_back(std::move(currentPoints));
			currentPoints.clear();
		}
	};

	for(const auto &segment : normalized) {
		switch(segment.key) {
			case 'M':
				appendPendingPoints();
				start = Point{segment.data[0], segment.data[1]};
				currentPoints.push_back(start);
				break;
			case 'L':
				appendPendingCurve();
				currentPoints.emplace_back(Point{segment.data[0], segment.data[1]});
				break;
			case 'C':
				if(pendingCurve.empty()) {
					const Point lastPoint = currentPoints.size() ? currentPoints.back() : start;
					pendingCurve.emplace_back(lastPoint);
				}
				pendingCurve.emplace_back(Point{segment.data[0], segment.data[1]});
				pendingCurve.emplace_back(Point{segment.data[2], segment.data[3]});
				pendingCurve.emplace_back(Point{segment.data[4], segment.data[5]});
				break;
			case 'Z':
				appendPendingCurve();
				currentPoints.push_back(start);
				break;
		}
	}
	appendPendingPoints();

	if(!distance) {
		return sets;
	}

	std::vector<std::vector<Point>> out;
	for(const auto &set : sets) {
		const auto simplifiedSet = PointsOnCurve::simplify(set, distance);
		if(!simplifiedSet.empty()) {
			out.emplace_back(simplifiedSet);
		}
	}
	return out;
}

} /* PointsOnPath */
} /* Rough */
 
