#ifndef ROUGH_PATH_DATA_PARSER_NORMALIZE_H
#define ROUGH_PATH_DATA_PARSER_NORMALIZE_H

#include <vector>

#include "Parser.h"

namespace Rough {
namespace PathDataParser {

extern std::vector<Segment> normalize(const std::vector<Segment> &segments);

} /* PathDataParser */
} /* Rough */
 
#endif /* ROUGH_PATH_DATA_PARSER_NORMALIZE_H */
