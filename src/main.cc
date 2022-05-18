#include <iostream>
#include <immintrin.h>
#include "affine.h"

using namespace Affine;

AffineMapSet<
	AffineMap<2, 1>,
	AffineMap<3, 0>,
	AffineMap<3, 2>,
	AffineMap<3, 7>
	> maps;

int main() {
	auto iterate_map = StandardIterateMap<maps>();

	iterate_map.set_initial({ 1 });

	iterate_map.compute_till({ .max = 1'000'000'00 });
	iterate_map.for_each_solution([&] (int64_t k, bool r) {
			if (k % 4 == 3 && !r) {
				std::cout << k << '\n';
				}
				});


	std::cout << iterate_map.maps().apply_once(1)[0] << '\n';
}
