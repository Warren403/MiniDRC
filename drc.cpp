#include "drc.hpp"
#include <climits>
#include <algorithm>
#include <iostream>



std::string bboxStr(const Shape& s) {
    return "(" + std::to_string(s.x1) + "," + std::to_string(s.y1) + ")-(" +
           std::to_string(s.x2) + "," + std::to_string(s.y2) + ")";
}

// 回傳 metal 對 via 的「四邊最小邊界距離」(left,right,bottom,top 的最小值)；
// 若 metal 沒有至少覆蓋到 via（任何一邊在 via 裡面），會得到負值。
int enclosureMargin(const Shape& metal, const Shape& via) {
    int left   = via.x1 - metal.x1;
    int right  = metal.x2 - via.x2;
    int bottom = via.y1 - metal.y1;
    int top    = metal.y2 - via.y2;
    return std::min(std::min(left, right), std::min(bottom, top));
}


// =================== DRC ===================

void check_min_width(const std::vector<Shape>& shapes, const RuleSet& rules) {
    for (size_t i = 0; i < shapes.size(); ++i) {
        const auto& s = shapes[i];
        int sw = shortSide(s);                 // 線寬（短邊）
        int lw = std::max(rectW(s), rectH(s)); // 線長（長邊）

        // ---- 檢查 min_width ----
        if (s.layer == "M1" && sw < rules.min_width_M1)
            std::cout << "[WIDTH][M1] idx=" << i << " short=" << sw << " < " << rules.min_width_M1 << "\n";

        if (s.layer == "M2" && sw < rules.min_width_M2)
            std::cout << "[WIDTH][M2] idx=" << i << " short=" << sw << " < " << rules.min_width_M2 << "\n";

        // ---- 檢查 max_width ----
        if (s.layer == "M1" && lw > rules.max_width_M1)
            std::cout << "[WIDTH][M1] idx=" << i << " width=" << lw << " > " << rules.max_width_M1 << "\n";

        if (s.layer == "M2" && lw > rules.max_width_M2)
            std::cout << "[WIDTH][M2] idx=" << i << " width=" << lw << " > " << rules.max_width_M2 << "\n";
    }
}


void check_min_spacing(const std::vector<Shape>& shapes, const RuleSet& rules){
    for(size_t i=0;i<shapes.size();++i){
        for(size_t j=i+1;j<shapes.size();++j){
            if (shapes[i].layer != shapes[j].layer) continue;
            double d = rectSpacing(shapes[i], shapes[j]);
            if (shapes[i].layer=="M1" && d < rules.min_spacing_M1)
                std::cout << "[SPACING][M1] ("<<i<<","<<j<<") d="<<d<<" < "<<rules.min_spacing_M1<<"\n";
            if (shapes[i].layer=="M2" && d < rules.min_spacing_M2)
                std::cout << "[SPACING][M2] ("<<i<<","<<j<<") d="<<d<<" < "<<rules.min_spacing_M2<<"\n";
        }
    }
}

// =================== Enclosure Check ===================

void check_via_enclosure_multi(const std::vector<Shape>& shapes, const RuleSet& rules){
    static const bool SHOW_ALL_ENCLOSURE = false; // 改成 true 會列出所有 enclosure 狀況（含 OK）

    // 依 layer 分桶
    std::unordered_map<std::string, std::vector<const Shape*>> bucket;
    for (const auto& s : shapes)
        bucket[s.layer].push_back(&s);

    for (const auto& kv : rules.via_encl_map){
        const std::string& viaL = kv.first;
        const auto& cfg = kv.second; // {under, over, min_enclose}

        auto vit = bucket.find(viaL);
        if (vit == bucket.end()) continue; // layout 沒這層

        for (const Shape* vp : vit->second){
            const Shape& v = *vp;

            // 找出最佳包覆距離
            int best_under = INT_MIN, best_over = INT_MIN;

            if (bucket.count(cfg.under)){
                for (auto p : bucket[cfg.under])
                    if (touchOrOverlap(*p, v))
                        best_under = std::max(best_under, enclosureMargin(*p, v));
            }

            if (bucket.count(cfg.over)){
                for (auto p : bucket[cfg.over])
                    if (touchOrOverlap(*p, v))
                        best_over = std::max(best_over, enclosureMargin(*p, v));
            }

            const int need = cfg.min_enclose;

            auto print_encl = [&](const char* pos, const std::string& lay, int best){
                if (best == INT_MIN) {
                    std::cout << "[ENCLOSURE][" << pos << " " << lay << "] "
                              << viaL << " bbox=" << bboxStr(v)
                              << " missing metal coverage (need +" << need << "nm)\n";
                    return;
                }

                int diff = best - need;
                if (!SHOW_ALL_ENCLOSURE && std::abs(diff) < 1) return; // OK，不列出

                std::cout << "[ENCLOSURE][" << pos << " " << lay << "] "
                          << viaL << " bbox=" << bboxStr(v) << " ";

                if (diff < 0)
                    std::cout << "need +" << (-diff) << "nm\n";   // 不足
                else if (diff > 0)
                    std::cout << "over by " << diff << "nm\n";   // 過包
                else
                    std::cout << "OK (exact)\n";
            };

            print_encl("UNDER", cfg.under, best_under);
            print_encl("OVER ", cfg.over , best_over );
        }
    }
}





void check_density(const std::vector<Shape>& shapes, const RuleSet& rules,
                   int die_x1,int die_y1,int die_x2,int die_y2)
{
    const int W = rules.density_window;
    for (int y=die_y1; y<die_y2; y+=W){
        for (int x=die_x1; x<die_x2; x+=W){
            int win_x2 = std::min(x+W, die_x2);
            int win_y2 = std::min(y+W, die_y2);
            int win_area = (win_x2-x)*(win_y2-y);
            long long metal_area = 0;

            for (const auto& s: shapes){
                // 只計在 rules.density_layers 裡的層（不含 POLY/VIA）
                if (rules.density_layers.count(s.layer))
                    metal_area += interArea(x,y,win_x2,win_y2, s.x1,s.y1,s.x2,s.y2);
            }

            double dens = win_area? (double)metal_area/win_area : 0.0;
            if (dens < rules.min_density){
                std::cout << "[DENSITY] ["<<x<<","<<y<<"]-["<<win_x2<<","<<win_y2<<"] "
                          << "density="<<dens<<" < "<<rules.min_density<<"\n";
            }
        }
    }
}


