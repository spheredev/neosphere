#include "minisphere.h"
#include "api.h"

#include "rng.h"

static duk_ret_t js_RNG_seed   (duk_context* ctx);
static duk_ret_t js_RNG_chance (duk_context* ctx);
static duk_ret_t js_RNG_normal (duk_context* ctx);
static duk_ret_t js_RNG_range  (duk_context* ctx);
static duk_ret_t js_RNG_sample (duk_context* ctx);
static duk_ret_t js_RNG_vary   (duk_context* ctx);

void
initialize_rng(void)
{
	console_log(1, "Seeding random number generator\n");

	init_genrand(time(NULL));
}

void
seed_rng(unsigned long seed)
{
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

	int u, v, w;
	int x;

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

long
rng_ranged(long lower, long upper)
{
	long range = abs(upper - lower) + 1;
	return (lower < upper ? lower : upper)
		+ genrand_int31() % range;
}

double
rng_uniform(double mean, double variance)
{
	double error = variance * 2 * (0.5 - genrand_real2());
	return mean + error;
}

void
init_rng_api(void)
{
	duk_push_global_object(g_duk);
	duk_push_object(g_duk);
	duk_push_c_function(g_duk, js_RNG_seed, DUK_VARARGS);
	duk_put_prop_string(g_duk, -2, "seed");
	duk_push_c_function(g_duk, js_RNG_chance, DUK_VARARGS);
	duk_put_prop_string(g_duk, -2, "chance");
	duk_push_c_function(g_duk, js_RNG_normal, DUK_VARARGS);
	duk_put_prop_string(g_duk, -2, "normal");
	duk_push_c_function(g_duk, js_RNG_range, DUK_VARARGS);
	duk_put_prop_string(g_duk, -2, "range");
	duk_push_c_function(g_duk, js_RNG_sample, DUK_VARARGS);
	duk_put_prop_string(g_duk, -2, "sample");
	duk_push_c_function(g_duk, js_RNG_vary, DUK_VARARGS);
	duk_put_prop_string(g_duk, -2, "vary");
	duk_put_prop_string(g_duk, -2, "RNG");
	duk_pop(g_duk);
}

static duk_ret_t
js_RNG_seed(duk_context* ctx)
{
	unsigned long seed = duk_require_number(ctx, 0);

	seed_rng(seed);
	return 0;
}

static duk_ret_t
js_RNG_chance(duk_context* ctx)
{
	double odds = duk_require_number(ctx, 0);

	duk_push_boolean(ctx, rng_chance(odds));
	return 1;
}

static duk_ret_t
js_RNG_normal(duk_context* ctx)
{
	double mean = duk_require_number(ctx, 0);
	double sigma = duk_require_number(ctx, 1);

	duk_push_number(ctx, rng_normal(mean, sigma));
	return 1;
}

static duk_ret_t
js_RNG_range(duk_context* ctx)
{
	long lower = duk_require_number(ctx, 0);
	long upper = duk_require_number(ctx, 1);

	duk_push_number(ctx, rng_ranged(lower, upper));
	return 1;
}

static duk_ret_t
js_RNG_sample(duk_context* ctx)
{
	duk_require_object_coercible(ctx, 0);

	duk_uarridx_t index;
	long          length;

	length = (long)duk_get_length(ctx, 0);
	index = rng_ranged(0, length - 1);
	duk_get_prop_index(ctx, 0, index);
	return 1;
}

static duk_ret_t
js_RNG_vary(duk_context* ctx)
{
	double mean = duk_require_number(ctx, 0);
	double variance = duk_require_number(ctx, 1);

	duk_push_number(ctx, rng_uniform(mean, variance));
	return 1;
}
