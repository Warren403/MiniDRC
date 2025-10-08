#pragma once
#include "common.hpp"
#include <vector>
#include <string>


int enclosureMargin(const Shape& metal, const Shape& via);
bool fullyCovers(const Shape& metal, const Shape& via);


void check_min_width(const std::vector<Shape>& shapes, const RuleSet& rules);
void check_min_spacing(const std::vector<Shape>& shapes, const RuleSet& rules);
void check_via_enclosure_multi(const std::vector<Shape>& shapes, const RuleSet& rules);
void check_density(const std::vector<Shape>& shapes, const RuleSet& rules,
                   int die_x1,int die_y1,int die_x2,int die_y2);
