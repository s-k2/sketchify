#include "Absolutize.h"

namespace Rough {
namespace PathDataParser {

std::vector<Segment> absolutize(const std::vector<Segment> &segments)
{
	double cx = 0, cy = 0;
	double subx = 0, suby = 0;
	std::vector<Segment> out;
	for(const auto &segment : segments) {
		const auto &data = segment.data;
		switch(segment.key) {
			case 'M':
				out.emplace_back(Segment{ .key = 'M', .data = data });
				cx = data[0], cy = data[1];
				subx = data[0], suby = data[1];
				break;
			case 'm':
				cx += data[0];
				cy += data[1];
				out.emplace_back(Segment{ .key = 'M', .data = std::vector<double>{ cx, cy } });
				subx = cx;
				suby = cy;
				break;
			case 'L':
				out.emplace_back(Segment{ .key = 'L', .data = data });
				cx = data[0], cy = data[1];
				break;
			case 'l':
				cx += data[0];
				cy += data[1];
				out.emplace_back(Segment{ .key = 'L', .data = std::vector<double>{ cx, cy } });
				break;
			case 'C':
				out.emplace_back(Segment{ .key = 'C', .data = data });
				cx = data[4];
				cy = data[5];
				break;
			case 'c': {
					const auto newdata = std::vector<double>{ data[0] + cx, data[1] + cy, data[2] + cx, data[3] + cy, data[4] + cx, data[5] + cy };
					out.emplace_back(Segment{ .key = 'C', .data = newdata });
					cx = newdata[4];
					cy = newdata[5];
					break;
				  }
			case 'Q':
				out.emplace_back(Segment{ .key = 'Q', .data = data });
				cx = data[2];
				cy = data[3];
				break;
			case 'q': {
					const auto newdata = std::vector<double>{ data[0] + cx, data[1] + cy, data[2] + cx, data[3] + cy };
					out.emplace_back(Segment{ .key = 'Q', .data = newdata });
					cx = newdata[2];
					cy = newdata[3];
					break;
				  }
			case 'A':
				out.emplace_back(Segment{ .key = 'A', .data = data });
				cx = data[5];
				cy = data[6];
				break;
			case 'a':
				cx += data[5];
				cy += data[6];
				out.emplace_back(Segment{ .key = 'A', .data = std::vector<double>{ data[0], data[1], data[2], data[3], data[4], cx, cy } });
				break;
			case 'H':
				out.emplace_back(Segment{ .key = 'H', .data = data });
				cx = data[0];
				break;
			case 'h':
				cx += data[0];
				out.emplace_back(Segment{ .key = 'H', .data = std::vector<double>{ cx } });
				break;
			case 'V':
				out.emplace_back(Segment{ .key = 'V', .data = data });
				cy = data[0];
				break;
			case 'v':
				cy += data[0];
				out.emplace_back(Segment{ .key = 'V', .data = std::vector<double>{ cy } });
				break;
			case 'S':
				out.emplace_back(Segment{ .key = 'S', .data = data });
				cx = data[2];
				cy = data[3];
				break;
			case 's': {
					const auto newdata = std::vector<double>{ data[0] + cx, data[1] + cy, data[2] + cx, data[3] + cy };
					out.emplace_back(Segment{ .key = 'S', .data = newdata });
					cx = newdata[2];
					cy = newdata[3];
					break;
				  }
			case 'T':
				out.emplace_back(Segment{ .key = 'T', .data = data });
				cx = data[0];
				cy = data[1];
				break;
			case 't':
				cx += data[0];
				cy += data[1];
				out.emplace_back(Segment{ .key = 'T', .data = std::vector<double>{ cx, cy } });
				break;
			case 'Z':
			case 'z':
				out.emplace_back(Segment{ .key = 'Z', .data = std::vector<double>{ } });
				cx = subx;
				cy = suby;
				break;
		}
	}
	return out;
}

} /* PathDataParser */
} /* Rough */
 
