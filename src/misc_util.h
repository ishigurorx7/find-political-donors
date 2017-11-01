#ifndef MISC_UTIL_H
#define MISC_UTIL_H

#define NUM_ELEMS(x) (sizeof(x) / sizeof((x)[0]))

extern int is_number(char *str);
extern int conv_str2int(char *str, int *num_p);
extern int hashfunc_short_strkey(char *str_key, int mod, int *val_p);

#endif // MISC_UTIL_H
