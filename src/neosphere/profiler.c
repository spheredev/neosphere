/**
 *  Sphere: the JavaScript game platform
 *  Copyright (c) 2015-2024, Fat Cerberus
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 *  * Neither the name of Spherical nor the names of its contributors may be
 *    used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
**/

#include "neosphere.h"
#include "profiler.h"

#include "jsal.h"
#include "table.h"

#define TIME_PRECISION 1.0e6    // microseconds
#define UNIT_NAME      "us"

struct record
{
	double    average_cost;
	js_ref_t* function;
	int       num_hits;
	char*     name;
	double    total_cost;
};

static bool js_instrumentedWrapper (int num_args, bool is_ctor, intptr_t magic);

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

	record_obj.name = strdup("[Sphere event loop]");
	record_obj.num_hits = g_tick_count;
	record_obj.total_cost = g_idle_time;
	record_obj.function = NULL;
	vector_push(s_records, &record_obj);

	print_results(runtime);

	iter = vector_enum(s_records);
	while ((record = iter_next(&iter))) {
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
	jsal_push_new_function(js_instrumentedWrapper, "", 0, false, index);
	shim_ref = jsal_pop_ref();
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
	while ((record = iter_next(&iter))) {
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
	table = table_new(heading, true);
	table_add_column(table, "event");
	table_add_column(table, "count");
	table_add_column(table, "time (%s)", UNIT_NAME);
	table_add_column(table, "%% run");
	table_add_column(table, "avg (%s)", UNIT_NAME);
	table_add_column(table, "%% avg");
	table_add_column(table, "ops/f");
	iter = vector_enum(s_records);
	while ((record = iter_next(&iter))) {
		if (record->num_hits <= 0)
			continue;
		table_add_text(table, 0, record->name);
		table_add_number(table, 1, record->num_hits);
		table_add_number(table, 2, record->total_cost * TIME_PRECISION);
		table_add_percentage(table, 3, record->total_cost / running_time);
		table_add_number(table, 4, record->average_cost * TIME_PRECISION);
		table_add_percentage(table, 5, record->average_cost / total_average);
		table_add_number(table, 6, floor(1.0 / record->average_cost / 60));
	}
	table_add_text(table, 0, "TOTAL");
	table_add_number(table, 1, total_hits);
	table_add_number(table, 2, total_time * TIME_PRECISION);
	table_add_percentage(table, 3, total_time / running_time);
	table_add_number(table, 4, total_average * TIME_PRECISION);
	table_add_percentage(table, 5, 1.0);
	table_add_text(table, 6, "n/a");
	table_print(table);
	table_free(table);
	free(heading);
}

static bool
js_instrumentedWrapper(int num_args, bool is_ctor, intptr_t magic)
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
