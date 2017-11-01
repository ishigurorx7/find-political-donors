#ifndef FEC_DATA_ACC_H
#define FEC_DATA_ACC_H

#include "FEC_metadata.h"
#include "FEC_list.h"

typedef enum {
  MEDIAN_BY_ZIP = 0,
  MEDIAN_BY_DATE,
  MEDIAN_BY_NONE,
} DATA_ACC_TYPE;

/*
 * Following macro defined number could be different by 
 * DATA_ACC_TYPE. 
 */
#define MAX_METADATA_SMPL_SZ       30
#define MAX_METADATA_SMPL_PER_LINE 10

typedef struct field_data {
  int  meta_name;
  char str_val[MAX_METADATA_SMPL_SZ];
} field_data_t;

typedef struct row_data {
  char key[MAX_KEY_SZ];
  field_data_t data[MAX_METADATA_SMPL_PER_LINE];
} row_data_t;

typedef struct stats_info {
  int acc_cnt;
  int sum;
} stats_info_t;

typedef struct link_node {
  row_data_t       *data_p;
  stats_info_t     *stats_p;
  struct link_node *next;
} link_node_t;

typedef enum {
  FLD_STATS_MEDIAN = FLD_TBL_SZ + 1, // Must not overlapped with METADATA_NAME
  FLD_STATS_N_CONTRB,                // # of contributions
  FLD_STATS_TOTAL_CONTRB,            // total amount of contributions
} FLD_STATS;

#define MAX_FLD_PER_LINE  30
#define MAX_METANAME_LIST_FOR_KEYGEN 5

typedef struct median_arch_parm {
  DATA_ACC_TYPE acc_type;
  int hash_sz;
  int max_key_sz;
  link_node_t **hashtab_pp;
  int metaname_lst_4keygen[MAX_METANAME_LIST_FOR_KEYGEN];
  int rd_meta_format[MAX_FLD_PER_LINE];
  int wrt_meta_format[MAX_FLD_PER_LINE];
  int meta_name_4stats_sum;
  unsigned int (*hashfunc)(char *key, int hash_sz);
  int (*put)(struct median_arch_parm *arch_p, char *hashkey_p,
	     link_node_t *node_p, list_class_t *list_obj_p);
  int (*cleanup)(struct median_arch_parm *arch_p, int *n_nodes_p);
} median_arch_parm_t;

typedef struct metadata_acc_class {
  int meta_spec;
  DATA_ACC_TYPE acc_type;
  char delimiter;
  char *input_fname_p;
  char *output_fname_p;
  FILE *fp_r;
  FILE *fp_w;
  int do_sort;                         // 0: disable sort
  int verbose;
  median_arch_parm_t *arch_parm_p;
  metadata_class_t   *metadata_obj_p;  // input file format
  list_class_t       *list_obj_p;

  int (*func_init)(struct metadata_acc_class *obj,
		   DATA_ACC_TYPE acc_type, char *input_fname_p,
		   char *output_fname_p, int do_sort, int verbose);
  int (*func_open)(struct metadata_acc_class *obj, void *ptr);
  int (*func_median_data)(struct metadata_acc_class *obj, void *ptr);
  int (*func_output_data)(struct metadata_acc_class *obj, void *ptr);
  int (*func_close)(struct metadata_acc_class *obj, void *ptr);

} metadata_acc_class_t;

extern int get_metadata_acc_obj(int meta_spec,
				metadata_acc_class_t **metadata_acc_obj_pp);

extern int attach_metadata_obj(metadata_acc_class_t *metadata_acc_obj_p,
			       metadata_class_t *metadata_obj_p);

extern int attach_list_obj(metadata_acc_class_t *metadata_acc_obj_p,
			   list_class_t *list_obj_p);

#endif // FEC_DATA_ACC_H
