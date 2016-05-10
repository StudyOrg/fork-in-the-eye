#pragma once

#include <stdarg.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#define TEST(_condition, _message) (assertf( (_condition), "\033[31;1mError\033[0m", (_message)))

int assertf( int condition, const char* head_msg, const char* str) {
	if(condition) {
		fprintf(stderr, "%s\n", head_msg);

		if( str && strlen(str) ) {
			fprintf(stderr, "%s\n", str);
		}

		exit(1);
	}

	return condition;
}
