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
#include <termios.h>
#include <unistd.h>

static int line;
static int stop;

struct clock_struct {
	time_t start_time;
	int x, y;
};

struct clocks_struct {
	struct clock_struct* clock;
	int anz_clocks;
};

/*
 * The function which runs the clocks
 */

static void *clock_loop(void* _clock_args) {
	int x, y, i;
	struct clocks_struct *clock_args = (struct clocks_struct*) _clock_args;
	while (stop == 0) {
		time_t now;
		time(&now);
		long dauer;
		int stunden, minuten, sekunden;
		for (i = 0; i < clock_args->anz_clocks; i++) {
			dauer = difftime(now, clock_args->clock[i].start_time);
			x = clock_args->clock[i].x;
			y = clock_args->clock[i].y;
			stunden = dauer / 3600;
			minuten = (dauer % 3600) / 60;
			sekunden = (dauer % 3600) % 60;
			php_printf("\x1B[%d;%dH%02d:%02d:%02d\n", y, x, stunden, minuten, sekunden);
		}
		php_printf("\x1B[%d;%dH", line + 4, 3);
		fflush(stdout);
		sleep(1);
	}
//	free(clock_args->clock);
	return NULL;
}

/*
 * prepares the parameters for the clock thread and starts it
 */
static void run_clocks(pthread_t* clock_thread, struct clocks_struct* clock_args, zval* clock_array) {
	char *key;
	uint key_len;
	int array_count;
	HashTable *arr_hash;
	HashPosition pointer;
	zval **data;
	ulong index;

	arr_hash = Z_ARRVAL_P(clock_array);
	array_count = zend_hash_num_elements(arr_hash);
	clock_args->clock = calloc(array_count, sizeof(struct clock_struct));
	clock_args->anz_clocks = array_count;

	int i = 0;

	for(zend_hash_internal_pointer_reset_ex(arr_hash, &pointer);
			zend_hash_get_current_data_ex(arr_hash, (void**) &data, &pointer) == SUCCESS;
			zend_hash_move_forward_ex(arr_hash, &pointer)) {

		zend_hash_get_current_key_ex(arr_hash, &key, &key_len, &index, 0, &pointer);
		clock_args->clock[i].start_time = Z_LVAL_PP(data);
		clock_args->clock[i].x = 62;
		clock_args->clock[i].y = index + 4;
		i++;
	}

	pthread_create(clock_thread, NULL, clock_loop, (void*)clock_args);
}

/*
 * gets and processes the user input
 */
static char* getAction(long anz_activities) {
	static char action[100];
	char pressed_key;

	struct termios oldt, newt;
	tcgetattr( STDIN_FILENO, &oldt );
	newt = oldt;
	newt.c_lflag &= ~( ICANON | ECHO );
	tcsetattr( STDIN_FILENO, TCSANOW, &newt );
	strcpy(action, "undefined");
	while (1) {
		pressed_key = getchar();
		if (pressed_key == 65 && line > 0) { //is arrow up pressed?
			line--;
			php_printf("\x1B[%d;%dH", line + 4, 3);
		}
		if (pressed_key == 66 && line < anz_activities - 1) {   //is arrow down pressed?
			line++;
			php_printf("\x1B[%d;%dH", line + 4, 3);
		}
		if (pressed_key == 'q') {
			sprintf(action, "quit,%d", line);
			break;
		}
		if (pressed_key == 10) {  //is ENTER pressed?
			sprintf(action, "enter,%d", line);
			break;
		}
	}
	stop = 1;
	tcsetattr( STDIN_FILENO, TCSANOW, &oldt );
	return (char*)&action;
}

/*
 * The implementation of the PHP function which starts the whole thing
 */
PHP_FUNCTION(dime_clock_action) {
	long anz_activities;
	char action[100];
	void* status;
	zval *clock_array;
	pthread_t clock_thread;
	struct clocks_struct clock_args;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "la", &anz_activities, &clock_array) != SUCCESS) {
		return;
	}

	line = 0;
	stop = 0;

	clock_args.anz_clocks = 0;
	clock_args.clock = NULL;

	//	pthread_create(&clock_thread, NULL, clock_loop, (void*)&clock_args);
	run_clocks(&clock_thread, &clock_args, clock_array);
	strcpy(action, getAction(anz_activities));
	stop = 1;
	pthread_join(clock_thread, &status);
	free(clock_args.clock);
	RETURN_STRING(action, 1);
}

// We give PHP aware of our function, indicating its function table module.
const zend_function_entry clockthread_functions[] = {
	PHP_FE(dime_clock_action, NULL)
	PHP_FE_END
};

// We define a function that will cause php when connecting our expansion.
PHP_MINIT_FUNCTION( clockthread_init )
{
	return SUCCESS;
}

zend_module_entry clockthread_module_entry = {
	STANDARD_MODULE_HEADER,
	"clockthread", // the name of the extension.
	clockthread_functions,
	PHP_MINIT(clockthread_init),
	NULL, // MSHUTDOWN
	NULL, // RINIT
	NULL, // RSHUTDOWN
	NULL, // MINFO
	"0.1", //version of the extension.
	STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_CLOCKTHREAD
ZEND_GET_MODULE(clockthread)
#endif

