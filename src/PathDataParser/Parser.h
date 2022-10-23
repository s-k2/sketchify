#ifndef ROUGH_PATH_DATA_PARSER_PARSER_H
#define ROUGH_PATH_DATA_PARSER_PARSER_H

#include <string>
#include <variant>
#include <vector>

namespace Rough {
namespace PathDataParser {

struct Segment
{
	char key;
	std::vector<double> data;
};

extern std::vector<std::variant<char, double>> tokenize(const std::string &d);
extern std::vector<Segment> parsePath(const std::string &d);

} /* PathDataParser */
} /* Rough */
 
#endif /* ROUGH_PATH_DATA_PARSER_PARSER_H */
