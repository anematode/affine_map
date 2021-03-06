// g++ mse.cc -o mse -O3 -march=native -std=c++14 -fno-strict-aliasing

#include <iostream>
#include <bitset>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <immintrin.h>
#include <fstream>
#include <atomic>
#include <thread>
#include <vector>

#ifndef __AVX2__
#error "AVX not supported (please define -march=native if your CPU does support it)"
#endif

#ifndef __BMI2__
#error "BMI2 not supported (please define -march=native if your CPU does support it)"
#endif

#define SHOW_PROGRESS 1
#define GET_HISTORY 1

constexpr uint64_t _SEARCH_MAX = 2'000'000'000ULL;
constexpr uint64_t PROGRESS_EVERY = 1'000'000'000ULL;


constexpr uint64_t SEARCH_MAX = (_SEARCH_MAX / 1536 + 1) * 1536;

char* reached;

// Every four consecutive bits tells us how a certain number can be reached
char* path;

// Initialize the first 1536 elements straightforwardly
void init_1536() {
	auto b = reinterpret_cast<std::bitset<SEARCH_MAX>*>(reached);
	auto p = reinterpret_cast<std::bitset<SEARCH_MAX * 4>*>(path);

	// b->operator[](1) = true;

#define lol do { b->operator[](i) = true; p->operator[](4 * i + idx) = true; } while (0)
	for (uint64_t i = 2; i <= 1536; ++i) { 
		int idx = 0;
		if (i % 2 == 1 && b->operator[]((i - 1) / 2)) lol;
		
		idx++;
		if (i % 3 == 0 && b->operator[](i / 3)) lol;
		
		idx++;
		if (i % 3 == 2 && b->operator[]((i - 2) / 3)) lol;

		idx++;	
		if (i > 7 && i % 3 == 1 && b->operator[]((i - 7) / 3)) lol;	
	}
}


// Every three bits toggled, in one of three locations
#define BIT_MASK_1 0x9249249249249249ULL
#define BIT_MASK_2 (BIT_MASK_1 >> 1)
#define BIT_MASK_3 (BIT_MASK_1 >> 2)

// Vectorized computation of the next elements, between start and end (multiples of 1536)
void init_rest(uint64_t start = 1536, uint64_t end = SEARCH_MAX) {
	if (start >= SEARCH_MAX) start = SEARCH_MAX - 1536;
	if (end > SEARCH_MAX) end = SEARCH_MAX;

	uint64_t* _reached = reinterpret_cast<uint64_t*>(reached);

	__m128i read2, scale2p1, scale2p2;
	uint64_t scale2p12, scale2p21, scale2p11, scale2p22 = 0;
	uint64_t scale3p03, scale3p10, scale3p11,
		 scale3p12 = _pdep_u64(_reached[start / (3*64) - 1], BIT_MASK_3), scale2p02;
	uint64_t store1, store2, store3, store4, store5, store6; // For actual writing to the reached memory
	// uint64_t mask1, mask2, mask3, mask4;  // Masks for 2*x, 3*x, 3*x+2, 3*x+4, respectively

	for (uint64_t _i = start; _i < end; _i += 768 /* 64 * 12 */) { // i is index in bits
		uint64_t write_i = _i / 64;  // writing to this index in uint64_t
		uint64_t read2s = write_i / 2; // reading from here and spreading by 2 gives 2*x
		uint64_t read3s = write_i / 3 - 1; // read here and spread by 3 to get 3*x

#define compute_scale2_parts(addr) \
		/* 2x+1 */ \
		read2 = _mm_load_si128(reinterpret_cast<const __m128i*>(_reached + addr)); \
		/* Upper and lower halves of the result (2*x). throughput 1, latency 6, easily hidden */ \
		scale2p1 = _mm_clmulepi64_si128(read2, read2, 0x0); \
		scale2p2 = _mm_clmulepi64_si128(read2, read2, 0x11);

#define extract_scale2 \
		scale2p02 = scale2p22; \
		scale2p11 = _mm_extract_epi64(scale2p1, 0); \
		scale2p12 = _mm_extract_epi64(scale2p1, 1); \
		scale2p21 = _mm_extract_epi64(scale2p2, 0); \
		scale2p22 = _mm_extract_epi64(scale2p2, 1); \

		compute_scale2_parts(read2s);

		// We wish to extract 3x, 3x+2, and 3x+7. We do so with pdeps and shifts, first extracting 3x,
		// then performing some shifts

		// 3x+a
		uint64_t scale3p1 = _reached[read3s + 1];
		uint64_t scale3p2 = _reached[read3s + 2];
		uint64_t scale3p3 = _reached[read3s + 3];
		uint64_t scale3p4 = _reached[read3s + 4];

#define extract_scale3(src) \
		scale3p03 = scale3p12; \
		scale3p10 = _pdep_u64(src, BIT_MASK_1); \
		scale3p11 = _pdep_u64(src >> 22, BIT_MASK_2); \
		scale3p12 = _pdep_u64(src >> 43, BIT_MASK_3); 

#define compute_scale3_parts(store1, store2, store3) \
		store1 = (scale3p03 >> 57) | (scale3p10 << 2) | scale3p10 | (scale3p10 << 7) | (scale3p03 >> 62); \
		store2 = ((scale3p10 >> 57) | (scale3p11 << 2) | scale3p11 | (scale3p11 << 7)) | (scale3p10 >> 62); \
		store3 = ((scale3p11 >> 57) | (scale3p12 << 2) | scale3p12 | (scale3p12 << 7)) | (scale3p11 >> 62); 	

		extract_scale3(scale3p1);

		extract_scale2;

		compute_scale3_parts(store1, store2, store3);

		extract_scale3(scale3p2);

		compute_scale3_parts(store4, store5, store6);

#define merge_parts(store1, store2, store3, store4) \
		store1 |= (scale2p11 << 1) | (scale2p02 >> 63); \
		store2 |= (scale2p12 << 1) | (scale2p11 >> 63); \
		store3 |= (scale2p21 << 1) | (scale2p12 >> 63); \
		store4 |= (scale2p22 << 1) | (scale2p21 >> 63);

#define write_data(offset, store1, store2, store3, store4) \
		/* Write accumulated data */ \
		*(_reached + write_i + offset) = store1; \
		*(_reached + write_i + offset + 1) = store2; \
		*(_reached + write_i + offset + 2) = store3; \
		*(_reached + write_i + offset + 3) = store4;

		merge_parts(store1, store2, store3, store4);

		write_data(0, store1, store2, store3, store4);

		compute_scale2_parts(read2s + 2);

		extract_scale3(scale3p3);

		extract_scale2;

		compute_scale3_parts(store1, store2, store3);

		merge_parts(store5, store6, store1, store2);

		write_data(4, store5, store6, store1, store2);

		extract_scale3(scale3p4);

		compute_scale2_parts(read2s + 4);

		compute_scale3_parts(store4, store5, store6);

		extract_scale2;

		merge_parts(store3, store4, store5, store6);

		write_data(8, store3, store4, store5, store6);

		if (SHOW_PROGRESS && _i % (768 * (PROGRESS_EVERY / 768)) == 0) {
			printf("Percentage complete: %f%%\n", (double)_i / (SEARCH_MAX / 100));
		}
	}
}

