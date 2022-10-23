#include <argp.h>
#include <expat.h>
#include <stdio.h>
#include <string.h>

#include <string_view>
#include <ranges>
#include <regex>
#include <map>

#include "Renderer.h"
#include "Core.h"

#include "PointsOnPath/PointsOnPath.h"

class State;

class SvgPathRecorder : public Rough::Graphics
{
public:
        std::string getAndClear()
        {
                std::string tmp = std::move(path);
                path.clear();
                return(tmp);
        }

	virtual void moveTo(double x, double y)
        {
                path += "M " + std::to_string(x) + " " + std::to_string(y) + " ";
        }
        virtual void lineTo(double x, double y)
        {
                path += "L " + std::to_string(x) + " " + std::to_string(y) + " ";
        }
        virtual void bezierCurveTo(double x1, double y1, double x2, double y2, double x3, double y3)
        {
                path += "C " + std::to_string(x1) + " " + std::to_string(y1) + " "
                                + std::to_string(x2) + " " + std::to_string(y2) + " "
                                + std::to_string(x3) + " " + std::to_string(y3) + " ";
        }

private:
        std::string path;
};

struct State {
        State(FILE *in, FILE *out, const Rough::RoughOptions &roughOptions);

        FILE *getOutFile() { return(out); };
        FILE *getInFile() { return(in); };

        FILE *in;
        FILE *out;

        SvgPathRecorder svgRecorder;
        Rough::Renderer renderer;
        Rough::RoughOptions roughOptions;
};

State::State(FILE *in, FILE *out, const Rough::RoughOptions &roughOptions)
        : in(in), out(out), renderer(svgRecorder), roughOptions(roughOptions)
{
}

void expandStyleAttribute(std::map<std::string_view, std::string_view> &attributes, std::string_view style)
{
    const std::regex ws_re("\\s*([^:\\s]+)\\s*:\\s*([^;\\s]+)\\s*(;\\s*|$)");

    typedef std::regex_token_iterator<std::string_view::const_iterator> TokenIterator;
    TokenIterator it(style.begin(), style.end(), ws_re, { -1, 1, 2 });
    const TokenIterator end;

    std::pair<std::string_view, std::string_view> toInsert;
    for(; it != end; ++it) {
            if(it->first != it->second) {
                    if(toInsert.first.empty()) 
                            toInsert.first = std::string_view(it->first, it->second);
                    else if(toInsert.second.empty())
                            toInsert.second = std::string_view(it->first, it->second);
                    else
                            throw std::runtime_error("style malformed: illegal sequence: " + std::string(it->first, it->second));
            } else {
                    if(!toInsert.first.empty() && !toInsert.second.empty())
                            attributes.insert(toInsert);
                    toInsert.first = toInsert.second = "";
            }
    }
    if(!toInsert.first.empty() && !toInsert.second.empty()) {
            attributes.insert(toInsert);
    }
}

std::map<std::string_view, std::string_view> makeAttributesMap(const char **attributes)
{
        std::map<std::string_view, std::string_view> out;

        for(size_t i = 0; attributes[i]; i += 2) {
                auto toInsert = std::make_pair<std::string_view, std::string_view>(attributes[i], attributes[i + 1]);
                if(toInsert.first == "style")
                        expandStyleAttribute(out, toInsert.second);
                else
                        out.insert(toInsert);
        }

        return(out);
}

std::string_view extractAttributeOrDefault(std::map<std::string_view, std::string_view> &attributes, std::string_view key, std::string_view defaultValue = "")
{
        auto node = attributes.extract(key);
        return(node ? node.mapped() : defaultValue);
}

static void pathOut(State &state, std::string_view path, std::map<std::string_view, std::string_view> &attributes)
{
        std::string_view fill = extractAttributeOrDefault(attributes, "fill", "none");
        std::string_view stroke = extractAttributeOrDefault(attributes, "stroke", "none");

        fprintf(state.getOutFile(), "<g");
        for(const auto &attribute : attributes) {
                fprintf(state.getOutFile(), " %s=\"%s\"", std::string(attribute.first).data(), std::string(attribute.second).data());
        }
        fprintf(state.getOutFile(), ">");

        if(fill != "none") {
                const double distance = ((1 + state.roughOptions.roughness) / 2);

                auto sets = Rough::PointsOnPath::pointsOnPath(std::string(path), 1, distance);
                std::vector<Rough::Point> pointsOnPolygon;
                for(const auto &set : sets) {
                        for(const auto &point : set) {
                                pointsOnPolygon.push_back(point);
                        }
                }

                state.renderer.patternFillPolygon(pointsOnPolygon, state.roughOptions);
                fprintf(state.getOutFile(), "<path fill=\"none\" stroke=\"%s\" stroke-width=\"%.3f\" stroke-linecap=\"round\" d=\"%s\"></path>\n", std::string(fill).c_str(), state.roughOptions.strokeWidth, state.svgRecorder.getAndClear().c_str());
        }

        if(stroke != "none") {
                state.renderer.svgPath(std::string(path), state.roughOptions);
                fprintf(state.getOutFile(), "<path fill=\"none\" stroke=\"%s\" stroke-width=\"%.3f\" stroke-linecap=\"round\" d=\"%s\"></path>", std::string(stroke).c_str(), state.roughOptions.strokeWidth, state.svgRecorder.getAndClear().c_str());
        }

        fprintf(state.getOutFile(), "</g>");
}

