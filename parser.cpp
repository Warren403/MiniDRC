#include "parser.hpp"

// 讀取版圖矩形列表：每行格式為 "<layer> <x1> <y1> <x2> <y2>"
std::vector<Shape> readLayout(const std::string& filename){
    std::vector<Shape> v;                 // 用來存所有矩形 (layer 與座標)
    std::ifstream fin(filename);          // 開檔（RAII：離開作用域自動關閉）
    if(!fin.is_open()){                   // 檔案開啟失敗就回傳空 vector
        std::cerr<<"Error opening "<<filename<<"\n";
        return v;
    }
    std::string layer; int x1,y1,x2,y2;   // 暫存單行欄位
    // 透過 operator>> 逐欄讀取；只要一欄讀失敗（含 EOF）整行就結束迴圈
    while(fin >> layer >> x1 >> y1 >> x2 >> y2)
        // 以聚合初始化(aggregate initialization) 直接建構 Shape 並 push_back
        v.push_back({layer,x1,y1,x2,y2});
    return v;                              // 回傳收集到的所有 Shape（RVO/NRVO）
}


// 讀取設計規則（DRC/密度/導電層/導通資訊…）自 JSON 檔
RuleSet readRules(const std::string& filename){
    std::ifstream fin(filename);
    if(!fin.is_open()){
        throw std::runtime_error("Cannot open rules file: " + filename);
    }

    json j;
    try {
        fin >> j;
    } catch (const json::parse_error& e) {
        throw std::runtime_error(
            std::string("rules.json parse error at byte ") + std::to_string(e.byte) +
            ": " + e.what()
        );
    }

    auto need = [&](const char* path){
        if(!j.contains(path)) {
            throw std::runtime_error(std::string("rules.json missing key: ") + path);
        }
    };

    // 逐項檢查關鍵鍵值是否存在
    need("min_spacing"); need("min_width"); need("max_width");
    need("density_check");

    RuleSet r{};

    try {
        r.min_spacing_M1 = j["min_spacing"].at("M1").get<int>();
        r.min_spacing_M2 = j["min_spacing"].at("M2").get<int>();
        r.min_width_M1   = j["min_width"].at("M1").get<int>();
        r.min_width_M2   = j["min_width"].at("M2").get<int>();
        r.max_width_M1   = j["max_width"].at("M1").get<int>();
        r.max_width_M2   = j["max_width"].at("M2").get<int>();
        r.density_window = j["density_check"].at("window_size").get<int>();
        r.min_density    = j["density_check"].at("min_density").get<double>();

        if (j.contains("via_enclosure")) {
            for (auto it = j["via_enclosure"].begin(); it != j["via_enclosure"].end(); ++it) {
                const std::string via = it.key();
                const auto &obj = it.value();
                if (obj.contains("under") && obj.contains("over") && obj.contains("min_enclose")) {
                    r.via_encl_map[via] = RuleSet::Encl{
                        obj.at("under").get<std::string>(),
                        obj.at("over").get<std::string>(),
                        obj.at("min_enclose").get<int>()
                    };
                }
            }
        }

        if (j.contains("conductive_layers")) {
            for (const auto& s : j["conductive_layers"])
                r.conductive_layers.insert(s.get<std::string>());
        }

        if (j.contains("density_layers")) {
            for (const auto& s : j["density_layers"])
                r.density_layers.insert(s.get<std::string>());
        } else {
            r.density_layers.insert("M1");
            r.density_layers.insert("M2");
            r.density_layers.insert("M3");
        }
    } catch (const json::type_error& e) {
        throw std::runtime_error(std::string("rules.json type error: ") + e.what());
    } catch (const json::out_of_range& e) {
        throw std::runtime_error(std::string("rules.json missing subkey: ") + e.what());
    }

    return r;
}
