/******************************************************************************
 * misc_util.c
 *
 * Author: Hisakazu Ishiguro
 * Email:  hisakazuishiguro@gmail.com
 * Date:   10/30/2017
 ******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>

#include "err_number.h"


int is_number(char *str)
{
  int i, len, rtn;
  
  assert(str != NULL);
  len = strlen(str);
  assert(len > 0);

  for (i = 0; i < len; i++) {
    // supports negative number
    if (i == 0 && str[i] == '-')
      continue;
    
    if ((rtn = isdigit(str[i])) == 0) {
      printf("ERR, Sting is not digit, [%s, line %d]\n",
	      __FILE__, __LINE__);
      return Err_INVALID_ARG;
    }
  }

  return 0;
}

int conv_str2int(char *str, int *num_p)
{
  int err = 0;
  
  assert(str != NULL && num_p != NULL);

  if ((err = is_number(str)) != 0) {
    return err;
  }

  *num_p = atoi(str);
  return err;
}

int hashfunc_short_strkey(char *str_key, int mod, int *val_p)
{
  int err = 0, len;
  int i, sum;

  assert(str_key != NULL && val_p != NULL && mod > 1);
  len = strlen(str_key);
  
  if (len <= 0 || len > 20) {
    err = Err_INVALID_ARG;
    printf("ERR, invalid 'str_key' detected(len=%d) (%s, line=%d)\n",
	   len, __FILE__, __LINE__);
    return err;
  }

  for (i = 0, sum = 0; i < len; i++) {
    sum += (int)str_key[i];
  }

  *val_p = sum % mod;
  return err;
}

