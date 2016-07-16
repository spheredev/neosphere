#include "minisphere.h"
#include "rng.h"

#include "xoroshiro.h"

static const char* const RNG_STRING_CORPUS =
	"0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

static long    s_corpus_size;
static xoro_t* s_xoro;

void
initialize_rng(void)
{
	uint64_t seed;
	
	console_log(1, "initializing RNG subsystem");
	
	seed = time(NULL);
	s_xoro = xoro_new(seed);
	console_log(2, "    seed: 0x%"PRIx64, seed);

	s_corpus_size = (long)strlen(RNG_STRING_CORPUS);
}

void
shutdown_rng(void)
{
	xoro_free(s_xoro);
}

void
seed_rng(uint64_t seed)
{
	console_log(2, "reseeding random number generator");
	console_log(2, "    new seed: 0x%"PRIx64, seed);
	
	xoro_reseed(s_xoro, seed);
}

void
rng_get_state(char* buffer)
{
	xoro_get_state(s_xoro, buffer);
}

bool
rng_set_state(const char* state)
{
	return xoro_set_state(s_xoro, state);
}

bool
rng_chance(double odds)
{
	return odds > xoro_gen_double(s_xoro);
}

double
rng_normal(double mean, double sigma)
{
	static bool   s_have_y = false;
	static double s_y;

	double u, v, w;
	double x;

	if (!s_have_y) {
		do {
			u = 2.0 * xoro_gen_double(s_xoro) - 1.0;
			v = 2.0 * xoro_gen_double(s_xoro) - 1.0;
			w = u * u + v * v;
		} while (w >= 1.0);
		w = sqrt(-2 * log(w) / w);
		x = u * w;
		s_y = v * w;
		s_have_y = true;
	}
	else {
		x = s_y;
		s_have_y = false;
	}
	return mean + x * sigma;
}

double
rng_random(void)
{
	return xoro_gen_double(s_xoro);
}

int
rng_int(int lower, int upper)
{
	int range = abs(upper - lower) + 1;
	return (lower < upper ? lower : upper)
		+ xoro_gen_uint(s_xoro) % range;
}

const char*
rng_string(int length)
{
	static char s_name[256];

	long index;
	
	int i;

	for (i = 0; i < length; ++i) {
		index = rng_int(0, s_corpus_size - 1);
		s_name[i] = RNG_STRING_CORPUS[index];
	}
	s_name[length - 1] = '\0';
	return s_name;
}

double
rng_uniform(double mean, double variance)
{
	double error;
	
	error = variance * 2 * (0.5 - xoro_gen_double(s_xoro));
	return mean + error;
}
