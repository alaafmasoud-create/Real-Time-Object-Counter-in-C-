#pragma once

#include "config.hpp"

#include <string>

AppConfig parseArguments(int argc, char** argv);
void printUsage(const std::string& executableName);

int parseIntegerOrThrow(const std::string& value, const std::string& optionName);
