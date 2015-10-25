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
			php_printf("\x1B[%d;%dH%02d:%02d:%02d\n", y, x, stunden, minuten, sekunden); //moves the cursor to row y, col x and prints the time
		}
		php_printf("\x1B[%d;%dH", line + 4, 3); //moves the cursor back
		fflush(stdout);
		sleep(1);
	}
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

//reads the clock params from the zval param into internal structure
	arr_hash = Z_ARRVAL_P(clock_array);
	array_count = zend_hash_num_elements(arr_hash);
	clock_args->clock = ecalloc(array_count, sizeof(struct clock_struct));
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

//starts the actual clock thread
	pthread_create(clock_thread, NULL, clock_loop, (void*)clock_args);
}

/*
 * gets and processes the user input
 */
void getAction(long anz_activities, zval* return_value) {
	char pressed_key;

//deactivates standard console behaviour (no echo, no carriage return if Enter is pressed)
	struct termios oldt, newt;
	tcgetattr( STDIN_FILENO, &oldt );
	newt = oldt;
	newt.c_lflag &= ~( ICANON | ECHO );
	tcsetattr( STDIN_FILENO, TCSANOW, &newt );

	array_init_size(return_value, 2);
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
			//says we want to exit the application
			add_next_index_string(return_value, "quit", 1);
			add_next_index_long(return_value, line);
			break;
		}
		if (pressed_key == 10) {  //is ENTER pressed?
			//says we want to switch the chosen activity
			add_next_index_string(return_value, "enter", 1);
			add_next_index_long(return_value, line);
			break;
		}
	}
	stop = 1; //if q or enter is pressed we send the cklock loop a stop signal
	tcsetattr( STDIN_FILENO, TCSANOW, &oldt ); //we reactivate the standard console behaviour
	return;
}

/*
 * The implementation of the PHP function
 */
PHP_FUNCTION(dime_clock_action) {
	//the function params
	long anz_activities;
	zval *clock_array;

	//for the thread control
	pthread_t clock_thread;
	void* status;

	line = 0;
	stop = 0;

	struct clocks_struct clock_args;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "la", &anz_activities, &clock_array) != SUCCESS) {
		return;
	}

	clock_args.anz_clocks = 0;
	clock_args.clock = NULL;

	run_clocks(&clock_thread, &clock_args, clock_array);
	getAction(anz_activities, return_value);
	stop = 1;
	pthread_join(clock_thread, &status);
	efree(clock_args.clock);

	RETURN_ZVAL(return_value, 0, 0);
}

//Parameter info
ZEND_BEGIN_ARG_INFO(arginfo_dime_clock_action, 0)
ZEND_ARG_INFO(0, anz_activities)
ZEND_ARG_INFO(0, clock_array)
ZEND_END_ARG_INFO()

//Information about the module functions for the Zend engine
const zend_function_entry clockthread_functions[] = {
	PHP_FE(dime_clock_action, arginfo_dime_clock_action)
	PHP_FE_END
};

//Module init
PHP_MINIT_FUNCTION( clockthread_init )
{
	return SUCCESS;
}

//Information about the module for the Zend engine
zend_module_entry clockthread_module_entry = {
	STANDARD_MODULE_HEADER,
	"clockthread", // the name of the extension.
	clockthread_functions,
	PHP_MINIT(clockthread_init), //when the the extension is loaded
	NULL, // MSHUTDOWN
	NULL, // RINIT  //when a request is started
	NULL, // RSHUTDOWN
	NULL, // MINFO  // for PHPinfo
	"0.1", //version of the extension.
	STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_CLOCKTHREAD
ZEND_GET_MODULE(clockthread)
#endif

