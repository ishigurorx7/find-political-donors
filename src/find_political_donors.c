/******************************************************************************
 * find_political_donors.c
 *
 * Author: Hisakazu Ishiguro
 * Email:  hisakazuishiguro@gmail.com
 * Date:   10/30/2017
 ******************************************************************************/
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <time.h>

#include "err_number.h"
#include "FEC_metadata.h"
#include "FEC_list.h"
#include "FEC_data_acc.h"
#include "misc_util.h"

static void print_usage(char *command)
{
  assert(command != NULL);

  printf("%s usage:[-hvs] -f input_fname -o output_fname\n", command);
  printf("Arguments:\n"
	 "\t-v         : Enable verbose mode\n"
	 "\t-s         : Do sort before output to file\n"
	 "\t-f <fname> : Specify input  file name [Mandentory]\n"
	 "\t-o <fname> : Specify output file name [Mandentory]\n"
	 "\t-m <num>   : %d[Default]: MEDIAN_BY_ZIP, %d: MEDIAN_BY_DATE\n"
	 "\t-h         : Print command usage\n\n",
	 MEDIAN_BY_ZIP, MEDIAN_BY_DATE);
}

static void print_start_msg(char *command,
			    char *input_fname,
			    char *output_fname,
			    int do_sort,
			    int median_by)
{
  time_t t = time(NULL);
  struct tm *tm = localtime(&t);
  
  assert(command != NULL &&
	 input_fname != NULL &&
	 output_fname != NULL);

  printf("Start %s command ...\n", command);
  printf("with arguments:\n"
	 "\tInput  fname: \"%s\"\n"
	 "\tOutput fname: \"%s\"\n"
	 "\tMedianBy: %d\n",
	 input_fname, output_fname, median_by);

  if (do_sort) {
    printf("\tSort: Enabled\n");
  }

  printf("\n%s", asctime(tm));
}

static int chk_command_args(int argc,
			    char **argv,
			    int *arg_help_p,
			    int *arg_verbose_p,
			    int *arg_do_sort_p,
			    int *arg_median_by_p,
			    char **arg_input_fname_pp,
			    char **arg_output_fname_pp)
{
  extern char *optarg;
  int opt, err = 0;

  assert(arg_input_fname_pp != NULL &&
	 arg_output_fname_pp != NULL);

  while ((opt = getopt(argc, argv, "hvsf:o:m:")) != -1) {
    switch (opt) {
    case 'h':
    case '?':
      *arg_help_p = 1;
      break;

    case 'v':
      *arg_verbose_p = 1;
      break;

    case 's':
      *arg_do_sort_p = 1;
      break;
      
    case 'f':
      *arg_input_fname_pp = optarg;
      break;

    case 'o':
      *arg_output_fname_pp = optarg;
      break;

    case 'm':
      if ((err = conv_str2int(optarg, arg_median_by_p)) != 0) {
	return err;
      }

      switch (*arg_median_by_p) {
      case MEDIAN_BY_ZIP:
      case MEDIAN_BY_DATE:
	break;

      default:
	err = Err_INVALID_CMD_ARG;
	printf("ERR, -m %d, Invalid argument [%s, line=%d]\n",
	       *arg_median_by_p, __FILE__, __LINE__);
	break;
      }
      break;

    default:
      err = Err_INVALID_CMD_ARG;
      break;
    }
  }

  if (err || *arg_help_p ||
      *arg_input_fname_pp == NULL || *arg_output_fname_pp == NULL) {
    print_usage(argv[0]);
    return 0;
  }

  if (*arg_verbose_p) {
    print_start_msg(argv[0], *arg_input_fname_pp, *arg_output_fname_pp,
		    *arg_do_sort_p, *arg_median_by_p);
  }

  return 0;
}

static int set_metadata_objects(int meta_spec,
				metadata_acc_class_t **meta_obj_pp)
{
  int err = 0;
  metadata_class_t *metadata_obj_p = NULL;
  list_class_t *list_obj_p = NULL;

  if ((err = get_metadata_acc_obj(meta_spec, meta_obj_pp)) != 0) {
    return err;
  }

  if ((err = get_metadata_obj(meta_spec, &metadata_obj_p)) != 0) {
    return err;
  }

  if ((err = attach_metadata_obj(*meta_obj_pp, metadata_obj_p)) != 0) {
    return err;
  }

  if ((err = get_list_obj(meta_spec, &list_obj_p)) != 0) {
    return err;
  }

  if ((err = attach_list_obj(*meta_obj_pp, list_obj_p)) != 0) {
    return err;
  }

  return err;
}

int main(int argc, char **argv)
{
  int err = 0;
  char *cmd = argv[0];

  // cmd argument variables
  int arg_verbose = 0;                   // -v (disable in default)
  int arg_do_sort = 0;                   // -s (disable in default)
  int arg_help = 0;                      // -h (disable in default)
  int arg_median_by = MEDIAN_BY_ZIP;     // -m <0/1/..> (0: default)
  char *arg_input_fname_p = NULL;        // -f <input  fname> [Mandentory]
  char *arg_output_fname_p = NULL;       // -0 <output fname> [Mandentory]

  int meta_spec = META_FEC_CONTRIBUTION_BY_IND;
  metadata_acc_class_t *meta_obj_p = NULL;

  printf("\nStart '%s' program ...", argv[0]);
  
  /*
   * Test and set command argument.
   */
  if ((err = chk_command_args(argc, argv,
			      &arg_help, &arg_verbose, &arg_do_sort, &arg_median_by,
			      &arg_input_fname_p, &arg_output_fname_p)) != 0) {
    return err;
  }

  /*
   * Set specified metadata obj and their access obj.
   */
  if ((err = set_metadata_objects(meta_spec, &meta_obj_p)) != 0) {
    return err;
  }

  printf("\nInitialize and validating program  ...");
  
  /*
   * Initialize metadata obj by putting command arguments.
   */
  if ((err = (meta_obj_p->func_init)(meta_obj_p,
				     arg_median_by, arg_input_fname_p, arg_output_fname_p,
				     arg_do_sort, arg_verbose)) != 0) {
    return err;
  }

  printf("\nStart reading data from input file ...");
  
  /*
   * Open metadata input/output files
   */
  if ((err = (meta_obj_p->func_open)(meta_obj_p, NULL)) != 0) {
    return err;
  }

  printf("\nStart processing data ...");
  
  if ((err = (meta_obj_p->func_median_data)(meta_obj_p, NULL)) != 0) {
    return err;
  }

  printf("\nStart writing median values to output ...");
  
  if ((err = (meta_obj_p->func_output_data)(meta_obj_p, NULL)) != 0) {
    return err;
  }

  printf("\nCleaning up before exiting program ...");
  
  if ((err = (meta_obj_p->func_close)(meta_obj_p, NULL)) != 0) {
    return err;
  }
  printf("\nProgram complete!\n");
  return err;
}
