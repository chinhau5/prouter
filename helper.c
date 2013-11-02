/*
 * helper.c
 *
 *  Created on: Nov 2, 2013
 *      Author: chinhau5
 */

#include <stdlib.h>
#include "helper.h"

void tokenize(char *str, const char *delim, s_list *tokens)
{
	char *token;

	init_list(tokens);
	token = strtok(str, delim);
	while (token) {
		insert_into_list(tokens, token);
		token = strtok(NULL, " ");
	}
}
