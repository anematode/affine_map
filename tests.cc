#include "linear_map_def.h"
#include <cassert>
#include <iostream>

using namespace AffineMap;

AffineMapSet<
    AffineMap<2, 1>,
    AffineMap<3, 0>,
    AffineMap<3, 2>,
    AffineMap<3, 7>
    > map_set;

int main() {
    assert(map_set.get_coeffs().size() == 4);
}