// Checksum 100000: 15063046391347018756
uint64_t checksum(uint64_t lim) {
	uint64_t s = 1;

	if (lim >= SEARCH_MAX) {
		throw std::runtime_error("Limit too high");
	}

	uint64_t* start = reinterpret_cast<uint64_t*>(reached);
	for (uint64_t* i = start; i < start + lim / 64; ++i) {
		s *= *i;
		s += 2;
	}

	return s;
}



template <typename L>
void for_each_entry(L l) {
	// (i, reached) -> void

	for (uint64_t i = 1; i < SEARCH_MAX; ++i) {
		l(i, reached[i]);
	}
}

struct timespec start, finish;
void clock_start() {
	clock_gettime(CLOCK_MONOTONIC, &start);
}

void clock_end(const char* msg) {
	clock_gettime(CLOCK_MONOTONIC, &finish);

	printf("Timer '%s' took %f s\n", msg, (double)(finish.tv_sec - start.tv_sec) + 
			(finish.tv_nsec - start.tv_nsec) / 1000000000.0);
}

void count_solutions(uint64_t begin=0, uint64_t end=SEARCH_MAX) {
	if (begin % 1536 != 0 || end % 1536 != 0) {
		throw std::runtime_error("Invalid congruence range");
	}
	// To count congruences, we iterate over 256 bits at a time, mask out the congruences,
	// and perform a population count on the resultant vectors. AVX2 lacks a popcnt instruction,
	// so we need to do some shuffling. Code mostly thanks to Wojciech Mula.
	
	const __m256i lookup = _mm256_setr_epi8(
        /* 0 */ 0, /* 1 */ 1, /* 2 */ 1, /* 3 */ 2,
        /* 4 */ 1, /* 5 */ 2, /* 6 */ 2, /* 7 */ 3,
        /* 8 */ 1, /* 9 */ 2, /* a */ 2, /* b */ 3,
        /* c */ 2, /* d */ 3, /* e */ 3, /* f */ 4,

        /* 0 */ 0, /* 1 */ 1, /* 2 */ 1, /* 3 */ 2,
        /* 4 */ 1, /* 5 */ 2, /* 6 */ 2, /* 7 */ 3,
        /* 8 */ 1, /* 9 */ 2, /* a */ 2, /* b */ 3,
        /* c */ 2, /* d */ 3, /* e */ 3, /* f */ 4
    	);

    	const __m256i low_mask = _mm256_set1_epi8(0x0f);

   	__m256i acc = _mm256_setzero_si256();

	__m256i lo, hi, popcnt1, popcnt2;

	const char* _reached = reinterpret_cast<char*>(reached);

#define PERFORM_ITER { \
	__m256i data = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(_reached + i)); \
	const __m256i lo  = _mm256_and_si256(data, low_mask); \
        const __m256i hi  = _mm256_and_si256(_mm256_srli_epi16(data, 4), low_mask); \
        const __m256i popcnt1 = _mm256_shuffle_epi8(lookup, lo); \
        const __m256i popcnt2 = _mm256_shuffle_epi8(lookup, hi); \
        local = _mm256_add_epi8(local, popcnt1); \
        local = _mm256_add_epi8(local, popcnt2); \
	i += 32; \
}

	for (size_t i = 0; i < SEARCH_MAX / 8; ) {
       		__m256i local = _mm256_setzero_si256();
		PERFORM_ITER PERFORM_ITER PERFORM_ITER PERFORM_ITER
		PERFORM_ITER PERFORM_ITER PERFORM_ITER PERFORM_ITER
		acc = _mm256_add_epi64(acc, _mm256_sad_epu8(local, _mm256_setzero_si256())); // accumulate
	}
	
   	uint64_t result = 0;

    	result += static_cast<uint64_t>(_mm256_extract_epi64(acc, 0));
    	result += static_cast<uint64_t>(_mm256_extract_epi64(acc, 1));
    	result += static_cast<uint64_t>(_mm256_extract_epi64(acc, 2));
    	result += static_cast<uint64_t>(_mm256_extract_epi64(acc, 3));

	std::cout << "Counted " << result << " solutions out of " << SEARCH_MAX << "\n";
}

