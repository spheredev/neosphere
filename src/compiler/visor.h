#ifndef CELL__VISOR_H__INCLUDED
#define CELL__VISOR_H__INCLUDED

typedef struct visor visor_t;

visor_t* visor_new        (void);
void     visor_free       (visor_t* visor);
int      visor_num_errors (const visor_t* visor);
int      visor_num_warns  (const visor_t* visor);
void     visor_begin_op   (visor_t* visor, const char* fmt, ...);
void     visor_end_op     (visor_t* visor);
void     visor_error      (visor_t* visor, const char* fmt, ...);
void     visor_info       (visor_t* visor, const char* fmt, ...);
void     visor_warn       (visor_t* visor, const char* fmt, ...);

#endif // CELL__VISOR_H__INCLUDED
