#include "map_def.h"
#include <cassert>
#include <iostream>

using namespace Affine;

AffineMapSet<
    AffineMap<2, 1>,
    AffineMap<3, 0>,
    AffineMap<3, 2>,
    AffineMap<3, 7>
    > map_set;

int main() {
    assert(map_set.get_coeffs().size() == 4);

    auto a = map_set.apply_once(5);

    assert(a[0] == 11);
    assert(a[1] == 15);
    assert(a[2] == 17);
    assert(a[3] == 22);
}
