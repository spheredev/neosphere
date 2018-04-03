#include "minisphere.h"
#include "profiler.h"

#include "jsal.h"
#include "table.h"

struct record
{
	double    average_cost;
	js_ref_t* function;
	int       num_hits;
	char*     name;
	double    total_cost;
};

static bool js_instrumentedWrapper (int num_args, bool is_ctor, int magic);

static int  order_records (const void* a_ptr, const void* b_ptr);
static void print_results (double running_time);

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
	
	record_obj.name = strdup("[event loop - idle]");
	record_obj.num_hits = g_tick_count;
	record_obj.total_cost = g_idle_time;
	record_obj.function = NULL;
	vector_push(s_records, &record_obj);
	
	print_results(runtime);
	
	iter = vector_enum(s_records);
	while (record = iter_next(&iter)) {
		jsal_unref(record->function);
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

static int
order_records(const void* a_ptr, const void* b_ptr)
{
	const struct record* a;
	const struct record* b;

	a = a_ptr;
	b = b_ptr;
	return b->total_cost > a->total_cost ? 1
		: b->total_cost < a->total_cost ? -1
		: 0;
}

static void
print_results(double running_time)
{
	char*          heading;
	table_t*       table;
	double         total_average = 0.0;
	int            total_hits = 0;
	double         total_time = 0.0;
	struct record* record;

	iter_t iter;

	vector_sort(s_records, order_records);
	iter = vector_enum(s_records);
	while (record = iter_next(&iter)) {
		if (record->num_hits <= 0)
			continue;
		record->average_cost = record->total_cost / record->num_hits;
		total_average += record->average_cost;
		total_hits += record->num_hits;
		total_time += record->total_cost;
	}

	printf("\n");
	
	heading = strnewf("performance report - %.1f%% LF",
		round(1000 * (1.0 - g_idle_time / running_time)) / 10);
	table = table_new(heading);
	table_add_column(table, "event");
	table_add_column(table, "count");
	table_add_column(table, "time (us)");
	table_add_column(table, "% run");
	table_add_column(table, "avg (us)");
	table_add_column(table, "% avg");
	iter = vector_enum(s_records);
	while (record = iter_next(&iter)) {
		if (record->num_hits <= 0)
			continue;
		table_add_text(table, 0, record->name);
		table_add_number(table, 1, record->num_hits);
		table_add_number(table, 2, 1.0e6 * record->total_cost);
		table_add_percentage(table, 3, record->total_cost / running_time);
		table_add_number(table, 4, 1.0e6 * record->average_cost);
		table_add_percentage(table, 5, record->average_cost / total_average);
	}
	table_add_text(table, 0, "total");
	table_add_number(table, 1, total_hits);
	table_add_number(table, 2, 1.0e6 * total_time);
	table_add_percentage(table, 3, total_time / running_time);
	table_add_number(table, 4, 1.0e6 * total_average);
	table_add_percentage(table, 5, 1.0);
	table_print(table);
	table_free(table);
	free(heading);
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
