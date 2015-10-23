/*
 * PHP Extension clockthread
 * Author: Thomas Jez
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_clockthread.h"
#include "pthread.h"
#include <stdio.h>
//#include <termios.h>
//#include <unistd.h>

struct clock_struct {
	time_t start_time;
	int x, y;
};

struct clocks_struct {
	struct clock_struct* clock;
	int anz_clocks;
};

static pthread_t clock_thread = 0;
static int line;
static int stop;

zend_class_entry *clockthread_class;

static void *clock_loop(void* _clock_array) {
	int x, y, i;
	char *key;
	uint key_len;
	int array_count;
	HashTable *arr_hash;
	HashPosition pointer;
	zval **data;
	ulong index;
	static struct clocks_struct clock_args;

	zval* clock_array = (zval*) _clock_array;
	arr_hash = Z_ARRVAL_P(clock_array);
	array_count = zend_hash_num_elements(arr_hash);
	clock_args.clock = ecalloc(array_count, sizeof(struct clock_struct));
	clock_args.anz_clocks = array_count;

	i = 0;

	for(zend_hash_internal_pointer_reset_ex(arr_hash, &pointer);
			zend_hash_get_current_data_ex(arr_hash, (void**) &data, &pointer) == SUCCESS;
			zend_hash_move_forward_ex(arr_hash, &pointer)) {

		zend_hash_get_current_key_ex(arr_hash, &key, &key_len, &index, 0, &pointer);
		clock_args.clock[i].start_time = Z_LVAL_PP(data);
		clock_args.clock[i].x = 62;
		clock_args.clock[i].y = index + 4;
		i++;
	}

	while (stop == 0) {
		time_t now;
		time(&now);
		long dauer;
		int stunden, minuten, sekunden;
		for (i = 0; i < clock_args.anz_clocks; i++) {
			dauer = difftime(now, clock_args.clock[i].start_time);
			x = clock_args.clock[i].x;
			y = clock_args.clock[i].y;
			stunden = dauer / 3600;
			minuten = (dauer % 3600) / 60;
			sekunden = (dauer % 3600) % 60;
			php_printf("\x1B[%d;%dH%02d:%02d:%02d\n", y, x, stunden, minuten, sekunden);
		}
		php_printf("\x1B[%d;%dH", line + 4, 3);
		fflush(stdout);
		sleep(1);
	}
	efree(clock_args.clock);
	return NULL;
}

/*
 * prepares the parameters for the clock thread and starts it
 */
static void run_clocks(zval* clock_array) {
	void* status;

	if (clock_thread != 0) {
		stop = 1;
		pthread_join(clock_thread, &status);
	}

	stop = 0;
	clock_thread = 0;
	line = 0;
	pthread_create(&clock_thread, NULL, clock_loop, (void*)clock_array);
}

PHP_METHOD(Clockthread, return2line) {
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &line) != SUCCESS) {
		return;
	}

	RETURN_NULL();
}

PHP_METHOD(Clockthread, stop) {
	void* status;
	stop = 1;
	pthread_join(clock_thread, &status);

	RETURN_NULL();
}

PHP_METHOD(Clockthread, start)
{
	long anz_activities;
	zval *clock_array;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "la", &anz_activities, &clock_array) != SUCCESS) {
		return;
	}
	run_clocks(clock_array);
	RETURN_LONG(clock_thread);
}

ZEND_BEGIN_ARG_INFO(arginfo_clockthread_stop, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_clockthread_return2line, 0)
ZEND_ARG_INFO(0, line)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_clockthread_start, 0)
ZEND_ARG_INFO(0, anz_activities)
ZEND_ARG_INFO(0, clock_array)
ZEND_END_ARG_INFO()

const zend_function_entry clockthread_functions[] = {
	PHP_ME(Clockthread, return2line, arginfo_clockthread_return2line, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	PHP_ME(Clockthread, stop, arginfo_clockthread_stop, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	PHP_ME(Clockthread, start, arginfo_clockthread_start, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	PHP_FE_END
};

PHP_MINIT_FUNCTION(clockthread_init)
{
	zend_class_entry tmp_clockthread;
	INIT_CLASS_ENTRY(tmp_clockthread, "Clockthread", clockthread_functions);
	clockthread_class = zend_register_internal_class(&tmp_clockthread TSRMLS_CC);
	return SUCCESS;
}

PHP_MSHUTDOWN_FUNCTION(clockthread_shutdown)
{
	void* status;
	stop = 1;
	pthread_join(clock_thread, &status);
	return SUCCESS;
}

zend_module_entry clockthread_module_entry = {
	STANDARD_MODULE_HEADER,
	"clockthread", // the name of the extension.
	clockthread_functions,
	PHP_MINIT(clockthread_init),
	PHP_MSHUTDOWN(clockthread_shutdown),
	NULL, // RINIT
	NULL, // RSHUTDOWN
	NULL, // MINFO
	"0.1", //version of the extension.
	STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_CLOCKTHREAD
ZEND_GET_MODULE(clockthread)
#endif
