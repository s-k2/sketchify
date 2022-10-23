#ifndef ROUGH_FILL_SCANLINE_HACHURE
#define ROUGH_FILL_SCANLINE_HACHURE

#include <vector>

#include "../Core.h"

namespace Rough {
namespace ScanLineHachure {

extern std::vector<Line> polygonHachureLines(std::vector<Point> points, const RoughOptions &o);

} /* namespace ScanLineHachure */
} /* namespace Rough */

#endif /* ROUGH_FILL_SCANLINE_HACHURE */