#define USE_THREADS_THRESHOLD 400'000'000 /* bits */
#define NUM_THREADS 4

void wrap_init_rest(uint64_t start, uint64_t end) {
#if !USE_THREADS
	init_rest(start, end);
#else
	if (end - start < USE_THREADS_THRESHOLD) {
		init_rest(start, end);
	} else {
		// Spawn some threads to divvy up the work
		uint64_t s_d = start / 1536;
		uint64_t e_d = end / 1536;

		std::vector<std::thread> threads;

		for (int i = 0; i < NUM_THREADS; ++i) {
			uint64_t t_start = (s_d + (e_d - s_d) * i / NUM_THREADS) * 1536;
		       	uint64_t t_end = (s_d + (e_d - s_d) * (i + 1) / NUM_THREADS) * 1536;

			std::thread th{init_rest, t_start, t_end};
			threads.push_back(std::move(th));
		}

		for (auto& th : threads) {
			th.join();
		}

	}
#endif // USE_THREADS
}

void write_unreachable() {
	std::ofstream dat("./unreachable.dat", std::ios::out | std::ios::binary);

	if (!dat) {
		throw std::runtime_error("Failed to open file");
	}

	const char* HEADER = "UNREACHS";

	dat.write(HEADER, strlen(HEADER));
	printf("Writing solutions to file");
	clock_start();

	uint64_t* sus = reinterpret_cast<uint64_t*>(reached);
	for (size_t k = 0; k < SEARCH_MAX / 64; ++k) {
		uint64_t bitset = ~sus[k];  // only write UNREACHABLE solutions, hence bitwise not

		while (bitset != 0) {
			uint64_t t = bitset & -bitset;
			int zeros = __builtin_ctzll(bitset);
			uint64_t c = k * 64 + zeros;
			dat.write(reinterpret_cast<const char*>(&c), sizeof(uint64_t));
			bitset ^= t;
		}
	}

	clock_end("disk write");
}


void large_compute() {
	std::ios_base::sync_with_stdio(false);
	if (SEARCH_MAX % 1536 != 0) {
		throw std::runtime_error("SEARCH_MAX must be a multiple of 1536");
	}

	reached = (char*)malloc(SEARCH_MAX / 8);
#ifdef GET_HISTORY
	path = (char*)malloc(SEARCH_MAX / 2);
#endif

	printf("Forcing page allocation\n");
	// Force page allocation
	memset((void*)reached, 0, SEARCH_MAX / 8);
	memset((void*)path, 0, SEARCH_MAX / 2);
	printf("Allocated %llu bytes of scratch memory\n", SEARCH_MAX / 8);

	init_1536(); // init first results

	clock_start();
	
	// Split up the process into [1536 * 2^n, 1536 * 2^(n+1)] for various n
	
	uint64_t n = 1;

	do {
		wrap_init_rest(1536 * n, 1536 * 2 * n);
		n <<= 1;
	} while (n < SEARCH_MAX / 1536);

	clock_end("large computation");

	printf("%llu entries computed\n", SEARCH_MAX);
	clock_start();
	count_solutions();
	clock_end("count solutions");

	//write_unreachable();
}

int main() {
	large_compute();
	std::cout << checksum(100000) <<'\n';
}
