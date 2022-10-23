#include "Normalize.h"

#include <algorithm>
#include <array>
#include <math.h>
#include <ranges>
#include <utility>

namespace Rough {
namespace PathDataParser {

double degToRad(double degrees);
std::pair<double, double> rotate(double x, double y, double angleRad);
std::vector<std::vector<double>> arcToCubicCurves(double x1, double y1, double x2, double y2, double r1, double r2, double angle, bool largeArcFlag, bool sweepFlag, std::vector<double> recursive = std::vector<double>{});
 
// Normalize path to include only M, L, C, and Z commands
extern std::vector<Segment> normalize(const std::vector<Segment> &segments)
{
	std::vector<Segment> out;
	char lastType = 0x00;
	double cx = 0, cy = 0;
	double subx = 0, suby = 0;
	double lcx = 0, lcy = 0;

	for(const auto &segment : segments) {
		const auto &data = segment.data;
		switch(segment.key) {
			case 'M':
				out.emplace_back(Segment{ .key = 'M', .data = data });
				cx = data[0], cy = data[1];
				subx = data[0], suby = data[1];
				break;
			case 'C':
				out.emplace_back(Segment{ .key = 'C', .data = data });
				cx = data[4];
				cy = data[5];
				lcx = data[2];
				lcy = data[3];
				break;
			case 'L':
				out.emplace_back(Segment{ .key = 'L', .data = data });
				cx = data[0], cy = data[1];
				break;
			case 'H':
				cx = data[0];
				out.emplace_back(Segment{ .key = 'L', .data = std::vector<double>{ cx, cy } });
				break;
			case 'V':
				cy = data[0];
				out.emplace_back(Segment{ .key = 'L', .data = std::vector<double>{ cx, cy } });
				break;
			case 'S': {
					  double cx1 = 0, cy1 = 0;
					  if(lastType == 'C' || lastType == 'S') {
						  cx1 = cx + (cx - lcx);
						  cy1 = cy + (cy - lcy);
					  } else {
						  cx1 = cx;
						  cy1 = cy;
					  }

					  out.emplace_back(Segment{ .key = 'C', .data = std::vector<double>{ cx1, cy1, data[0], data[1], data[2], data[3] } });
					  lcx = data[0];
					  lcy = data[1];
					  cx = data[2];
					  cy = data[3];
					  break;
				  }
			case 'T': {
					  const double x = data[0], y = data[1];
					  double x1 = 0, y1 = 0;
					  if(lastType == 'Q' || lastType == 'T') {
						  x1 = cx + (cx - lcx);
						  y1 = cy + (cy - lcy);
					  } else {
						  x1 = cx;
						  y1 = cy;
					  }
					  const double cx1 = cx + 2 * (x1 - cx) / 3;
					  const double cy1 = cy + 2 * (y1 - cy) / 3;
					  const double cx2 = x + 2 * (x1 - x) / 3;
					  const double cy2 = y + 2 * (y1 - y) / 3;
					  out.emplace_back(Segment{ .key = 'C', .data = std::vector<double>{ cx1, cy1, cx2, cy2, x, y } });
					  lcx = x1;
					  lcy = y1;
					  cx = x;
					  cy = y;
					  break;
				  }
			case 'Q': {
					  const double x1 = data[0], y1 = data[1], x = data[2], y = data[3];
					  const double cx1 = cx + 2 * (x1 - cx) / 3;
					  const double cy1 = cy + 2 * (y1 - cy) / 3;
					  const double cx2 = x + 2 * (x1 - x) / 3;
					  const double cy2 = y + 2 * (y1 - y) / 3;
					  out.emplace_back(Segment{ .key = 'C', .data = std::vector<double>{ cx1, cy1, cx2, cy2, x, y } });
					  lcx = x1;
					  lcy = y1;
					  cx = x;
					  cy = y;
					  break;
				  }
			case 'A': {
					  const double r1 = abs(data[0]);
					  const double r2 = abs(data[1]);
					  const double angle = data[2];
					  const bool largeArcFlag = data[3] > 0.5; // funny way to check if a float is not zero
					  const bool sweepFlag = data[4] > 0.5;
					  const double x = data[5];
					  const double y = data[6];
					  if(r1 == 0 || r2 == 0) {
						  out.emplace_back(Segment{ .key = 'C', .data = std::vector<double>{ cx, cy, x, y, x, y } });
						  cx = x;
						  cy = y;
					  } else {
						  if(cx != x || cy != y) {
							  std::vector<std::vector<double>> curves = arcToCubicCurves(cx, cy, x, y, r1, r2, angle, largeArcFlag, sweepFlag);

							  for(const auto &curve : curves) {
								  out.emplace_back(Segment{ .key = 'C', .data = curve });
							  };
							  cx = x;
							  cy = y;
						  }
					  }
					  break;
				  }
			case 'Z':
				  out.emplace_back(Segment{ .key = 'Z', .data = std::vector<double>{ } });
				  cx = subx;
				  cy = suby;
				  break;
		}
		lastType = segment.key;
	}
	return out;
}

double degToRad(double degrees)
{
	return(M_PI * degrees) / 180;
}

std::pair<double, double> rotate(double x, double y, double angleRad)
{
	const double X = x * cos(angleRad) - y * sin(angleRad);
	const double Y = x * sin(angleRad) + y * cos(angleRad);
	return std::make_pair(X, Y);
}

std::vector<std::vector<double>> arcToCubicCurves(double x1, double y1, double x2, double y2, double r1, double r2, double angle, bool largeArcFlag, bool sweepFlag, std::vector<double> recursive)
{
	const double angleRad = degToRad(angle);
	std::vector<std::vector<double>> params;

	double f1 = 0, f2 = 0, cx = 0, cy = 0;
	if(!recursive.empty()) {
		f1 = recursive[0], f2 = recursive[1], cx = recursive[2], cy = recursive[3];
	} else {
		auto p1 = rotate(x1, y1, -angleRad);
		auto p2 = rotate(x2, y2, -angleRad);
		x1 = p1.first;
		y1 = p1.second;
		x2 = p2.first;
		y2 = p2.second;

		const double x = (x1 - x2) / 2;
		const double y = (y1 - y2) / 2;
		double h = (x * x) / (r1 * r1) + (y * y) / (r2 * r2);
		if(h > 1) {
			h = sqrt(h);
			r1 = h * r1;
			r2 = h * r2;
		}

		const double sign = (largeArcFlag == sweepFlag) ? -1 : 1;

		const double r1Pow = r1 * r1;
		const double r2Pow = r2 * r2;

		const double left = r1Pow * r2Pow - r1Pow * y * y - r2Pow * x * x;
		const double right = r1Pow * y * y + r2Pow * x * x;

		const double k = sign * sqrt(abs(left / right));

		cx = k * r1 * y / r2 + (x1 + x2) / 2;
		cy = k * -r2 * x / r1 + (y1 + y2) / 2;

		f1 = asin((y1 - cy) / r2); // TODO: was toFixed(9)
		f2 = asin((y2 - cy) / r2); // TODO: was toFixed(9)

		if(x1 < cx) {
			f1 = M_PI - f1;
		}
		if(x2 < cx) {
			f2 = M_PI - f2;
		}

		if(f1 < 0) {
			f1 = M_PI * 2 + f1;
		}
		if(f2 < 0) {
			f2 = M_PI * 2 + f2;
		}

		if(sweepFlag && f1 > f2) {
			f1 = f1 - M_PI * 2;
		}
		if(!sweepFlag && f2 > f1) {
			f2 = f2 - M_PI * 2;
		}
	}

	double df = f2 - f1;

	if(abs(df) > (M_PI * 120 / 180)) {
		const double f2old = f2;
		const double x2old = x2;
		const double y2old = y2;

		if(sweepFlag && f2 > f1) {
			f2 = f1 + (M_PI * 120 / 180) * (1);
		}
		else {
			f2 = f1 + (M_PI * 120 / 180) * (-1);
		}

		x2 = cx + r1 * cos(f2);
		y2 = cy + r2 * sin(f2);
		params = arcToCubicCurves(x2, y2, x2old, y2old, r1, r2, angle, 0, sweepFlag, std::vector<double>{f2, f2old, cx, cy});
	}

	df = f2 - f1;

	const double c1 = cos(f1);
	const double s1 = sin(f1);
	const double c2 = cos(f2);
	const double s2 = sin(f2);
	const double t = tan(df / 4);
	const double hx = 4.0 / 3 * r1 * t;
	const double hy = 4.0 / 3 * r2 * t;

	const auto m1 = std::array<double, 2>{x1, y1};
	auto m2 = std::array<double, 2>{x1 + hx * s1, y1 - hy * c1};
	const auto m3 = std::array<double, 2>{x2 + hx * s2, y2 - hy * c2};
	const auto m4 = std::array<double, 2>{x2, y2};

	m2[0] = 2 * m1[0] - m2[0];
	m2[1] = 2 * m1[1] - m2[1];

	if(!recursive.empty()) {
		auto paramsOut = std::vector<std::vector<double>>{ std::vector<double>{ m2[0], m2[1] }, std::vector<double>{ m3[0], m3[1] }, std::vector<double>{ m4[0], m4[1] } };
		std::ranges::copy(params, std::back_inserter(paramsOut));
		return paramsOut;
	}
	else {
		params.insert(params.begin(), std::vector<double>{ m4[0], m4[1] });
		params.insert(params.begin(), std::vector<double>{ m3[0], m3[1] });
		params.insert(params.begin(), std::vector<double>{ m2[0], m2[1] });
		std::vector<std::vector<double>> curves;
		for(size_t i = 0; i < params.size(); i += 3) {
			const auto [r1_0, r1_1] = rotate(params[i][0], params[i][1], angleRad);
			const auto [r2_0, r2_1] = rotate(params[i + 1][0], params[i + 1][1], angleRad);
			const auto [r3_0, r3_1] = rotate(params[i + 2][0], params[i + 2][1], angleRad);
			curves.emplace_back(std::vector<double>{ r1_0, r1_1, r2_0, r2_1, r3_0, r3_1 });
		}
		return curves;
	}
}

} /* PathDataParser */
} /* Rough */

