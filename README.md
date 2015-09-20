# PHP Extension ClockThread
## What is it?
It is a PHP Extension which adds one ticking clock (or some) to an interactive PHP Console Application. 

For now it works only with the DimeConsole (https://github.com/dime-timetracker/console). But hopefully it will grow into some more universal tool.

## Installation
Check it out.

And install it like any other PHP Extension:

	`phpize`
	
	`./configure`
	
	`make`
	
	`make install`
	

Add the line "extension=clockthread.so" to your php.ini

## Uninstallation
Remove the line "extension=clockthread.so" from your php.ini

Delete the file clockthread.so from your PHP Extension folder.

Delete the checked out sources.