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
	int c = 0;
	std::cout << c << '\n';
}
