#pragma once
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <array>
#include <sstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

struct Shape { std::string layer; int x1,y1,x2,y2; };
struct Label { std::string name, layer; int x,y; };

// ---- 規則 ----
struct RuleSet {
    int min_spacing_M1, min_spacing_M2;
    int min_width_M1,   min_width_M2;
    int max_width_M1,   max_width_M2;
    int via_enclose;
    int density_window;
    double min_density;

    
    std::unordered_map<std::string, std::array<std::string,2>> via_conn;
    std::unordered_set<std::string> conductive_layers;
    struct Encl { std::string under, over; int min_enclose; };
    std::unordered_map<std::string, Encl> via_encl_map;

    // 密度計算要納入哪些層（預設只算金屬，不含 POLY/任何 VIA）
    std::unordered_set<std::string> density_layers;
};


// ---- DSU ----
struct DSU{
    std::vector<int> p, r;
    DSU(int n=0){ reset(n); }
    void reset(int n){ p.resize(n); r.assign(n,0); for(int i=0;i<n;++i)p[i]=i; }
    int find(int x){ return p[x]==x?x:p[x]=find(p[x]); }
    void unite(int a,int b){ a=find(a); b=find(b); if(a==b) return; if(r[a]<r[b]) std::swap(a,b); p[b]=a; if(r[a]==r[b]) r[a]++; }
};

// ---- 幾何小工具----
inline int  rectW(const Shape& s){ return std::abs(s.x2 - s.x1); }
inline int  rectH(const Shape& s){ return std::abs(s.y2 - s.y1); }
inline int  shortSide(const Shape& s){ return std::min(rectW(s), rectH(s)); }

inline double rectSpacing(const Shape& a, const Shape& b){
    int dx=0, dy=0;
    if (a.x2 <= b.x1) dx = b.x1 - a.x2;
    else if (b.x2 <= a.x1) dx = a.x1 - b.x2;
    if (a.y2 <= b.y1) dy = b.y1 - a.y2;
    else if (b.y2 <= a.y1) dy = a.y1 - b.y2;
    if (dx==0 && dy==0) return 0.0;
    if (dx==0) return (double)dy;
    if (dy==0) return (double)dx;
    return std::sqrt((double)dx*dx + (double)dy*dy);
}

inline bool rectCovers(const Shape& a, int bx1,int by1,int bx2,int by2){
    return a.x1 <= bx1 && a.y1 <= by1 && a.x2 >= bx2 && a.y2 >= by2;
}
inline int interArea(int ax1,int ay1,int ax2,int ay2,
                     int bx1,int by1,int bx2,int by2){
    int x1 = std::max(ax1, bx1), y1 = std::max(ay1, by1);
    int x2 = std::min(ax2, bx2), y2 = std::min(ay2, by2);
    if (x2 <= x1 || y2 <= y1) return 0;
    return (x2 - x1) * (y2 - y1);
}
inline bool contains(const Shape& s, int x, int y){
    return x >= s.x1 && x <= s.x2 && y >= s.y1 && y <= s.y2;
}
inline bool touchOrOverlap(const Shape& a, const Shape& b){
    bool xo = !(a.x2 < b.x1 || b.x2 < a.x1);
    bool yo = !(a.y2 < b.y1 || b.y2 < a.y1);
    return xo && yo;
}