static void sketchifyPath(State &state, const char **attributes_in)
{
        auto attributes = makeAttributesMap(attributes_in);

        pathOut(state, extractAttributeOrDefault(attributes, "d"), attributes);
}

static void sketchifyPolygon(State &state, const char **attributes_in)
{
        auto attributes = makeAttributesMap(attributes_in);

        std::string path = "M " + std::string(extractAttributeOrDefault(attributes, "points"));
        pathOut(state, path, attributes);
}

static void sketchifyRect(State &state, const char **attributes_in)
{
        auto attributes = makeAttributesMap(attributes_in);

        double x = std::atof(extractAttributeOrDefault(attributes, "x", "0.0").data());
        double y = std::atof(extractAttributeOrDefault(attributes, "y", "0.0").data());
        double width = std::atof(extractAttributeOrDefault(attributes, "width", "0.0").data());
        double height = std::atof(extractAttributeOrDefault(attributes, "height", "0.0").data());

        std::string path =
                "M " + std::to_string(x) + "," + std::to_string(y) + 
                "L " + std::to_string(x + width) + "," + std::to_string(y) + 
                "L " + std::to_string(x + width) + "," + std::to_string(y + height) + 
                "L " + std::to_string(x) + "," + std::to_string(y + height) + 
                "L " + std::to_string(x) + "," + std::to_string(y);

        pathOut(state, path, attributes);
}

static void sketchifyEllipse(State &state, const char **attributes_in)
{
        auto attributes = makeAttributesMap(attributes_in);

        std::string_view fill = extractAttributeOrDefault(attributes, "fill", "none");
        std::string_view stroke = extractAttributeOrDefault(attributes, "stroke", "none");

        double cx = std::atof(extractAttributeOrDefault(attributes, "cx", "0.0").data());
        double cy = std::atof(extractAttributeOrDefault(attributes, "cy", "0.0").data());
        double rx = std::atof(extractAttributeOrDefault(attributes, "rx", "0.0").data());
        double ry = std::atof(extractAttributeOrDefault(attributes, "ry", "0.0").data());

        auto ellipseParams = state.renderer.generateEllipseParams(2 * rx, 2 * ry, state.roughOptions);
        auto ellipseResponse = state.renderer.ellipseWithParams(cx, cy, state.roughOptions, ellipseParams);

        std::string strokePathD = state.svgRecorder.getAndClear();

        fprintf(state.getOutFile(), "<g");
        for(const auto &attribute : attributes) {
                fprintf(state.getOutFile(), " %s=\"%s\"", std::string(attribute.first).data(), std::string(attribute.second).data());
        }
        fprintf(state.getOutFile(), ">");

        if(stroke != "none") {
                fprintf(state.getOutFile(), "<path fill=\"none\" stroke=\"%s\" d=\"%s\"></path>", std::string(stroke).c_str(), strokePathD.c_str());
        }

        if(fill != "none") {
                state.renderer.patternFillPolygon(ellipseResponse, state.roughOptions);
                fprintf(state.getOutFile(), "<path fill=\"none\" stroke-width=\"0.5\" stroke=\"%s\" d=\"%s\"></path>", std::string(stroke).c_str(), state.svgRecorder.getAndClear().c_str());
        }

        fprintf(state.getOutFile(), "</g>");
}

 
static void XMLCALL startElement(void *userData, const XML_Char *name, const XML_Char **attributes)
{
	State *state = (State *) userData;

        if(!strcmp(name, "path")) {
                sketchifyPath(*state, attributes);
        } else if(!strcmp(name, "polygon")) {
                sketchifyPolygon(*state, attributes);
        } else if(!strcmp(name, "rect")) {
                sketchifyRect(*state, attributes);
        } else if(!strcmp(name, "ellipse")) {
                sketchifyEllipse(*state, attributes);
        } else {
                fprintf(state->getOutFile(), "<%s", name);

                for(size_t i = 0; attributes[i]; i += 2) {
                        fprintf(state->getOutFile(), " %s=\"%s\"", attributes[i], attributes[i + 1]);
                }

                fprintf(state->getOutFile(), ">\n");
        }
}

static void XMLCALL endElement(void *userData, const XML_Char *name)
{
	State *state = (State *) userData;

        if(strcmp(name, "path") && strcmp(name, "polygon") && strcmp(name, "rect") && strcmp(name, "ellipse"))
                fprintf(state->getOutFile(), "</%s>\n", name);
}

static void XMLCALL characterData(void *userData, const XML_Char *data, int len)
{
	State *state = (State *) userData;
        fwrite(data, len, 1, state->getOutFile());
}

