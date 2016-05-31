// xoroshiro128+ by David Blackman and Sebastiano Vigna (vigna@acm.org)
// SplitMix64 by Sebastiano Vigna (vigna@acm.org)

// to the extent possible under law, the author has dedicated all copyright
// and related and neighboring rights to this software to the public domain
// worldwide. this software is distributed without any warranty.
//
//     http://creativecommons.org/publicdomain/zero/1.0/

#include "xoroshiro.h"

#include <stdlib.h>
#include <stdint.h>

#define ROTL64(x, n) ((x) << (n) | (x) >> (64 - (n)))

struct xoro
{
	unsigned int refcount;
	uint64_t     s[2];
};

xoro_t*
xoro_new(uint64_t seed)
{
	xoro_t* xoro;

	xoro = calloc(1, sizeof(xoro_t));
	xoro_reseed(xoro, seed);
	return xoro_ref(xoro);
}

xoro_t*
xoro_ref(xoro_t* xoro)
{
	++xoro->refcount;
	return xoro;
}

void
xoro_free(xoro_t* xoro)
{
	if (--xoro->refcount > 0)
		return;
	free(xoro);
}

double
xoro_gen_double(xoro_t* xoro)
{
	union { uint64_t i; double d; } u;
	uint64_t                        x;

	x = xoro_gen_int(xoro);
	u.i = 0x3ffUi64 << 52 | x >> 12;
	return u.d - 1.0;

}

uint64_t
xoro_gen_int(xoro_t* xoro)
{
	uint64_t s0;
	uint64_t s1;
	uint64_t result;

	s0 = xoro->s[0];
	s1 = xoro->s[1];
	result = s0 + s1;

	s1 ^= s0;
	xoro->s[0] = ROTL64(s0, 55) ^ s1 ^ (s1 << 14);
	xoro->s[1] = ROTL64(s1, 36);

	return result;
}

void
xoro_jump(xoro_t* xoro)
{
	static const uint64_t JUMP[] =
	{
		0xbeac0467eba5facb,
		0xd86b048b86aa9922
	};

	uint64_t s0 = 0;
	uint64_t s1 = 0;

	int i, b;

	for (i = 0; i < sizeof JUMP / sizeof *JUMP; ++i) {
		for (b = 0; b < 64; ++b) {
			if (JUMP[i] & 1Ui64 << b) {
				s0 ^= xoro->s[0];
				s1 ^= xoro->s[1];
			}
			xoro_gen_int(xoro);
		}
	}

	xoro->s[0] = s0;
	xoro->s[1] = s1;
}

void
xoro_reseed(xoro_t* xoro, uint64_t seed)
{
	uint64_t z;

	int i;

	// seed xoroshiro128+ using SplitMix64
	for (i = 0; i < 2; ++i) {
		z = (seed += 0x9E3779B97F4A7C15Ui64);
		z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9Ui64;
		z = (z ^ (z >> 27)) * 0x94D049BB133111EBUi64;
		xoro->s[i] = z ^ (z >> 31);
	}
}
