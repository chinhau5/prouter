/*
 * helper.h
 *
 *  Created on: Nov 2, 2013
 *      Author: chinhau5
 */

#ifndef HELPER_H_
#define HELPER_H_

#include "list.h"

void tokenize(char *str, const char *delim, s_list *tokens);
char *tokenize_name_and_index(char *reference, int *low, int *high);

#endif /* HELPER_H_ */