static bool processSvg(FILE *in, FILE *out, const Rough::RoughOptions &roughOptions)
{
        char buf[BUFSIZ];
	XML_Parser parser = XML_ParserCreate(NULL);
	int done;
	State state(in, out, roughOptions);

	XML_SetUserData(parser, &state);
	XML_SetElementHandler(parser, startElement, endElement);
        XML_SetCharacterDataHandler(parser, characterData);
	do {
		size_t len = fread(buf, 1, sizeof(buf), state.in);
		done = len < sizeof(buf);
		if(XML_Parse(parser, buf, (int)len, done) == XML_STATUS_ERROR) {
			fprintf(stderr, "%s at line %lu\n",
					XML_ErrorString(XML_GetErrorCode(parser)),
					XML_GetCurrentLineNumber(parser));
			XML_ParserFree(parser);
			return(false);
		}
	} while(!done);
	XML_ParserFree(parser);

        return(true);
}

struct Config
{
        Rough::RoughOptions roughOptions;
        std::string infile;
        std::string outfile;
};

static char doc[] = "make svg files look sketchy - rough.js translated into c++";
const char *argp_program_version = "sketchify 0.1.0";
const char *argp_program_bug_address = "klein.stefan1@googlemail.com";

static char args_doc[] = "[SVG]";
static struct argp_option options[] = {
        { "max-randomness-offset", 'm', "units", 0, "maximum number of units a point will be translated" },
        { "roughness", 'r', "number", 0, "numerical value indicating how rough the drawing is" },
        { "bowing", 'b', "number", 0, "numerical value indicating how curvy the lines are" },
        { "disable-multi-stroke", 256, 0, OPTION_ARG_OPTIONAL, "don't apply multiple strokes to sketch a shape" },
        { "disable-multi-stroke-fill", 257, 0, OPTION_ARG_OPTIONAL, "don't apply multiple strokes to sketch the hachure lines filling a shape" },
        { "curve-tightness", 258, "ratio", 0, "number (0-1) indicating how thight curve-points follow the original points" },
        { "curve-fitting", 259, "ratio", 0, "number (0-1) indicating how close curves follow original curves" },
        { "curve-step-count", 260, "number", 0, "number of points to estimate the shape" },
        { "hachure-angle", 261, "angle", 0, "angle of hachure" },
        { "hachure-gap", 262, "distance", 0, "gap between hachure lines" },
        { "stroke-width", 'w', "width", 0, "width of sketchy strokes" },
        { "out", 'o', "file", 0, "place the output into file" },
        { 0 }
};

static error_t parse_opt(int key, char *arg, struct argp_state *state)
{
        Config *config = (Config *) state->input;

        try {
                switch(key) {
                        case 'm': config->roughOptions.maxRandomnessOffset = std::stod(arg); break;
                        case 'r': config->roughOptions.roughness = std::stod(arg); break;
                        case 'b': config->roughOptions.bowing = std::stod(arg); break;
                        case 256: config->roughOptions.disableMultiStroke = true; break;
                        case 257: config->roughOptions.disableMultiStrokeFill = true; break;
                        case 258: config->roughOptions.curveTightness = std::stod(arg); break;
                        case 259: config->roughOptions.curveFitting = std::stod(arg); break;
                        case 260: config->roughOptions.curveStepCount = std::stod(arg); break;
                        case 261: config->roughOptions.hachureAngle = std::stod(arg); break;
                        case 262: config->roughOptions.hachureGap = std::stod(arg); break;
                        case 'w': config->roughOptions.strokeWidth = std::stod(arg); break;
                        case 'o': config->outfile = arg; break; 

                        case ARGP_KEY_ARG:
                                if(state->arg_num > 1)
                                        argp_usage(state);
                                config->infile = arg;
                                break;
                        case ARGP_KEY_END:
                                if(state->arg_num != 1)
                                        config->infile = "-";
                                break;

                        default:
                                return ARGP_ERR_UNKNOWN;
                }
        } catch(std::invalid_argument &e) {
                fprintf(stderr, "invalid number: %s\n", arg);
        }

        return 0;
}

static struct argp argp = {options, parse_opt, args_doc, doc};

int main(int argc, char *argv[])
{
        Config config = {
                .roughOptions{
                        .maxRandomnessOffset = 2,
                        .roughness = 1,
                        .bowing = 1.5,
                        .disableMultiStroke = false,
                        .disableMultiStrokeFill = false,
                        .curveTightness = 0,
                        .curveFitting = 0.95,
                        .curveStepCount = 9,
                        .hachureAngle = -41,
                        .hachureGap = 3,
                        .strokeWidth = 0.5
                },
                .infile = "-",
                .outfile = "-"
        };
        argp_parse(&argp, argc, argv, 0, 0, &config);

        FILE *in = stdin;
        FILE *out = stdout;
        if(config.infile != "-") {
                if((in = fopen(config.infile.c_str(), "r")) == NULL) {
                        fprintf(stderr, "%s: cannot open file for read\n", config.infile.c_str());
                        return(1);
                }
        }
        if(config.outfile != "-") {
                if((out = fopen(config.outfile.c_str(), "w")) == NULL) {
                        fprintf(stderr, "%s: cannot open file for write\n", config.outfile.c_str());
                        return(1);
                }
        }


        return(processSvg(in, out, config.roughOptions) ? 0 : 1);
}
