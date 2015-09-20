#ifndef PHP_CLOCKTHREAD_H
#define PHP_CLOCKTHREAD_H

extern zend_module_entry clockthread_module_entry;
#define phpext_clockthread_ptr &clockthread_module_entry

//If we compile a thread-safe version, connect the appropriate header file.
#ifdef ZTS
#include "TSRM.h"
#endif

#endif

