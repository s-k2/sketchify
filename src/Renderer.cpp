#include "Renderer.h"

#include <array>
#include <math.h>

//#include "Geometry.h"
#include "fillers/HachureFiller.h"
#include "PathDataParser/Parser.h"
#include "PathDataParser/Absolutize.h"
#include "PathDataParser/Normalize.h"

#include "PointsOnPath/PointsOnPath.h"

namespace Rough
{

void Renderer::line(double x1, double y1, double x2, double y2, const RoughOptions &o)
{
	doubleLine(x1, y1, x2, y2, o);
}

void Renderer::linearPath(const std::vector<Point> &points, bool close, const RoughOptions &o)
{
	const double len = points.size();
	if(len > 2) {
		for(size_t i = 0; i < (len - 1); i++) {
			doubleLine(points[i][0], points[i][1], points[i + 1][0], points[i + 1][1], o);
		}
		if(close) {
			doubleLine(points[len - 1][0], points[len - 1][1], points[0][0], points[0][1], o);
		}
	} else if(len == 2) {
		line(points[0][0], points[0][1], points[1][0], points[1][1], o);
	}
}

void Renderer::polygon(const std::vector<Point> &points, const RoughOptions &o)
{
	linearPath(points, true, o);
}

void Renderer::rectangle(double x, double y, double width, double height, const RoughOptions &o)
{
	std::vector<Point> points{
		{x, y},
		{x + width, y},
		{x + width, y + height},
		{x, y + height}
	};
	return polygon(points, o);
}

void Renderer::curve(const std::vector<Point> &points, const RoughOptions &o)
{
	curveWithOffset(points, 1 * (1 + o.roughness * 0.2), o);
	if(!o.disableMultiStroke) {
		curveWithOffset(points, 1.5 * (1 + o.roughness * 0.22), cloneOptionsAlterSeed(o));
	}
}

void Renderer::ellipse(double x, double y, double width, double height, const RoughOptions &o)
{
	const auto params = generateEllipseParams(width, height, o);
	ellipseWithParams(x, y, o, params);
}

EllipseParams Renderer::generateEllipseParams(double width, double height, const RoughOptions &o)
{
	const double psq = sqrt(M_PI * 2 * sqrt((pow(width / 2, 2) + pow(height / 2, 2)) / 2));
	const double stepCount = std::max(o.curveStepCount, (o.curveStepCount / sqrt(200)) * psq);
	const double increment = (M_PI * 2) / stepCount;
	double rx = abs(width / 2);
	double ry = abs(height / 2);
	const double curveFitRandomness = 1 - o.curveFitting;
	rx += offsetOpt(rx * curveFitRandomness, o);
	ry += offsetOpt(ry * curveFitRandomness, o);
	return EllipseParams{ increment, rx, ry };
}

std::vector<Point> Renderer::ellipseWithParams(double x, double y, const RoughOptions o, const EllipseParams &ellipseParams)
{
	const auto [ap1, cp1] = computeEllipsePoints(ellipseParams.increment, x, y, ellipseParams.rx, ellipseParams.ry, 1.0, ellipseParams.increment * offset(0.1, offset(0.4, 1.0, o), o), o);
	curve(ap1, std::nullopt, o);
	if(!o.disableMultiStroke) {
		const auto [ap2, cp1] = computeEllipsePoints(ellipseParams.increment, x, y, ellipseParams.rx, ellipseParams.ry, 1.5, 0, o);
		curve(ap2, std::nullopt, o);
	}

	return(cp1);
}

void Renderer::arc(double x, double y, double width, double height, double start, double stop, bool closed, bool roughClosure, const RoughOptions &o)
{
	const double cx = x;
	const double cy = y;
	double rx = abs(width / 2);
	double ry = abs(height / 2);
	rx += offsetOpt(rx * 0.01, o);
	ry += offsetOpt(ry * 0.01, o);
	double strt = start;
	double stp = stop;
	while(strt < 0) {
		strt += M_PI * 2;
		stp += M_PI * 2;
	}
	if((stp - strt) > (M_PI * 2)) {
		strt = 0;
		stp = M_PI * 2;
	}
	const double ellipseInc = (M_PI * 2) / o.curveStepCount;
	const double arcInc = std::min(ellipseInc / 2, (stp - strt) / 2);
	privateArc(arcInc, cx, cy, rx, ry, strt, stp, 1, o);
	if(!o.disableMultiStroke) {
		privateArc(arcInc, cx, cy, rx, ry, strt, stp, 1.5, o);
	}
	if(closed) {
		if(roughClosure) {
			doubleLine(cx, cy, cx + rx * cos(strt), cy + ry * sin(strt), o);
			doubleLine(cx, cy, cx + rx * cos(stp), cy + ry * sin(stp), o);
		} else {
			graphics.lineTo(cx, cy);
			graphics.lineTo(cx + rx * cos(strt), cy + ry * sin(strt));
		}
	}
}

void Renderer::svgPath(const std::string &path, const RoughOptions &o)
{
	std::vector<PathDataParser::Segment> segments = PathDataParser::normalize(PathDataParser::absolutize(PathDataParser::parsePath(path)));
	Point first{0, 0};
	Point current{0, 0};
	for(const auto &segment : segments) {
		const auto &data = segment.data;
		switch (segment.key) {
			case 'M': {
					  const double ro = 1 * o.maxRandomnessOffset;
					  graphics.moveTo(data[0] + offsetOpt(ro, o), data[1] + offsetOpt(ro, o));
					  current = Point{data[0], data[1]};
					  first = Point{data[0], data[1]};
					  break;
				  }
			case 'L':
				  doubleLine(current[0], current[1], data[0], data[1], o);
				  current = Point{data[0], data[1]};
				  break;
			case 'C': {
					  const double x1 = data[0], y1 = data[1], x2 = data[2], y2 = data[3], x = data[4], y = data[5];
					  bezierTo(x1, y1, x2, y2, x, y, current, o);
					  current = Point{x, y};
					  break;
				  }
			case 'Z':
				  doubleLine(current[0], current[1], first[0], first[1], o);
				  current = Point{first[0], first[1]};
				  break;
		}
	}
}


/*
 * svgPath()
 */
void Renderer::solidFillPolygon(std::vector<Point> points, const RoughOptions &o)
{
	if(points.size()) {
		const double offset = o.maxRandomnessOffset;
		const size_t len = points.size();
		if(len > 2) {
			graphics.moveTo(points[0][0] + offsetOpt(offset, o), points[0][1] + offsetOpt(offset, o));
			for(size_t i = 1; i < len; i++) {
				graphics.lineTo(points[i][0] + offsetOpt(offset, o), points[i][1] + offsetOpt(offset, o));
			}
		}
	}
	// TODO: fill?
}

/*
 * patternFillPolygon()
 */
void Renderer::patternFillPolygon(std::vector<Point> points, const RoughOptions &o)
{
	// TODO: call filler algorithm
	Filler::HachureFiller filler(*this);
	filler.fillPolygon(points, o);
}

void Renderer::patternFillArc(double x, double y, double width, double height, double start, double stop, const RoughOptions &o)
{
	const double cx = x;
	const double cy = y;
	double rx = abs(width / 2);
	double ry = abs(height / 2);
	rx += offsetOpt(rx * 0.01, o);
	ry += offsetOpt(ry * 0.01, o);
	double strt = start;
	double stp = stop;
	while(strt < 0) {
		strt += M_PI * 2;
		stp += M_PI * 2;
	}
	if((stp - strt) > (M_PI * 2)) {
		strt = 0;
		stp = M_PI * 2;
	}
	const double increment = (stp - strt) / o.curveStepCount;
	std::vector<Point> points;
	for(double angle = strt; angle <= stp; angle = angle + increment) {
		points.emplace_back(Point{cx + rx * cos(angle), cy + ry * sin(angle)});
	}
	points.emplace_back(Point{cx + rx * cos(stp), cy + ry * sin(stp)});
	points.emplace_back(Point{cx, cy});

	patternFillPolygon(points, o);
}

/*
 * randOffset()
 * randOffsetWithRange()
 * doubleLineFillOps()
 */
void Renderer::doubleLineFillOps(double x1, double y1, double x2, double y2, const RoughOptions &o)
{
	doubleLine(x1, y1, x2, y2, o, true);
}
RoughOptions Renderer::cloneOptionsAlterSeed(const RoughOptions &options)
{
	RoughOptions result{ options };
	/* 
	   result.randomizer = undefined;
	   if(ops.seed) {
		   result.seed = ops.seed + 1;
	   }
	 */
	return result;
}

double Renderer::randomWithOptions(const RoughOptions &options)
{
	return((rand() % (10000)) / 10000.0);
}

double Renderer::offset(double min, double max, const RoughOptions &o, double roughnessGain)
{
	return o.roughness * roughnessGain * ((randomWithOptions(o) * (max - min)) + min);
}

double Renderer::offsetOpt(double x, const RoughOptions &o, double roughnessGain)
{
	return offset(-x, x, o, roughnessGain);
}

void Renderer::doubleLine(double x1, double y1, double x2, double y2, const RoughOptions &o, bool filling)
{
	const bool singleStroke = filling ? o.disableMultiStrokeFill : o.disableMultiStroke;
	privateLine(x1, y1, x2, y2, o, true, false);
	if(!singleStroke) {
                privateLine(x1, y1, x2, y2, o, true, true);
	}
}

void Renderer::privateLine(int x1, int y1, int x2, int y2, const RoughOptions &o, bool move, bool overlay)
{
	const double lengthSq = pow((x1 - x2), 2) + pow((y1 - y2), 2);
	const double length = sqrt(lengthSq);

	double roughnessGain = 1;
	if(length < 200) {
		roughnessGain = 1;
	} else if(length > 500) {
		roughnessGain = 0.4;
	} else {
		roughnessGain = (-0.0016668) * length + 1.233334;
	}

	double offset = o.maxRandomnessOffset;
	if((offset * offset * 100) > lengthSq) {
		offset = length / 10;
	}

	const double halfOffset = offset / 2;
	const double divergePoint = 0.2 + randomWithOptions(o) * 0.2;
	double midDispX = o.bowing * o.maxRandomnessOffset * (y2 - y1) / 200;
	double midDispY = o.bowing * o.maxRandomnessOffset * (x1 - x2) / 200;
	midDispX = offsetOpt(midDispX, o, roughnessGain);
	midDispY = offsetOpt(midDispY, o, roughnessGain);

	if(move) {
		if(overlay) {
			graphics.moveTo(x1 + offsetOpt(halfOffset, o, roughnessGain), y1 + offsetOpt(halfOffset, o, roughnessGain));
		} else {
			graphics.moveTo(x1 + offsetOpt(offset, o, roughnessGain), y1 + offsetOpt(offset, o, roughnessGain));
		}
	}

	if(overlay) {
		graphics.bezierCurveTo(midDispX + x1 + (x2 - x1) * divergePoint + offsetOpt(halfOffset, o, roughnessGain),
				midDispY + y1 + (y2 - y1) * divergePoint + offsetOpt(halfOffset, o, roughnessGain),
				midDispX + x1 + 2 * (x2 - x1) * divergePoint + offsetOpt(halfOffset, o, roughnessGain),
				midDispY + y1 + 2 * (y2 - y1) * divergePoint + offsetOpt(halfOffset, o, roughnessGain),
				x2 + offsetOpt(halfOffset, o, roughnessGain),
				y2 + offsetOpt(halfOffset, o, roughnessGain));

	} else {
		graphics.bezierCurveTo(midDispX + x1 + (x2 - x1) * divergePoint + offsetOpt(offset, o, roughnessGain),
				midDispY + y1 + (y2 - y1) * divergePoint + offsetOpt(offset, o, roughnessGain),
				midDispX + x1 + 2 * (x2 - x1) * divergePoint + offsetOpt(offset, o, roughnessGain),
				midDispY + y1 + 2 * (y2 - y1) * divergePoint + offsetOpt(offset, o, roughnessGain),
				x2 + offsetOpt(offset, o, roughnessGain),
				y2 + offsetOpt(offset, o, roughnessGain));

	}
}

void Renderer::curveWithOffset(const std::vector<Point> &points, double offset, const RoughOptions &o)
{
	std::vector<Point> ps;
	ps.emplace_back(Point{points[0][0] + offsetOpt(offset, o),
			points[0][1] + offsetOpt(offset, o)});
	ps.emplace_back(Point{points[0][0] + offsetOpt(offset, o),
			points[0][1] + offsetOpt(offset, o)});
	for(size_t i = 1; i < points.size(); i++) {
		ps.emplace_back(Point{points[i][0] + offsetOpt(offset, o),
				points[i][1] + offsetOpt(offset, o)});
		if(i == (points.size() - 1)) {
			ps.emplace_back(Point{points[i][0] + offsetOpt(offset, o),
					points[i][1] + offsetOpt(offset, o)});
		}
	}
	curve(ps, std::nullopt, o);
}

void Renderer::curve(const std::vector<Point> &points, std::optional<Point> closePoint, const RoughOptions &o)
{
	const size_t len = points.size();
	if(len > 3) {
		std::array<Point, 4> b;
		const double s = 1 - o.curveTightness;
		graphics.moveTo(points[1][0], points[1][1]);
		for(size_t i = 1; (i + 2) < len; i++) {
			const Point cachedVertArray = points[i];
			b[0] = Point{cachedVertArray[0], cachedVertArray[1]};
			b[1] = Point{cachedVertArray[0] + (s * points[i + 1][0] - s * points[i - 1][0]) / 6, cachedVertArray[1] + (s * points[i + 1][1] - s * points[i - 1][1]) / 6};
			b[2] = Point{points[i + 1][0] + (s * points[i][0] - s * points[i + 2][0]) / 6, points[i + 1][1] + (s * points[i][1] - s * points[i + 2][1]) / 6};
			b[3] = Point{points[i + 1][0], points[i + 1][1]};
			graphics.bezierCurveTo(b[1][0], b[1][1], b[2][0], b[2][1], b[3][0], b[3][1]);
		}
		if(closePoint.has_value()) {
			const double ro = o.maxRandomnessOffset;
			graphics.lineTo(closePoint.value()[0] + offsetOpt(ro, o), closePoint.value()[1] + offsetOpt(ro, o));
		}
	} else if(len == 3) {
		graphics.moveTo(points[1][0], points[1][1]);
		graphics.bezierCurveTo(points[1][0], points[1][1],
				points[2][0], points[2][1],
				points[2][0], points[2][1]);
	} else if(len == 2) {
		doubleLine(points[0][0], points[0][1], points[1][0], points[1][1], o);
	}
}

std::tuple<std::vector<Point>, std::vector<Point>> Renderer::computeEllipsePoints(double increment, double cx, double cy, double rx, double ry, double offset, double overlap, const RoughOptions &o)
{
	std::vector<Point> corePoints;
	std::vector<Point> allPoints;
	const double radOffset = offsetOpt(0.5, o) - (M_PI / 2);

	allPoints.emplace_back(Point{offsetOpt(offset, o) + cx + 0.9 * rx * cos(radOffset - increment),
			offsetOpt(offset, o) + cy + 0.9 * ry * sin(radOffset - increment)});

	for(double angle = radOffset; angle < (M_PI * 2 + radOffset - 0.01); angle = angle + increment) {
		Point p{offsetOpt(offset, o) + cx + rx * cos(angle),
				offsetOpt(offset, o) + cy + ry * sin(angle)};
		corePoints.push_back(p);
		allPoints.push_back(p);
	}

	allPoints.emplace_back(Point{offsetOpt(offset, o) + cx + rx * cos(radOffset + M_PI * 2 + overlap * 0.5),
			offsetOpt(offset, o) + cy + ry * sin(radOffset + M_PI * 2 + overlap * 0.5)});

	allPoints.emplace_back(Point{offsetOpt(offset, o) + cx + 0.98 * rx * cos(radOffset + overlap),
			offsetOpt(offset, o) + cy + 0.98 * ry * sin(radOffset + overlap)});

	allPoints.emplace_back(Point{offsetOpt(offset, o) + cx + 0.9 * rx * cos(radOffset + overlap * 0.5),
			offsetOpt(offset, o) + cy + 0.9 * ry * sin(radOffset + overlap * 0.5)});

	return { allPoints, corePoints };
}

void Renderer::privateArc(double increment, double cx, double cy, double rx, double ry, double strt, double stp, double offset, const RoughOptions &o)
{
	const double radOffset = strt + offsetOpt(0.1, o);
	std::vector<Point> points;
	points.emplace_back(Point{offsetOpt(offset, o) + cx + 0.9 * rx * cos(radOffset - increment),
			offsetOpt(offset, o) + cy + 0.9 * ry * sin(radOffset - increment)});
	for(double angle = radOffset; angle <= stp; angle = angle + increment) {
		points.emplace_back(Point{offsetOpt(offset, o) + cx + rx * cos(angle),
				offsetOpt(offset, o) + cy + ry * sin(angle)});
	}
	points.emplace_back(Point{cx + rx * cos(stp),
			cy + ry * sin(stp)});
	points.emplace_back(Point{cx + rx * cos(stp),
			cy + ry * sin(stp)});

	curve(points, std::nullopt, o);
}

void Renderer::bezierTo(double x1, double y1, double x2, double y2, double x, double y, Point current, const RoughOptions &o)
{
	const double ros[2] = { o.maxRandomnessOffset, o.maxRandomnessOffset + 0.3 };
	double f[2] = { 0.0f, 0.0f };
	const size_t iterations = o.disableMultiStroke ? 1 : 2;
	for(size_t i = 0; i < iterations; i++) {
		if(i == 0) {
			graphics.moveTo(current[0], current[1]);
		} else {
			graphics.moveTo(current[0] + offsetOpt(ros[0], o), current[1] + offsetOpt(ros[0], o));
		}
		f[0] = x + offsetOpt(ros[i], o, 1);
		f[1] = y + offsetOpt(ros[i], o, 1);

		graphics.bezierCurveTo(x1 + offsetOpt(ros[i], o), y1 + offsetOpt(ros[i], o),
				x2 + offsetOpt(ros[i], o), y2 + offsetOpt(ros[i], o),
				f[0], f[1]);
	}
}

void Renderer::fillPath(const std::string &path, const RoughOptions &o)
{
	// -> path
	auto sets = PointsOnPath::pointsOnPath(path, 1, (1 + o.roughness) / 2);


	std::vector<Point> combined;
	for(const auto &set : sets)
		for(const auto &point : set)
			combined.push_back(point);

	patternFillPolygon(combined, o);
}

} /* namespace Rough */
