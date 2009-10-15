#ifndef PARSER_H__
#define PARSER_H__

#include "db.h"

bool splitLine(const std::string &line, std::vector<std::string> &target);
int ParseLine(const std::string &line, Game &g, std::ostream &out);

#endif
