#include "common.hpp"
#include "drc.hpp"
#include "report.hpp"
#include <fstream>
#include <iostream>
#include <climits>
#include <algorithm>
#include <unordered_map>
#include <vector>
#include <string>


#ifndef DRC_VIOLATION_DEFINED
#define DRC_VIOLATION_DEFINED
struct Violation {
    std::string type;   
    std::string layer;  
    std::string object; 
    std::string bbox;   
    std::string rule;   
    double actual = 0.0; 
    double delta  = 0.0; 
    std::string status;  
};
#endif


#ifndef DRC_BBOXSTR_HELPER
#define DRC_BBOXSTR_HELPER
static inline std::string bboxStr(const Shape& s){
    return "(" + std::to_string(s.x1) + "," + std::to_string(s.y1) + ")-(" +
           std::to_string(s.x2) + "," + std::to_string(s.y2) + ")";
}
#endif


void writeDRCReport(const std::string& path,
                    const std::vector<Shape>& shapes,
                    const RuleSet& rules,
                    int die_x1,int die_y1,int die_x2,int die_y2)
{
    std::ofstream out(path);
    if(!out.is_open()){ std::cerr<<"ERROR: cannot write "<<path<<"\n"; return; }
    std::streambuf* bak = std::cout.rdbuf();
    std::cout.rdbuf(out.rdbuf());

    check_min_width(shapes, rules);
    check_min_spacing(shapes, rules);
    check_via_enclosure_multi(shapes, rules);
    check_density(shapes, rules, die_x1,die_y1,die_x2,die_y2);

    std::cout.rdbuf(bak);
}
 


