#ifndef ROUGH_RENDERER_H
#define ROUGH_RENDERER_H

#include <vector>
#include <optional>
#include <string>

#include "Core.h"

namespace Rough {

class Graphics
{
public:
	virtual void moveTo(double x, double y) = 0;
	virtual void lineTo(double x, double y) = 0;
	virtual void bezierCurveTo(double x1, double y1, double x2, double y2, double x3, double y3) = 0;
};

struct EllipseParams
{
	double increment;
	double rx;
	double ry;
};

class Renderer
{
public:
	Renderer(Graphics &graphics)
		: graphics(graphics)
	{
	}

	void line(double x1, double y1, double x2, double y2, const RoughOptions &o);
	void linearPath(const std::vector<Point> &points, bool close, const RoughOptions &o);
	void polygon(const std::vector<Point> &points, const RoughOptions &o);
	void rectangle(double x, double y, double width, double height, const RoughOptions &o);
	void curve(const std::vector<Point> &points, const RoughOptions &o);
	void ellipse(double x, double y, double width, double height, const RoughOptions &o);
	EllipseParams generateEllipseParams(double width, double height, const RoughOptions &o);
	std::vector<Point> ellipseWithParams(double x, double y, const RoughOptions o, const EllipseParams &ellipseParams);
	void arc(double x, double y, double width, double height, double start, double stop, bool closed, bool roughClosure, const RoughOptions &o);
	void svgPath(const std::string &path, const RoughOptions &o);
	void solidFillPolygon(std::vector<Point> points, const RoughOptions &o);
	void patternFillPolygon(std::vector<Point> points, const RoughOptions &o);
	void patternFillArc(double x, double y, double width, double height, double start, double stop, const RoughOptions &o);
	// randOffset()
	// randOffsetWithRange()
	void doubleLineFillOps(double x1, double y1, double x2, double y2, const RoughOptions &o);

	void fillPath(const std::string &path, const RoughOptions &o);
private:
	RoughOptions cloneOptionsAlterSeed(const RoughOptions &options);
	double randomWithOptions(const RoughOptions &options);
	double offset(double min, double max, const RoughOptions &o, double roughnessGain = 1.0);
	double offsetOpt(double x, const RoughOptions &o, double roughnessGain = 1.0);
	void doubleLine(double x1, double y1, double x2, double y2, const RoughOptions &o, bool filling  = false);
	void privateLine(int x1, int y1, int x2, int y2, const RoughOptions &o, bool move, bool overlay);
	void curveWithOffset(const std::vector<Point> &points, double offset, const RoughOptions &o);
	void curve(const std::vector<Point> &points, std::optional<Point> closePoint, const RoughOptions &o);
	std::tuple<std::vector<Point>, std::vector<Point>> computeEllipsePoints(double increment, double cx, double cy, double rx, double ry, double offset, double overlap, const RoughOptions &o);
	void privateArc(double increment, double cx, double cy, double rx, double ry, double strt, double stp, double offset, const RoughOptions &o);
	void bezierTo(double x1, double y1, double x2, double y2, double x, double y, Point current, const RoughOptions &o);

	Graphics &graphics;
};

}

#endif /* ROUGH_RENDERER_H */
