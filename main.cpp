#include "parser.hpp"
#include "drc.hpp"
#include "report.hpp"
#include <windows.h>
#include <algorithm>
#include <cctype>
#include <string>
#include <vector>
#include <iostream>




static std::vector<std::string> find_layouts() {
    std::vector<std::string> files;
    WIN32_FIND_DATAA data;
    HANDLE h = FindFirstFileA("layout*.txt", &data);
    if (h != INVALID_HANDLE_VALUE) {
        do {
            if (!(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                files.push_back(data.cFileName);
            }
        } while (FindNextFileA(h, &data));
        FindClose(h);
    }
    std::sort(files.begin(), files.end());
    return files;
}

static std::string choose_layout() {
    auto files = find_layouts();
    std::cout << "\n=== 選擇要讀的 layout 檔 ===\n";
    if (files.empty()) {
        std::cout << "（找不到，請直接輸入檔名，如：layout 1.txt）\n> ";
        std::string manual; std::getline(std::cin, manual);
        if (manual.empty()) std::getline(std::cin, manual); // 若前面有殘留換行
        return manual;
    }
    for (size_t i=0;i<files.size();++i)
        std::cout << "  ["<<i+1<<"] " << files[i] << "\n";
    std::cout << "輸入編號（Enter=第1個），或直接輸入檔名： ";
    std::string input; std::getline(std::cin, input);
    if (input.empty()) return files[0];

    bool allDigit = !input.empty() && std::all_of(input.begin(), input.end(), ::isdigit);
    if (allDigit) {
        int idx = std::stoi(input);
        if (idx>=1 && idx<=(int)files.size()) return files[idx-1];
        std::cout << "超出範圍，改用第1個。\n";
        return files[0];
    }
    return input; // 當作手動檔名
}


int main(){
    
    // 1) 先選 layout 檔
    std::string layoutFile = choose_layout();
    std::cout << "使用檔案: " << layoutFile << "\n";

    // 2) 讀 layout / rules
    auto shapes = readLayout(layoutFile);
    RuleSet rules = readRules("rules.json");
    std::cout << "Loaded " << shapes.size() << " shapes\n";

    // 4) DRC
    check_min_width(shapes, rules);
    check_min_spacing(shapes, rules);
    check_via_enclosure_multi(shapes, rules);
    check_density(shapes, rules, 0,0, 200,100);
    writeDRCReport("drc_report.txt", shapes, rules, 0,0,200,100);
    std::cout << "DRC results saved to drc_report.txt\n";

    // 5) 表格化 (CSV by print_table )
    writeDRCReportTable("drc_fail_table.csv", shapes, rules, 0,0,200,100);
    std::cout << "DRC table saved to drc_fail_table.csv\n";

    return 0;
}
