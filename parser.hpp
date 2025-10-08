#pragma once
#include "common.hpp"

std::vector<Shape> readLayout(const std::string& filename);
RuleSet readRules(const std::string& filename);
std::vector<Label> readLabels(const std::string& file);
std::unordered_map<std::string, std::vector<std::string>>
readSchematic(const std::string& file);
