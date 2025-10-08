#pragma once
#include <string>
#include <vector>
#include <iostream>
#include <iomanip>
#include "common.hpp"   


static constexpr double EPS = 1e-9;

struct ReportRow {
    std::string type;     // WIDTH / SPACING / ENCLOSURE / DENSITY
    std::string layer;    
    std::string object;   
    std::string bbox;     
    std::string rule;     
    double actual = 0.0;  
    double threshold = 0.0;
    double margin = 0.0;  
    std::string status;   
};

// 文字報告（把 check_* 的輸出導到檔案）
void writeDRCReport(const std::string& path,
                    const std::vector<Shape>& shapes,
                    const RuleSet& rules,
                    int die_x1,int die_y1,int die_x2,int die_y2);

// CSV 報表
void writeDRCReportTable(const std::string& path,
                         const std::vector<Shape>& shapes,
                         const RuleSet& rules,
                         int die_x1,int die_y1,int die_x2,int die_y2);

class ReportWriter {
public:
    void add(const ReportRow& r) { rows.push_back(r); }

    static bool pass_ge(double actual, double thr) { return (actual + EPS) >= thr; }

    void print(std::ostream& os, const std::string& fmt="md") const {
        if (fmt == "csv") {
            os << "type,layer,object,bbox,rule,actual,margin,status\n";
            for (const auto& r : rows) {
                os << r.type << "," << r.layer << "," << r.object << ","
                   << r.bbox << "," << r.rule << ","
                   << r.actual << "," << r.margin << ","
                   << r.status << "\n";
            }
        } else {
            os << "| type | layer | object | bbox | rule | actual | margin | status |\n";
            os << "|------|--------|---------|-------|-------|--------:|--------:|--------|\n";
            for (const auto& r : rows) {
                os << "| " << r.type << " | " << r.layer << " | " << r.object
                   << " | " << r.bbox << " | " << r.rule << " | "
                   << r.actual << " | " << r.margin << " | " << r.status << " |\n";
            }
        }
    }

private:
    std::vector<ReportRow> rows;
};
