#include <iostream>
#include <immintrin.h>
#include "linear_map_def.h"

using namespace AffineMap;

LinearMapSet<
	LinearMap<2, 1>,
	LinearMap<3, 0>,
	LinearMap<3, 2>,
	LinearMap<3, 7>
	> maps;

int main() {
	int c = 0;
	std::cout << c << '\n';
}
