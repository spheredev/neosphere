#include "minisphere.h"
#include "profiler.h"

#include "jsal.h"

struct record
{
	js_ref_t* function;
	int       num_hits;
	char*     name;
	double    total_cost;
};

static bool js_instrumentedWrapper (int num_args, bool is_ctor, int magic);

bool      s_initialized = false;
vector_t* s_records;
double    s_startup_time;

void
profiler_init(void)
{
	s_records = vector_new(sizeof(struct record));
	s_initialized = true;

	s_startup_time = al_get_time();
}

void
profiler_uninit(void)
{
	struct record* record;
	struct record  record_obj;
	double         runtime;
	
	iter_t iter;

	runtime = al_get_time() - s_startup_time;
	
	printf("Profiling Results for '%s' (ran for ~%g seconds)\n", game_name(g_game), round(runtime));
	
	record_obj.name = strdup("[in Sphere event loop]");
	record_obj.num_hits = g_tick_count;
	record_obj.total_cost = g_lost_time;
	record_obj.function = NULL;
	vector_push(s_records, &record_obj);
	
	iter = vector_enum(s_records);
	while (record = iter_next(&iter)) {
		jsal_unref(record->function);
		printf("  %s - spent %g us (%g%%) - avg. %g us\n",
			record->name,
			round(record->total_cost * 1.0e6),
			round(record->total_cost / runtime * 1000.0) / 10.0,
			round(record->total_cost / record->num_hits * 1.0e6));
		free(record->name);
	}
	vector_free(s_records);
}

bool
profiler_enabled(void)
{
	return s_initialized;
}

js_ref_t*
profiler_attach_to(js_ref_t* function, const char* description)
{
	int           index;
	struct record record_obj;
	js_ref_t*     shim_ref;

	record_obj.name = strdup(description);
	record_obj.num_hits = 0;
	record_obj.total_cost = 0.0;
	record_obj.function = function;
	vector_push(s_records, &record_obj);
	
	index = vector_len(s_records) - 1;
	jsal_push_new_function(js_instrumentedWrapper, "", 0, index);
	shim_ref = jsal_ref(-1);
	jsal_pop(1);
	return shim_ref;
}

static bool
js_instrumentedWrapper(int num_args, bool is_ctor, int magic)
{
	double         end_time;
	struct record* record;
	double         start_time;

	record = vector_get(s_records, magic);

	jsal_push_ref_weak(record->function);
	jsal_push_this();
	jsal_insert(0);
	jsal_insert(0);
	start_time = al_get_time();
	jsal_call_method(num_args);
	end_time = al_get_time();
	record->total_cost += end_time - start_time;
	++record->num_hits;
	return true;
}
