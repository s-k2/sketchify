#ifndef ROUGH_PATH_DATA_PARSER_ABSOLUTIZE_H
#define ROUGH_PATH_DATA_PARSER_ABSOLUTIZE_H

#include <vector>

#include "Parser.h"

namespace Rough {
namespace PathDataParser {

extern std::vector<Segment> absolutize(const std::vector<Segment> &segments);

} /* PathDataParser */
} /* Rough */
 
#endif /* ROUGH_PATH_DATA_PARSER_ABSOLUTIZE_H */
