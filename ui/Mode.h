#pragma once
#include <vector>
#include <string>
#include <map>
#include <any>
#include <iostream>
#include <cmath>

class Mode {
public:
    std::string name;
    std::vector<double> ratios;       // frequency ratios
    std::vector<std::string> labels;  // note labels
    std::map<std::string, std::any> meta; // open metadata

    Mode(const std::string& n,
         const std::vector<double>& r,
         const std::vector<std::string>& l,
         const std::map<std::string, std::any>& m = {})
        : name(n), ratios(r), labels(l), meta(m) {}

    void print() const {
        std::cout << "Mode: " << name << "\n";
        for (size_t i = 0; i < ratios.size(); i++) {
            std::cout << "  " << labels[i]
                      << " (" << ratios[i] << ")\n";
        }
        if (!meta.empty()) {
            std::cout << "  Meta:\n";
            for (const auto& [k, v] : meta) {
                std::cout << "    " << k << "\n";
            }
        }
    }
    void debugPrint(const std::string& base, int numKeys, double baseFreq) const {
        std::cout << "---- Mode Debug: " << name << " ----\n";
        std::cout << "Ratios: [ ";
        for (auto r : ratios) std::cout << r << " ";
        std::cout << "]\n";

        for (int i = 0; i < numKeys; i++) {
            int idx = i % ratios.size();
            int octave = i / ratios.size();
            double factor = std::pow(2.0, octave);
            double freq = baseFreq * ratios[idx] * factor;
            std::cout << "Key " << i 
                    << " label=" << ((idx < labels.size()) ? labels[idx] : "?")
                    << " ratio=" << ratios[idx]
                    << " octave=" << octave
                    << " -> freq=" << freq
                    << "\n";
        }
    }

    // --- Example mode ---
    static std::vector<Mode> Babel() {
        return {
            Mode("Custom Example",
                 {12.0/12, 12.0/11, 12.0/10, 12.0/9, 12.0/8},
                 {"C 12/12", "D 11/12", "E 10/12", "F 9/12", "G 8/12"},
                 {{"tradition", std::string("demo")}})
        };
    }

    // --- Equal Temperament Generator ---
    static Mode equalTemperament(int N,
        const std::string& name = "Equal Temperament")
    {
        std::vector<double> r;
        std::vector<std::string> l;

        for (int k = 0; k < N; k++) {
            double ratio = std::pow(2.0, double(k) / double(N));
            r.push_back(ratio);

            // simple label: Step k/N
            l.push_back("Step " + std::to_string(k) + "/" + std::to_string(N));
        }

        return Mode(name + " " + std::to_string(N),
                    r, l,
                    {{"system", std::string("equal temperament")}});
    }

};
