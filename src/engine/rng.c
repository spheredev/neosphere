#include "minisphere.h"
#include "rng.h"

#include "api.h"
#include "mt19937ar.h"

static duk_ret_t js_RNG_seed    (duk_context* ctx);
static duk_ret_t js_RNG_chance  (duk_context* ctx);
static duk_ret_t js_RNG_normal  (duk_context* ctx);
static duk_ret_t js_RNG_random  (duk_context* ctx);
static duk_ret_t js_RNG_range   (duk_context* ctx);
static duk_ret_t js_RNG_sample  (duk_context* ctx);
static duk_ret_t js_RNG_string  (duk_context* ctx);
static duk_ret_t js_RNG_uniform (duk_context* ctx);

static const char* const RNG_STRING_CORPUS =
	"0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

static long s_corpus_size;

void
initialize_rng(void)
{
	unsigned long seed;
	
	console_log(1, "initializing random number generator");
	
	seed = (unsigned long)time(NULL);
	console_log(2, "    initial seed: %lu", seed);
	init_genrand(seed);

	s_corpus_size = (long)strlen(RNG_STRING_CORPUS);
}

void
seed_rng(unsigned long seed)
{
	console_log(2, "seeding random number generator");
	console_log(2, "    seed value: %lu", seed);
	
	init_genrand(seed);
}

bool
rng_chance(double odds)
{
	return odds > genrand_real2();
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
			u = 2.0 * genrand_res53() - 1.0;
			v = 2.0 * genrand_res53() - 1.0;
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
	return genrand_res53();
}

long
rng_ranged(long lower, long upper)
{
	long range = abs(upper - lower) + 1;
	return (lower < upper ? lower : upper)
		+ genrand_int31() % range;
}

const char*
rng_string(int length)
{
	static char s_name[256];

	long index;
	
	int i;

	for (i = 0; i < length; ++i) {
		index = rng_ranged(0, s_corpus_size - 1);
		s_name[i] = RNG_STRING_CORPUS[index];
	}
	s_name[length - 1] = '\0';
	return s_name;
}

double
rng_uniform(double mean, double variance)
{
	double error;
	
	error = variance * 2 * (0.5 - genrand_real2());
	return mean + error;
}

void
init_rng_api(void)
{
	api_register_function(g_duk, "RNG", "seed", js_RNG_seed);
	api_register_function(g_duk, "RNG", "chance", js_RNG_chance);
	api_register_function(g_duk, "RNG", "normal", js_RNG_normal);
	api_register_function(g_duk, "RNG", "random", js_RNG_random);
	api_register_function(g_duk, "RNG", "range", js_RNG_range);
	api_register_function(g_duk, "RNG", "sample", js_RNG_sample);
	api_register_function(g_duk, "RNG", "string", js_RNG_string);
	api_register_function(g_duk, "RNG", "uniform", js_RNG_uniform);
}

static duk_ret_t
js_RNG_seed(duk_context* ctx)
{
	unsigned long new_seed;
	
	new_seed = duk_require_number(ctx, 0);
	seed_rng(new_seed);
	return 0;
}

static duk_ret_t
js_RNG_chance(duk_context* ctx)
{
	double odds;
	
	odds = duk_require_number(ctx, 0);
	duk_push_boolean(ctx, rng_chance(odds));
	return 1;
}

static duk_ret_t
js_RNG_normal(duk_context* ctx)
{
	double mean;
	double sigma;

	mean = duk_require_number(ctx, 0);
	sigma = duk_require_number(ctx, 1);
	duk_push_number(ctx, rng_normal(mean, sigma));
	return 1;
}

static duk_ret_t
js_RNG_random(duk_context* ctx)
{
	duk_push_number(ctx, rng_random());
	return 1;
}

static duk_ret_t
js_RNG_range(duk_context* ctx)
{
	long lower;
	long upper;

	lower = duk_require_number(ctx, 0);
	upper = duk_require_number(ctx, 1);
	duk_push_number(ctx, rng_ranged(lower, upper));
	return 1;
}

static duk_ret_t
js_RNG_sample(duk_context* ctx)
{
	duk_uarridx_t index;
	long          length;

	duk_require_object_coercible(ctx, 0);
	length = (long)duk_get_length(ctx, 0);
	index = rng_ranged(0, length - 1);
	duk_get_prop_index(ctx, 0, index);
	return 1;
}

static duk_ret_t
js_RNG_string(duk_context* ctx)
{
	int num_args;
	int length;

	num_args = duk_get_top(ctx);
	length = num_args >= 1 ? duk_require_number(ctx, 0)
		: 10;
	if (length < 1 || length > 255)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "RNG.string(): length must be [1-255] (got: %d)", length);
	
	duk_push_string(ctx, rng_string(length));
	return 1;
}

static duk_ret_t
js_RNG_uniform(duk_context* ctx)
{
	double mean;
	double variance;

	mean = duk_require_number(ctx, 0);
	variance = duk_require_number(ctx, 1);
	duk_push_number(ctx, rng_uniform(mean, variance));
	return 1;
}