void writeDRCReportTable(const std::string& path,
                         const std::vector<Shape>& shapes,
                         const RuleSet& rules,
                         int die_x1,int die_y1,int die_x2,int die_y2)
{std::cerr << "[DEBUG] reached writeDRCReportTable in report.cpp\n";
    std::ofstream out(path);
    if(!out.is_open()){ std::cerr<<"ERROR: cannot write "<<path<<"\n"; return; }

    auto csvEscape = [](const std::string& s)->std::string {
        bool need = false;
        for(char c : s)
            if(c==',' || c=='"' || c=='\n' || c=='\r'){ need = true; break; }
        if(!need) return s;
        std::string t; t.reserve(s.size()+2);
        t.push_back('"');
        for(char c : s){ t.push_back(c); if(c=='"') t.push_back('"'); }
        t.push_back('"');
        return t;
    };

 
    const double EPS = 1e-9;

    std::vector<Violation> V;


    for(size_t i=0;i<shapes.size();++i){
        const auto& s = shapes[i];
        int w = std::min(std::abs(s.x2 - s.x1), std::abs(s.y2 - s.y1));
        int L = std::max(std::abs(s.x2 - s.x1), std::abs(s.y2 - s.y1));

        if(s.layer=="M1"){
            if(w + EPS < rules.min_width_M1)
                V.push_back({"WIDTH","M1","idx="+std::to_string(i),bboxStr(s),
                    ">= "+std::to_string(rules.min_width_M1),
                    static_cast<double>(w),
                    static_cast<double>(rules.min_width_M1 - w),
                    "FAIL"});
            if(L > rules.max_width_M1 + EPS)
                V.push_back({"WIDTH","M1","idx="+std::to_string(i),bboxStr(s),
                    "<= "+std::to_string(rules.max_width_M1),
                    static_cast<double>(L),
                    static_cast<double>(L - rules.max_width_M1),
                    "FAIL"});
        } else if(s.layer=="M2"){
            if(w + EPS < rules.min_width_M2)
                V.push_back({"WIDTH","M2","idx="+std::to_string(i),bboxStr(s),
                    ">= "+std::to_string(rules.min_width_M2),
                    static_cast<double>(w),
                    static_cast<double>(rules.min_width_M2 - w),
                    "FAIL"});
            if(L > rules.max_width_M2 + EPS)
                V.push_back({"WIDTH","M2","idx="+std::to_string(i),bboxStr(s),
                    "<= "+std::to_string(rules.max_width_M2),
                    static_cast<double>(L),
                    static_cast<double>(L - rules.max_width_M2),
                    "FAIL"});
        }
    }



    auto spacing = [](const Shape& a,const Shape& b)->double{
        
        if(!(a.x2 < b.x1 || b.x2 < a.x1 || a.y2 < b.y1 || b.y2 < a.y1)) return 0.0;
        int dx=0,dy=0;
        if(a.x2<b.x1) dx=b.x1-a.x2; else if(b.x2<a.x1) dx=a.x1-b.x2;
        if(a.y2<b.y1) dy=b.y1-a.y2; else if(b.y2<a.y1) dy=a.y1-b.y2;
        return static_cast<double>(std::max(dx,dy));
    };

    for(size_t i=0;i<shapes.size();++i){
        for(size_t j=i+1;j<shapes.size();++j){
            if(shapes[i].layer != shapes[j].layer) continue;
            double d = spacing(shapes[i], shapes[j]);
            if(shapes[i].layer=="M1" && d + EPS < rules.min_spacing_M1)
                V.push_back({"SPACING","M1","("+std::to_string(i)+","+std::to_string(j)+")","-",
                    ">= "+std::to_string(rules.min_spacing_M1),
                    d,
                    static_cast<double>(rules.min_spacing_M1) - d,
                    "FAIL"});
            if(shapes[i].layer=="M2" && d + EPS < rules.min_spacing_M2)
                V.push_back({"SPACING","M2","("+std::to_string(i)+","+std::to_string(j)+")","-",
                    ">= "+std::to_string(rules.min_spacing_M2),
                    d,
                    static_cast<double>(rules.min_spacing_M2) - d,
                    "FAIL"});
        }
    }


    auto enclosureMarginLocal = [](const Shape& metal, const Shape& via)->int{
        int left   = via.x1 - metal.x1;
        int right  = metal.x2 - via.x2;
        int bottom = via.y1 - metal.y1;
        int top    = metal.y2 - via.y2;
        return std::min(std::min(left, right), std::min(bottom, top));
    };
    auto fullyCoversLocal = [](const Shape& m,const Shape& v)->bool{
        return m.x1<=v.x1 && m.y1<=v.y1 && m.x2>=v.x2 && m.y2>=v.y2;
    };
    auto overlap = [](const Shape& a,const Shape& b)->bool{
        return !(a.x2 < b.x1 || b.x2 < a.x1 || a.y2 < b.y1 || b.y2 < a.y1);
    };

    std::unordered_map<std::string, std::vector<const Shape*>> bucket;
    for(const auto& s: shapes) bucket[s.layer].push_back(&s);

    auto best_encl = [&](const Shape& v, const std::string& lay)->int{
        int best = INT_MIN;
        auto it = bucket.find(lay);
        if(it == bucket.end()) return best;

        for(auto p : it->second)
            if(fullyCoversLocal(*p, v))
                best = std::max(best, enclosureMarginLocal(*p, v));

        if(best == INT_MIN){
            for(auto p : it->second)
                if(overlap(*p, v))
                    best = std::max(best, enclosureMarginLocal(*p, v));
        }
        return best;
    };

    for(const auto& kv: rules.via_encl_map){
        const std::string& viaL = kv.first;
        const auto& cfg = kv.second;

        auto vit = bucket.find(viaL);
        if(vit == bucket.end()) continue;

        for(const Shape* vp : vit->second){
            const Shape& v = *vp;
            int best_under = best_encl(v, cfg.under);
            int best_over  = best_encl(v, cfg.over);

            auto pushEnc=[&](const char* pos, const std::string& lay, int best){
                if(best==INT_MIN){
                    V.push_back({"ENCLOSURE",std::string(pos)+" "+lay,viaL,bboxStr(v),
                        ">= "+std::to_string(cfg.min_enclose),
                        0.0,
                        static_cast<double>(cfg.min_enclose),
                        "FAIL"});
                } else if(best + EPS < cfg.min_enclose){
                    V.push_back({"ENCLOSURE",std::string(pos)+" "+lay,viaL,bboxStr(v),
                        ">= "+std::to_string(cfg.min_enclose),
                        static_cast<double>(best),
                        static_cast<double>(cfg.min_enclose - best),
                        "FAIL"});
                }
            };
            pushEnc("UNDER",cfg.under,best_under);
            pushEnc("OVER", cfg.over ,best_over );
        }
    }


    auto interAreaLocal = [](int ax1,int ay1,int ax2,int ay2,
                             int bx1,int by1,int bx2,int by2)->int{
        int x1 = std::max(ax1,bx1), y1 = std::max(ay1,by1);
        int x2 = std::min(ax2,bx2), y2 = std::min(ay2,by2);
        if(x2<=x1 || y2<=y1) return 0;
        return (x2-x1)*(y2-y1);
    };

    const int W = rules.density_window;
    for(int y=die_y1;y<die_y2;y+=W){
        for(int x=die_x1;x<die_x2;x+=W){
            int win_x2 = std::min(x+W, die_x2);
            int win_y2 = std::min(y+W, die_y2);
            int win_area = (win_x2 - x) * (win_y2 - y);
            long long metal_area = 0;

            for(const auto& s : shapes)
                if(rules.density_layers.count(s.layer))
                    metal_area += interAreaLocal(x, y, win_x2, win_y2,
                                                 s.x1, s.y1, s.x2, s.y2);

            double dens = win_area ? (double)metal_area / win_area : 0.0;
            if(dens + EPS < rules.min_density){
                V.push_back({
                    "DENSITY", "window",
                    "[" + std::to_string(x) + "," + std::to_string(y) + "]",
                    "(" + std::to_string(x) + "," + std::to_string(y) + ")-(" +
                        std::to_string(win_x2) + "," + std::to_string(win_y2) + ")",
                    ">= " + std::to_string(rules.min_density),
                    dens,
                    static_cast<double>(rules.min_density - dens),
                    "FAIL"
                });
            }
        }
    }


    out << "type,layer,object,bbox,rule,actual,delta,status\n";
    for (const auto& v : V) { 
        out << csvEscape(v.type)   << ','
            << csvEscape(v.layer)  << ','
            << csvEscape(v.object) << ','
            << csvEscape(v.bbox)   << ','
            << csvEscape(v.rule)   << ','
            << v.actual            << ','
            << v.delta             << ','
            << csvEscape(v.status) << '\n';
    }
    out.flush();
}
