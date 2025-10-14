#pragma once
#include <fstream>
#include <map>
#include <vector>
#include <string>
#include <stdexcept>
#include "Mode.h"
#include "../json.hpp"

using json = nlohmann::json;

class ModeLoader {
public:
    // ✅ Existing: load from file path
    static std::vector<Mode> fromJSON(const std::string& path) {
        std::ifstream in(path);
        if (!in.is_open()) {
            throw std::runtime_error("Could not open file: " + path);
        }

        json j;
        in >> j;
        return fromJSON(j); // ✅ delegate to new overload
    }

    // ✅ New: load directly from a JSON object (for server data)
    static std::vector<Mode> fromJSON(const json& j) {
        std::map<std::string, std::vector<double>> ratioGroups;
        std::map<std::string, std::vector<std::string>> labelGroups;

        for (const auto& entry : j) {
            std::string gah   = entry.value("gah_name", "Unknown");
            double ratio      = entry.value("ratio", 1.0);
            std::string label = entry.value("ratio_str", "?");

            ratioGroups[gah].push_back(ratio);
            labelGroups[gah].push_back(label);
        }

        std::vector<Mode> modes;
        for (auto& [gah, ratios] : ratioGroups) {
            modes.emplace_back(
                gah,
                ratios,
                labelGroups[gah]
            );
        }

        return modes;
    }
};













// #pragma once
// #include <fstream>
// #include <map>
// #include <vector>
// #include <string>
// #include <stdexcept>
// #include "Mode.h"
// #include "../json.hpp"

// using json = nlohmann::json;

// class ModeLoader {
// public:
//     static std::vector<Mode> fromJSON(const std::string& path) {
//         std::ifstream in(path);
//         if (!in.is_open()) {
//             throw std::runtime_error("Could not open file: " + path);
//         }

//         json j;
//         in >> j;

//         std::map<std::string, std::vector<double>> ratioGroups;
//         std::map<std::string, std::vector<std::string>> labelGroups;

//         for (const auto& entry : j) {
//             std::string gah   = entry["gah_name"];
//             double ratio      = entry["ratio"];
//             std::string label = entry["ratio_str"];

//             ratioGroups[gah].push_back(ratio);
//             labelGroups[gah].push_back(label);
//         }

//         std::vector<Mode> modes;
//         for (auto& [gah, ratios] : ratioGroups) {
//             modes.emplace_back(
//                 gah,
//                 ratios,
//                 labelGroups[gah]   // no meta
//             );
//         }
//         return modes;
//     }
// };
