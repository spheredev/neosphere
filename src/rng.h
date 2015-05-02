#ifndef MINISPHERE__RNG_H__INCLUDED
#define MINISPHERE__RNG_H__INCLUDED

extern void   initialize_rng (void);
extern void   seed_rng       (unsigned long seed);
extern bool   rng_chance     (double odds);
extern double rng_normal     (double mean, double sigma);
extern long   rng_ranged     (long lower, long upper);
extern double rng_uniform    (double mean, double variance);

extern void init_rng_api (void);

#endif // MINISPHERE__RNG_H__INCLUDED
