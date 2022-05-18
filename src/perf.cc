// Test for performance

#include <chrono>
#include <iostream>
#include <regex>
#include <optional>
#include <vector>
#include "affine.h"
#include <x86intrin.h>

using namespace Affine;
const char* current_test_case;

struct TestCase {
    const char* name;
    std::function <void()> f; // call to run test

    TestCase(const char* name, std::function <void()> f) {
        this->name = name;
        this->f = f;
    }

    void run() const {
        current_test_case = name;
        // std::cout << name << std::endl;

        try {
            f();
        } catch (std::exception& e) {
            std::cout << "Test case " << current_test_case << " raised exception: " << e.what() << std::endl;
        }

        current_test_case = "undefined";
    }
};

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define _assert(cond) do { if (!(cond)) throw std::runtime_error(std::string{"Test case "} + current_test_case + " failed assertion at line " TOSTRING(__LINE__)); } while (0)

AffineMapSet<
    AffineMap<2, 1>,
    AffineMap<3, 0>,
    AffineMap<3, 2>,
    AffineMap<3, 7>
    > standard_map_set;

void test_map_set_apply() {
    _assert(standard_map_set.get_coeffs().size() == 4);

    auto a = standard_map_set.apply_once(5);

    _assert(a[0] == 11);
    _assert(a[1] == 15);
    _assert(a[2] == 17);
    _assert(a[3] == 22);

    std::cout << "All tests passed." << std::endl;
}

const std::vector<TestCase> test_cases = {
    { "LinearMapSet::apply", test_map_set_apply }
};

int main(int argc, char** argv) {
    current_test_case = ("undefined");

    auto regex_opts = std::regex::icase;
    std::regex regex = std::regex{(argc >= 2) ? argv[0] : "", regex_opts};

    int cases_called = 0, total_cases = 0;
    for (auto& case_ : test_cases) {
        total_cases++;
        if (std::regex_search(case_.name, regex)) {
            cases_called++;

            case_.run();
        }
    }

    std::cout << "Completed " << cases_called << "/" << total_cases << " cases." << std::endl;
}
