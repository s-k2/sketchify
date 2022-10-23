#include "HachureFiller.h"

#include <algorithm>
#include <optional>

#include "ScanLineHachure.h"
#include "../Core.h"
#include "../Geometry.h"
#include "../Renderer.h"

namespace Rough {
namespace Filler {

struct IntersectionInfo
{
	Point point;
	double distance;
};

HachureFiller::HachureFiller(Renderer &renderer)
	: renderer(renderer)
{
}
void HachureFiller::fillPolygon(const std::vector<Point> &points, const RoughOptions &o, bool connectEnds)
{
	std::vector<Line> lines = ScanLineHachure::polygonHachureLines(points, o);
	if(connectEnds) {
		const std::vector<Line> connectingLines = this->connectingLines(points, lines);
		lines.insert(lines.end(), connectingLines.begin(), connectingLines.end());
	}
	renderLines(lines, o);
}

void HachureFiller::renderLines(const std::vector<Line> &lines, const RoughOptions &o)
{
	for(const auto &line : lines) {
		renderer.doubleLineFillOps(line[0][0], line[0][1], line[1][0], line[1][1], o);
	}
}

std::vector<Line> HachureFiller::connectingLines(const std::vector<Point> &polygon, const std::vector<Line> &lines)
{
	std::vector<Line> result;
	if(lines.size() > 1) {
		for(size_t i = 1; i < lines.size(); i++) {
			const Line &prev = lines[i - 1];
			if(Geometry::lineLength(prev) < 3) {
				continue;
			}
			const Line &current = lines[i];
			Line segment{current[0], prev[1]};
			if(Geometry::lineLength(segment) > 3) {
				const std::vector<Line> segSplits = splitOnIntersections(polygon, segment);
				result.insert(result.end(), segSplits.begin(), segSplits.end());
			}
		}
	}
	return result;
  }

bool HachureFiller::midPointInPolygon(const std::vector<Point> &polygon, const Line &segment)
{
	return Geometry::isPointInPolygon(polygon, (segment[0][0] + segment[1][0]) / 2, (segment[0][1] + segment[1][1]) / 2);
}

std::vector<Line> HachureFiller::splitOnIntersections(const std::vector<Point> &polygon, const Line &segment)
{
	const double error = std::max(5.0, Geometry::lineLength(segment) * 0.1);
	std::vector<IntersectionInfo> intersections;
	for(size_t i = 0; i < polygon.size(); i++) {
		const Point p1 = polygon[i];
		const Point p2 = polygon[(i + 1) % polygon.size()];
		if(Geometry::doIntersect(p1, p2, segment[0], segment[1])) {
			const std::optional<Point> ip = Geometry::lineIntersection(p1, p2, segment[0], segment[1]);
			if(ip.has_value()) {
				const double d0 = Geometry::lineLength(Line{ip.value(), segment[0]});
				const double d1 = Geometry::lineLength(Line{ip.value(), segment[1]});
				if(d0 > error && d1 > error) {
					intersections.emplace_back(IntersectionInfo{
							.point = ip.value(),
							.distance = d0
						});
				}
			}
		}
	}
	if(intersections.size() > 1) {
		std::ranges::sort(intersections, {}, &IntersectionInfo::distance);
		std::vector<Point> ips;
		std::ranges::transform(intersections, std::back_inserter(ips), &IntersectionInfo::point);

		if(!Geometry::isPointInPolygon(polygon, segment[0][0], segment[0][1])) {
			ips.erase(ips.begin());
		}
		if(!Geometry::isPointInPolygon(polygon, segment[1][0], segment[1][1])) {
			ips.pop_back();
		}
		if(ips.size() <= 1) {
			if(midPointInPolygon(polygon, segment)) {
				return std::vector<Line>{ segment };
			} else {
				return std::vector<Line>{};
			}
		}
		std::vector<Point> &spoints = ips;// = [segment[0], ...ips, segment[1]];
		spoints.emplace(spoints.begin(), Point{segment[0]});
		spoints.emplace_back(Point{segment[1]});
		std::vector<Line> slines;
		for(size_t i = 0; i < (spoints.size() - 1); i += 2) {
			const Line subSegment{ spoints[i], spoints[i + 1] };
			if(midPointInPolygon(polygon, subSegment)) {
				slines.emplace_back(std::move(subSegment));
			}
		}
		return slines;
	} else if(midPointInPolygon(polygon, segment)) {
		return std::vector<Line>{ segment };
	} else {
		return std::vector<Line>{};
	}
}

} /* namespace Filler */
} /* namespace Rough */
