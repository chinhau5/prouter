/*
 * helper.c
 *
 *  Created on: Nov 2, 2013
 *      Author: chinhau5
 */

#include <stdlib.h>
#include <assert.h>
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

char *tokenize_name_and_index(char *name_and_index, int *low, int *high)
{
	s_list name_and_index_tokens;
	s_list index_tokens;
	char *temp;

	tokenize(name_and_index, "[]", &name_and_index_tokens);

	if (name_and_index_tokens.num_items == 1) {
		*low = 0;
		*high = *low;
	} else {
		assert(name_and_index_tokens.num_items == 2);

		tokenize(name_and_index_tokens.head->next->data, ":", &index_tokens);

		if (index_tokens.num_items == 1) {
			*low = 0;
			*high = *low;
		} else {
			assert(index_tokens.num_items == 2);

			*low = atoi(index_tokens.head->next->data);
			*high = atoi(index_tokens.head->data);

			assert(*low <= *high);
		}
	}

	temp = name_and_index_tokens.head->data;

	return temp;
}
