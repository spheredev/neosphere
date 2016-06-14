#ifndef MINISPHERE__RNG_H__INCLUDED
#define MINISPHERE__RNG_H__INCLUDED

void        initialize_rng (void);
void        shutdown_rng   (void);
void        seed_rng       (uint64_t seed);
bool        rng_chance     (double odds);
double      rng_normal     (double mean, double sigma);
double      rng_random     (void);
int         rng_int     (int lower, int upper);
const char* rng_string     (int length);
double      rng_uniform    (double mean, double variance);

#endif // MINISPHERE__RNG_H__INCLUDED
