#include "ScanLineHachure.h"

#include <algorithm>
#include <iterator>
#include <ranges>
#include <vector>
#include <math.h>

#include "../Geometry.h"

namespace Rough {
namespace ScanLineHachure {

struct EdgeEntry
{
	double ymin;
	double ymax;
	double x;
	double islope;
};

struct ActiveEdgeEntry {
	double s;
	EdgeEntry edge;
};

std::vector<Line> straightHachureLines(std::vector<Point> &points, const RoughOptions &o);

std::vector<Line> polygonHachureLines(std::vector<Point> points, const RoughOptions &o)
{
	const Point rotationCenter{ 0, 0 };
	const double angle = round(o.hachureAngle + 90);
	if(angle) {
		Geometry::rotatePoints(points, rotationCenter, angle);
	}
	std::vector<Line> lines = straightHachureLines(points, o);
	if(angle) {
		// TODO: rotatePoints(points, rotationCenter, -angle); Not needed, as we're doing a copy
		Geometry::rotateLines(lines, rotationCenter, -angle);
	}
	return lines;
}

std::vector<Line> straightHachureLines(std::vector<Point> &points, const RoughOptions &o)
{
	std::vector<Point> &vertices = points;
	if(vertices.front() != vertices.back()) {
		vertices.emplace_back(vertices.front());
	}
	std::vector<Line> lines;
	if(vertices.size() > 2) {
		double gap = o.hachureGap;
		if(gap < 0) {
			gap = o.strokeWidth * 4;
		}
		gap = std::max(gap, 0.1);

		// Create sorted edges table
		std::vector<EdgeEntry> edges;
		for(double i = 0; i < vertices.size() - 1; i++) {
			const Point p1 = vertices[i];
			const Point p2 = vertices[i + 1];
			if(p1[1] != p2[1]) {
				const double ymin = std::min(p1[1], p2[1]);
				edges.push_back({
						.ymin = ymin,
						.ymax = std::max(p1[1], p2[1]),
						.x = ymin == p1[1] ? p1[0] : p2[0],
						.islope = (p2[0] - p1[0]) / (p2[1] - p1[1])
					});
			}
		}

		std::ranges::sort(edges, [](const EdgeEntry &e1, const EdgeEntry &e2) {
				if(e1.ymin < e2.ymin) {
					return true;
				}
				if(e1.ymin > e2.ymin) {
					return false;
				}
				if(e1.x < e2.x) {
					return true;
				}
				if(e1.x > e2.x) {
					return false;
				}
				if(e1.ymax == e2.ymax) {
					return true;
				}
				return ((e1.ymax - e2.ymax) / abs((e1.ymax - e2.ymax))) <= 0.0;
			});
		if(!edges.size()) {
			return lines;
		}

		// Start scanning
		std::vector<ActiveEdgeEntry> activeEdges;
		double y = edges[0].ymin;
		while(!activeEdges.empty() || !edges.empty()) {
			if(edges.size()) {
				size_t ix = -1;
				for(size_t i = 0; i < edges.size(); i++) {
					if(edges[i].ymin > y) {
						break;
					}
					ix = i;
				}
				std::ranges::transform(edges | std::ranges::views::take(ix + 1), std::back_inserter(activeEdges), [y](const EdgeEntry &edge) { return ActiveEdgeEntry{.s = y, .edge = edge}; });
				edges.erase(edges.begin(), edges.begin() + (ix + 1));
			}
			std::erase_if(activeEdges, [y](const ActiveEdgeEntry &ae) {
					if(ae.edge.ymax <= y) {
						return true;
					}
					return false;
				});
			
			std::ranges::sort(activeEdges, [](const ActiveEdgeEntry &ae1, const ActiveEdgeEntry &ae2) {
					if(ae1.edge.x == ae2.edge.x) {
						return false;
					}
					return ((ae1.edge.x - ae2.edge.x) / abs((ae1.edge.x - ae2.edge.x))) <= 0.0;
				});

			// fill between the edges
			if(activeEdges.size() > 1) {
				for(size_t i = 0; i < activeEdges.size(); i = i + 2) {
					const size_t nexti = i + 1;
					if(nexti >= activeEdges.size()) {
						break;
					}
					const EdgeEntry &ce = activeEdges[i].edge;
					const EdgeEntry &ne = activeEdges[nexti].edge;
					lines.emplace_back(Line{
							Point{round(ce.x), y},
							Point{round(ne.x), y}
							});
				}
			}

			y += gap;
			for(auto &ae : activeEdges) {
				ae.edge.x = ae.edge.x + (gap * ae.edge.islope);
			};
		}
	}
	return lines;
}

} /* namespace ScanLineHachure */
} /* namespace Rough */

