#include "Parser.h"

#include <iterator>
#include <map>
#include <regex>
#include <stdexcept>

namespace Rough {
namespace PathDataParser {
 
std::vector<std::variant<char, double>> tokenize(const std::string &d)
{
	static const std::regex command("^([aAcChHlLmMqQsStTvVzZ])");
	static const std::regex number("^(([-+]?[0-9]+(\\.[0-9]*)?|[-+]?\\.[0-9]+)([eE][-+]?[0-9]+)?)");
	static const std::regex whitespace("^([\t\n\r ,]+)");

	std::vector<std::variant<char, double>> tokens;
	auto it = d.begin();

	while(it != d.end()) {
		std::smatch match;
		if(std::regex_search(it, d.end(), match, command)) {
			tokens.emplace_back(match[0].str().front());
		} else if(std::regex_search(it, d.end(), match, number)) {
			tokens.emplace_back(std::stod(match[0]));
		} else if(std::regex_search(it, d.end(), match, whitespace)) {
			// just skip whitespaces
		} else {
			return(std::vector<std::variant<char, double>>());
		}

		std::advance(it, match.length(1));
	}

	return tokens;
}

static const std::map<char, size_t> PARAMS_COUNT { 
	{ 'A', 7 }, { 'a', 7 }, { 'C', 6 }, { 'c', 6 }, { 'H', 1 }, { 'h', 1 }, 
	{ 'L', 2 }, { 'l', 2 }, { 'M', 2 }, { 'm', 2 }, { 'Q', 4 }, { 'q', 4 },
	{ 'S', 4 }, { 's', 4 }, { 'T', 2 }, { 't', 2 }, { 'V', 1 }, { 'v', 1 },
	{ 'Z', 0 }, { 'z', 0 } 
	};

std::vector<Segment> parsePath(const std::string &d)
{
	std::vector<Segment> segments;
	auto tokens = tokenize(d);

	if(tokens.empty()) {
		return(segments);
	}

	char mode;

	if(std::holds_alternative<double>(tokens.front()) || (std::get<char>(tokens.front()) != 'M' && std::get<char>(tokens.front()) != 'm')) {
		segments.emplace_back(Segment{ .key = 'M', .data = { 0.0, 0.0 } });
		mode = 'M';
	}
	
	for(auto it = tokens.begin(); it != tokens.end(); ) {
		std::vector<double> params;
		size_t paramsCount = 0;

		if(std::holds_alternative<char>(*it)) {
			mode = std::get<char>(*it);
			++it;
		}
		paramsCount = PARAMS_COUNT.at(mode);

		if(paramsCount <= (size_t) std::distance(it, tokens.end())) {
			for(size_t i = 0; i < paramsCount && it != tokens.end(); ++it, i++) {
				if(!std::holds_alternative<double>(*it)) {
					throw std::invalid_argument("Param not a number: " + std::get<char>(*it));
				}

				params.emplace_back(std::get<double>(*it));
			}

			if(params.size() == PARAMS_COUNT.at(mode)) {
				segments.emplace_back(Segment{  .key = mode, .data = params });

				if(mode == 'M')
					mode = 'L';
				if(mode == 'm')
					mode = 'l';
			} else {
				throw std::invalid_argument("Bad segment: " + mode);
			}
		} else {
			throw std::invalid_argument("Path data ended short");
		}
	}

	return(segments);
}

} /* PathDataParser */
} /* Rough */
