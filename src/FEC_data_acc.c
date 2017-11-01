/******************************************************************************
 * FEC_data_acc.c
 *
 * Author: Hisakazu Ishiguro
 * Email:  hisakazuishiguro@gmail.com
 * Date:   10/30/2017
 ******************************************************************************/
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "err_number.h"
#include "FEC_metadata.h"
#include "FEC_data_acc.h"
#include "misc_util.h"

#define MAX_BYTSIZ_PER_LINE 1024

static int show_node_list(metadata_acc_class_t *obj_p, int *total_nodes_p)
{
  assert(obj_p != NULL);
  int err = 0;
  list_class_t *lstp;
  node_data_t *np, *head_p;
  int n_nodes, cnt;

  lstp = obj_p->list_obj_p;
  assert(lstp != NULL);

  if ((err = (lstp->func_get_head)(lstp, &head_p, &n_nodes)) != 0) {
    return err;
  }

  printf("NODE-LIST:\n");
  
  for (np = head_p, cnt = 0; np != NULL; np = np->next, cnt++) {
    printf("[%3d: k:%s p:%p]\n", cnt, np->key, np->targp);
  }
  printf("%d nodes detected in the list\n", n_nodes);

  if (total_nodes_p != NULL)
    *total_nodes_p = n_nodes;
  
  return err;
}

static int show_hashtbl(metadata_acc_class_t *obj_p, int *total_nodes_p)
{
  assert(obj_p != NULL);
  median_arch_parm_t *arch_p;
  link_node_t **hashtab_pp, *np;
  int i, node_cnt, idx_cnt;

  arch_p =  obj_p->arch_parm_p;
  hashtab_pp = arch_p->hashtab_pp;

  printf("HASHTAB:\n");
  
  for (i = 0, node_cnt = 0, idx_cnt = 0;
       i < arch_p->hash_sz;
       i++) {

    if (hashtab_pp[i] == NULL)
      continue;

    idx_cnt++;
    printf("[%04d]->", i);

    for (np = hashtab_pp[i]; np != NULL; np = np->next) {
      node_cnt++;
      printf("(%s:%2d)-", np->data_p->key, np->stats_p->acc_cnt);
    }

    printf("NULL\n");
  }

  printf("%d hashtab entry used\n", idx_cnt);
  printf("%d nodes detected\n", node_cnt);
  
  if (total_nodes_p != NULL)
    *total_nodes_p = node_cnt;

  return 0;
}

static int show_node_data(link_node_t *node_p)
{
  assert(node_p != NULL);
  int i;
  field_data_t *fldp;

  printf("NODE DATA:\n"
	 "\tdata_p = %p, stats_p = %p next = %p\n",
	 node_p->data_p, node_p->stats_p, node_p->next);

  if (node_p->data_p != NULL) {
    printf("\tkey=%s\n", node_p->data_p->key);

    fldp = node_p->data_p->data;
    for (i = 0;
	 (fldp[i].meta_name != FLD_TERMINATOR) &&
	   i < MAX_METADATA_SMPL_PER_LINE;
	 i++) {
      printf("\tmetaname=%2d data=%s\n",
	     fldp[i].meta_name, fldp[i].str_val);
    }
    printf("\n");
  } else {
    printf("No node data detected!\n");
  }

  return 0;
}

static int get_field_data(int meta_name, row_data_t *row_data_p, char **field_data_pp)
{
  int err = 0, idx = 0;
  int meta;
  assert(row_data_p != NULL && field_data_pp != NULL);

  while ((meta = row_data_p->data[idx].meta_name) != FLD_TERMINATOR) {
    if (meta == meta_name) {
      *field_data_pp = row_data_p->data[idx].str_val;
      return err;
    }
    idx++;
  }

  err = Err_CANNOT_FIND;
  printf("ERR, Cannot find meta name(=%d) in row_data. (%s, line=%d)\n",
	 meta_name, __FILE__, __LINE__);
  return err;
}

static int get_node_field_data(int meta_name, link_node_t *node_p, char **field_data_pp)
{
  assert(node_p != NULL);

  return (get_field_data(meta_name, node_p->data_p, field_data_pp));
}

static unsigned int hash_func(char *hash_key_p, int hash_sz)
{
  unsigned int hashval;
  char *key = hash_key_p;
  assert(hash_key_p != NULL && hash_sz > 0);

  for (hashval = 0; *key != '\0'; key++) {
    hashval = *key + (31 * hashval);    // 31 is prime number
  }

  return hashval % hash_sz;
}

static int build_node_acc_list(list_class_t *list_obj,
			       char *key_p,
			       link_node_t *node_p)
{
  int err = 0;
  assert(list_obj != NULL && key_p != NULL);

  if ((err = (list_obj->func_put_node)(list_obj, key_p, node_p)) != 0) {
    return err;
  }
  return err;
}

static int put_value(median_arch_parm_t *arch_p,
		     char *hashkey_p,
		     link_node_t *node_p,
		     list_class_t *list_obj_p)
{
  int err = 0;
  link_node_t *np;
  stats_info_t *stats_p = NULL;
  char *amount_str_p;
  int amount;

  assert(arch_p != NULL && hashkey_p != NULL && node_p != NULL);

  for (np = arch_p->hashtab_pp[arch_p->hashfunc(hashkey_p, arch_p->hash_sz)];
       np != NULL;
       np = np->next) {

    if (strcmp(hashkey_p, np->data_p->key) == 0) { // found
      /*
       * it means newly accessed node was already accessed hash table before
       * where both nodes has same key. So you need to get amound data from new node
       * (passed from arg), and add into the stats block where it's linked from the node
       * in hash table where it's created before.
       */
      assert(np->stats_p != NULL); // stats data block should be created already!
      /*
       * Where node_p is new node accessed from arg.
       * Where np is node that exists in hash table
       * both nd has same key.
       */
      if ((err = get_node_field_data(arch_p->meta_name_4stats_sum,
				     node_p,
				     &amount_str_p)) != 0) {
	return err;
      }
      
      if ((err = conv_str2int(amount_str_p, &amount)) != 0) {
	return err;
      }

      np->stats_p->acc_cnt++;
      np->stats_p->sum += amount;
      break;
    }
  }

  if (np == NULL) { // not found
    /*
     * it means the key of new node_p never been put in hash table before.
     * stats block need to be created and linked with then new node before put 
     * in the hash table.
     */
    if ((stats_p = (stats_info_t *)malloc(sizeof(stats_info_t))) == NULL) {
      err = Err_MALLOC;
      printf("ERR, malloc failed (%s, line=%d)\n", __FILE__, __LINE__);
      return err;
    }

    if ((err = get_node_field_data(arch_p->meta_name_4stats_sum,
				     node_p,
				     &amount_str_p)) != 0) {
      return err;
    }
      
    if ((err = conv_str2int(amount_str_p, &amount)) != 0) {
      goto err_handle;
    }

    stats_p->acc_cnt = 1;
    stats_p->sum = amount;
    node_p->stats_p = stats_p;
    /*
     * Put new node on head of linked list instead of tail.
     * it is faster. If it is first node in the list, their next
     * should point to the NULL.
     */
    node_p->next = arch_p->hashtab_pp[arch_p->hashfunc(hashkey_p, arch_p->hash_sz)];
    arch_p->hashtab_pp[arch_p->hashfunc(hashkey_p, arch_p->hash_sz)] = node_p;

    /*
     * build node access list here
     */
    if (list_obj_p != NULL) {
      if ((err = build_node_acc_list(list_obj_p, hashkey_p, node_p)) != 0) {
	goto err_handle;
      }
    }
  }
      
  return err;

 err_handle:
  if (stats_p != NULL)
    free(stats_p);
  return err;
}

static int cleanup(median_arch_parm_t *arch_p,
		   int *n_nodes_p)
{
  int err = 0;
  int i, n_nodes;
  link_node_t *np, *np_tmp;
  assert(arch_p != NULL);

  if (arch_p->hash_sz == 0 ||
      arch_p->hashtab_pp == NULL) {
    if (n_nodes_p != NULL) {
      *n_nodes_p = 0;
    }
    // there is no data to cleanup
    return err;
  }

  /*
   * Cleanup hash table
   */
  for (i = 0, n_nodes = 0; i < arch_p->hash_sz; i++) {
    if (arch_p->hashtab_pp[i] == NULL)
      continue;

    np = arch_p->hashtab_pp[i];

    while (np != NULL) {
      n_nodes++;
      np_tmp = np;
      np = np->next;
      if (np_tmp->data_p)  free(np_tmp->data_p);
      if (np_tmp->stats_p) free(np_tmp->stats_p);
      free(np_tmp);
    }
  }

  free(arch_p->hashtab_pp);
  
  if (n_nodes_p != NULL)
    *n_nodes_p = n_nodes;

  return err;
}

static median_arch_parm_t median_arch_parm_tbl[] = {
  { acc_type: MEDIAN_BY_ZIP,
    hash_sz: 100001,
    max_key_sz: MAX_KEY_SZ,
    hashtab_pp: NULL,
    metaname_lst_4keygen: { FLD_CMTE_ID, FLD_ZIP_CODE, FLD_TERMINATOR },
    rd_meta_format: { FLD_CMTE_ID, FLD_ZIP_CODE, FLD_TRANSACTION_DT,
		      FLD_TRANSACTION_AMT, FLD_OTHER_ID, FLD_TERMINATOR },
    wrt_meta_format: { FLD_CMTE_ID, FLD_ZIP_CODE, FLD_STATS_MEDIAN,
		       FLD_STATS_N_CONTRB, FLD_STATS_TOTAL_CONTRB, FLD_TERMINATOR },
    meta_name_4stats_sum: FLD_TRANSACTION_AMT,
    hashfunc: &hash_func,
    put:      &put_value,
    cleanup:  &cleanup,
  },
  
  { acc_type: MEDIAN_BY_DATE,
    hash_sz: 100001,
    max_key_sz: MAX_KEY_SZ,
    hashtab_pp: NULL,
    hashfunc: &hash_func,
    metaname_lst_4keygen: { FLD_CMTE_ID, FLD_TRANSACTION_DT, FLD_TERMINATOR },
    rd_meta_format: { FLD_CMTE_ID, FLD_TRANSACTION_DT,
		      FLD_TRANSACTION_AMT, FLD_OTHER_ID, FLD_TERMINATOR },
    wrt_meta_format: { FLD_CMTE_ID, FLD_TRANSACTION_DT, FLD_STATS_MEDIAN,
		       FLD_STATS_N_CONTRB, FLD_STATS_TOTAL_CONTRB, FLD_TERMINATOR },
    meta_name_4stats_sum: FLD_TRANSACTION_AMT,
    hashfunc: &hash_func,
    put:      &put_value,
    cleanup:  &cleanup,
  },

  { acc_type: MEDIAN_BY_NONE, // Table terminator
  },
};

static int get_arch_median_parm(metadata_acc_class_t *obj,
				DATA_ACC_TYPE acc_type,
				median_arch_parm_t **arch_pp)
{
  int err;
  median_arch_parm_t *arch_p;
  int i;

  assert(obj != NULL && acc_type < MEDIAN_BY_NONE && arch_pp != NULL);

  for (i = 0, arch_p = &(median_arch_parm_tbl[i]);
       arch_p->acc_type != MEDIAN_BY_NONE;
       i++) {

    if (arch_p->acc_type == acc_type) {
      *arch_pp = arch_p;
      return 0;
    }
  }
  
  err = Err_CANNOT_FIND;
  printf("ERR, Cannot find arch parm for (acc_type=%d), (%s, line=%d)\n",
	 acc_type, __FILE__, __LINE__);
  return err;
}

static int metadata_acc_init(metadata_acc_class_t *obj,
			     DATA_ACC_TYPE acc_type, char *input_fname_p,
			     char *output_fname_p, int do_sort, int verbose)
{
  int err = 0;
  metadata_class_t *metadata_obj_p = NULL;
  list_class_t *list_obj_p = NULL;
  
  assert(obj != NULL && input_fname_p != NULL && output_fname_p != NULL);

  obj->acc_type = acc_type;
  obj->input_fname_p = input_fname_p;
  obj->output_fname_p = output_fname_p;
  obj->do_sort = do_sort;
  obj->verbose = verbose;

  if ((err = get_arch_median_parm(obj, acc_type, &(obj->arch_parm_p))) != 0) {
    return err;
  }

  metadata_obj_p = obj->metadata_obj_p;
  assert(metadata_obj_p != NULL);

  if (metadata_obj_p->func_init != NULL) {
    if ((err = (metadata_obj_p->func_init)(metadata_obj_p)) != 0) {
      return err;
    }
  }

  list_obj_p = obj->list_obj_p;
  assert(list_obj_p != NULL);

  if (list_obj_p->func_init != NULL) {
    if ((err = (list_obj_p->func_init)(list_obj_p)) != 0) {
      return err;
    }
  }
  
  return err;
}

static int metadata_acc_open(metadata_acc_class_t *obj, void *ptr)
{
  int err = 0;
  metadata_class_t *metadata_obj_p = NULL;
  assert(obj != NULL);
  assert(obj->input_fname_p != NULL && obj->output_fname_p != NULL);
  assert(obj->metadata_obj_p != NULL);

  if ((obj->fp_r = fopen(obj->input_fname_p, "r")) == NULL) {
    printf("ERR, file \"%s\" cannot open! (%s, line=%d)\n",
	   obj->input_fname_p, __FILE__, __LINE__);
    err = Err_FILE_ACC;
    goto err_handler;
  }

  if ((obj->fp_w = fopen(obj->output_fname_p, "w")) == NULL) {
    printf("ERR, file \"%s\" cannot open! (%s, line=%d)\n",
	   obj->output_fname_p, __FILE__, __LINE__);
    err = Err_FILE_ACC;
    goto err_handler;
  }

  metadata_obj_p = obj->metadata_obj_p;

  if (metadata_obj_p->func_open != NULL) {
    if ((err = (metadata_obj_p->func_open)(metadata_obj_p)) != 0) {
      goto err_handler;
    }
  }

  return err;

 err_handler:
  if (obj->fp_r != NULL)
    fclose(obj->fp_r);

  if (obj->fp_w != NULL)
    fclose(obj->fp_w);

  return err;
}

static int metadata_acc_close(metadata_acc_class_t *obj, void *ptr)
{
  int err0, err1, err2;
  int n_nodes = 0;
  metadata_class_t *meta_objp;
  median_arch_parm_t *arch_p;
  list_class_t *lstp;
  
  assert(obj != NULL);
  err0 = err1 = err2 = 0;

  /*
   * Call cleanup methond of sub obj first.
   */
  if ((meta_objp = obj->metadata_obj_p) != NULL) {
    if (meta_objp->func_close != NULL) {
      err0 = meta_objp->func_close(meta_objp);
    }
  }

  if ((arch_p = obj->arch_parm_p) != NULL) {
    if (arch_p->cleanup != NULL) {
      err1 = arch_p->cleanup(arch_p, &n_nodes);
    }
  }

  if ((lstp = obj->list_obj_p) != NULL) {
    if (lstp->func_close != NULL) {
      err2 = lstp->func_close(lstp);
    }
  }

  if (obj->fp_r != NULL) fclose(obj->fp_r);
  if (obj->fp_w != NULL) fclose(obj->fp_w);

  if (err0) return err0;
  if (err1) return err1;
  if (err2) return err2;

  return 0;
}

 static int tokenize_data(metadata_acc_class_t *obj,
			  char        *str_p,    // string of each line from input file
			  link_node_t **node_pp, // data ptr to store hash table via hashtab
			  char        **key_pp,  // ptr of key for hash func
			  int         *skip_this_p) 
{
  int err = 0;
  row_data_t *row_p = NULL;
  link_node_t *node_p = NULL;
  char delimiter;
  int acc_type;
  median_arch_parm_t *arch_p;
  int n_tkn, targ_tkn, meta_key;
  int idx_targ_tkn, idx_row_field;
  int max_token;
  char *ptr_line_str, *ptr_keystr, *ptr_rtn;
  int is_complete;
  char *fld_data_p;
  int meta_name, key_len;
  int i;
  
  assert(obj != NULL && str_p != NULL && node_pp != NULL && key_pp != NULL);
  assert(obj->arch_parm_p);

  /*
   * Mem allocate for row meta data where it's to be stored inside of
   * link_node.
   */
  if ((row_p = (row_data_t *)malloc(sizeof(row_data_t))) == NULL) {
    err = Err_MALLOC;
    printf("ERR, malloc failed (%s, line=%d)\n", __FILE__, __LINE__);
    return err;
  }

  *skip_this_p = 0;
  delimiter = obj->delimiter;
  acc_type = obj->acc_type;
  arch_p = obj->arch_parm_p;
  row_p->data[0].meta_name = FLD_TERMINATOR; // just in case

  if ((err = obj->metadata_obj_p->get_numof_metanames(obj->metadata_obj_p,
						      &max_token)) != 0) {
    goto err_handle;
  }

  ptr_line_str = str_p;

  /*
   * Tokenize required meta named data, and save it as node data.
   */
  for (n_tkn = 0, idx_targ_tkn = 0, idx_row_field = 0, is_complete = 0;
       !is_complete && n_tkn < max_token;
       n_tkn++) {

    if ((targ_tkn = arch_p->rd_meta_format[idx_targ_tkn]) == FLD_TERMINATOR) {
      break;
    }

    if (targ_tkn == n_tkn) { // found target meta name, save the token!
      char _cnt = 0;
      char *_src_p = ptr_line_str;
      char *_dst_p = row_p->data[idx_row_field].str_val;

      row_p->data[idx_row_field].meta_name = targ_tkn;
       
      while (*_src_p != delimiter &&
	     *_src_p != '\0' &&
	     *_src_p != '\n' &&
	     _cnt < (MAX_METADATA_SMPL_SZ - 1)) {

	*_dst_p++ = *_src_p++;
	_cnt++;
      }

      if (*_src_p == delimiter) {
	ptr_line_str = _src_p + 1;
      } else if (*_src_p == '\0' || *_src_p == '\n') {
	is_complete = 1;
      } else if (_cnt >= (MAX_METADATA_SMPL_SZ - 1)) {
	printf("WAR, meta(name=%d) data size is too big! (%s, line=%d)\n",
	       targ_tkn, __FILE__, __LINE__);
      }

      *_dst_p = '\0';

      // adjust zip-code byte length
      if (n_tkn == FLD_ZIP_CODE &&
	  (strlen(row_p->data[idx_row_field].str_val) > 5)) {
	row_p->data[idx_row_field].str_val[5] = '\0';
      }
      
      idx_row_field++;
      idx_targ_tkn++;

    } else {
      // do skip token
      int _cnt = 0;
      char *_src_p = ptr_line_str;

      while (*_src_p != delimiter &&
	     *_src_p != '\0' &&
	     *_src_p != '\n' &&
	     _cnt < (MAX_METADATA_SMPL_SZ - 1)) {

	_src_p++;
      }

      if (*_src_p == delimiter) {
	ptr_line_str = _src_p + 1;
      } else if (*_src_p == '\0' || *_src_p == '\n') {
	is_complete = 1;
      } else if (_cnt >= MAX_METADATA_SZ) {
	printf("WAR, meta(name=%d) data size is too big! (%s, line=%d)\n",
	       n_tkn, __FILE__, __LINE__);
      }
    }
  }

  if (idx_row_field == 0 || idx_targ_tkn == 0) {
    err = Err_CANNOT_FIND;
    printf("ERR, Cannot find terget token from input file. (%s, line=%d)\n",
	   __FILE__, __LINE__);
    goto err_handle;
  }

  // Set termination symbol
  row_p->data[idx_row_field].meta_name = FLD_TERMINATOR;
  row_p->data[idx_row_field].str_val[0] = '\0';

  if ((err = get_field_data(FLD_OTHER_ID, row_p, &fld_data_p)) != 0) {
    goto err_handle;
  }

  if (*fld_data_p != '\0') {
    *skip_this_p = 1;
    goto skip_handle;
  }

  /*
   * Create hash key using specified meta names.
   */
  ptr_keystr = row_p->key;
  ptr_keystr[0] = '\0';

  for (i = 0; i < MAX_METANAME_LIST_FOR_KEYGEN; i++) {
    if ((meta_name = arch_p->metaname_lst_4keygen[i]) == FLD_TERMINATOR) {
      break;
    }

    if ((err = get_field_data(meta_name, row_p, &fld_data_p)) != 0) {
      goto err_handle;
    }

    if ((ptr_rtn = strcat(ptr_keystr, fld_data_p)) == NULL) {
      err = Err_CLIB_EXEC_ERR;
      printf("ERR, strcat() failed. (%s, line=%d)\n",
	     __FILE__, __LINE__);
      goto err_handle;
    }
  }
  
  if ((key_len = strlen(ptr_keystr)) <= 0) {
    err = Err_CANNOT_CREATE;
    printf("ERR, failed to create hash key. (%s, line=%d)\n",
	   __FILE__, __LINE__);
    goto err_handle;
  }

  /*
   * Mem allocate for link_node, and row_d will be ritten in the node
   * with hash key which is created above.
   */
  if ((node_p = (link_node_t *)malloc(sizeof(link_node_t))) == NULL) {
    err = Err_MALLOC;
    printf("ERR, malloc failed (%s, line=%d)\n", __FILE__, __LINE__);
    goto err_handle;
  }

  /*
   * Save hash key and required meta field datas in list node.
   */
  node_p->data_p = row_p;
  node_p->stats_p = NULL;
  node_p->next = NULL;

  *node_pp = node_p;
  *key_pp = node_p->data_p->key;
  
  return err;

 skip_handle:
 err_handle:
  if (row_p != NULL) {
    free(row_p);
    row_p = NULL;
  }

  if (node_p != NULL) {
    free(node_p);
    *node_pp = NULL;
  }

  return err;
}

static int median_data_with_sort(metadata_acc_class_t *obj, void *ptr)
{
  // Not supported yet !!!
  assert(obj != NULL);
  assert(obj->fp_r != NULL && obj->fp_w != NULL);
  assert(obj->arch_parm_p != NULL);

  return 0;
}

static int median_data_with_nosort(metadata_acc_class_t *obj, void *ptr)
{
  int err = 0, n_lines = 0;
  int len;
  char line[MAX_BYTSIZ_PER_LINE] = "";
  median_arch_parm_t *arch_p;
  link_node_t *node_p;
  char *hashkey_p;
  int skip_line;
  int meta_dtype;

  assert(obj != NULL);
  assert(obj->fp_r != NULL && obj->fp_w != NULL);
  assert(obj->arch_parm_p != NULL);

  arch_p = obj->arch_parm_p;

  /*
   * Check if metadata type for stats use.
   * their data type must be DT_NUM.
   */
  if ((err = (obj->metadata_obj_p->get_metadata_elem)(obj->metadata_obj_p,
						      arch_p->meta_name_4stats_sum,
						      &meta_dtype, NULL, NULL)) != 0) {
    return err;
  }

  if (meta_dtype != DT_NUM) {
    err = Err_INVALID_TYPE;
    printf("ERR, Invalid meta data type for stats operation is detected (dtype=%d) "
	   "(%s, line=%d)\n", meta_dtype, __FILE__, __LINE__);
    return err;
  }

  // Allocate hashtab array
  if ((arch_p->hashtab_pp =
       (link_node_t **)malloc(sizeof(link_node_t *) * arch_p->hash_sz)) == NULL) {
    err = Err_MALLOC;
    printf("ERR, malloc failed (%s, line=%d)\n", __FILE__, __LINE__);
    goto err_handle;
  }

  memset(arch_p->hashtab_pp, 0, arch_p->hash_sz);

  while (fgets(line, sizeof(line), obj->fp_r)) {
    if ((len = strlen(line)) <= 0) {
      continue;
    }

    // Tokenize input line data and caliculate hash key
    if ((err = tokenize_data(obj, line, &node_p, &hashkey_p, &skip_line)) != 0) {
      goto err_handle;
    }

    if (skip_line)
      continue;
#if 0 // Hisakazu
    if (obj->verbose) {
      show_node_data(node_p);
    }
#endif    
    // put node data into hash table
    if ((err = arch_p->put(arch_p, hashkey_p, node_p, obj->list_obj_p)) != 0) {
      goto err_handle;
    }
    //    printf("put finished ****** (%d)\n", n_lines); // Hisakazu
    n_lines++;
  }
#if 0 // Hisakazu
  if (obj->verbose) {
    show_node_list(obj, NULL);
  }
#endif
  return err;

 err_handle:
  if (arch_p->hashtab_pp != NULL) {
    free(arch_p->hashtab_pp);
    arch_p->hashtab_pp = NULL;
  }
  return err;
}

static int metadata_acc_median_data(metadata_acc_class_t *obj, void *ptr)
{
  int err = 0;
  int total_nodes = 0;
  assert(obj != NULL);

  switch (obj->acc_type) {
  case MEDIAN_BY_ZIP:
  case MEDIAN_BY_DATE:
    if (obj->do_sort) { // use balanced tree algorithme.
      if ((err = median_data_with_sort(obj, ptr)) != 0) {
	goto err_handler;
      }
    } else { // without sort, use has algoriithms.
      if ((err = median_data_with_nosort(obj, ptr)) != 0) {
	goto err_handler;
      }
    }
    break;
    
  default:
    err = Err_INVALID_TYPE;
    printf("ERR, unknown median calc type detected (%d), (%s, line=%d)\n",
	   obj->acc_type, __FILE__, __LINE__);
    goto err_handler;
  }

  if (obj->verbose) {
    show_hashtbl(obj, &total_nodes);
  }
  
  return err;
  
 err_handler:
  // Put right code later --Hisakazu
  return err;
}

static int median_output_with_sort(metadata_acc_class_t *obj, void *ptr)
{
  // Not supported yet !!!
  assert(obj != NULL);
  assert(obj->fp_r != NULL && obj->fp_w != NULL);
  assert(obj->arch_parm_p != NULL);

  return 0;
}

static int median_output_with_nosort(metadata_acc_class_t *obj, void *ptr)
{
  assert(obj != NULL);
  assert(obj->fp_r != NULL && obj->fp_w != NULL);
  assert(obj->arch_parm_p != NULL);
  assert(obj->list_obj_p != NULL);

  int i, err = 0;
  median_arch_parm_t *arch_p = obj->arch_parm_p;
  int *format_p = arch_p->wrt_meta_format;
  list_class_t *lstp = obj->list_obj_p;
  node_data_t *np, *head_p;
  link_node_t *node_p;
  int meta, median;
  int is_first_wrt = 1;
  char *field_data_p;
  char dlmt[2] = { obj->delimiter, '\0' };
  char meta_buf[MAX_METADATA_SZ];
  char line_buf[MAX_BYTSIZ_PER_LINE] = {'\0',};

  if ((err = (lstp->func_get_head)(lstp, &head_p, NULL)) != 0) {
    return err;
  }

  for (np = head_p; np != NULL; np = np->next) {
    if ((node_p = (link_node_t *)(np->targp)) == NULL) {
      err = Err_NULL_DATA;
      printf("ERR, No target node pointer(key=%s). (%s, line=%d)\n",
	     np->key, __FILE__, __LINE__);
      return err;
    }

    i = 0;
    is_first_wrt = 1;
    
    while ((meta = format_p[i++]) != FLD_TERMINATOR) {
      if (meta < FLD_TBL_SZ) {
	if ((err = get_node_field_data(meta, node_p, &field_data_p)) != 0) {
	  return err;
	}
	strcpy(meta_buf, field_data_p);
      } else {
	assert(node_p->stats_p != NULL);
	switch (meta) {
	case FLD_STATS_MEDIAN:
	  median = node_p->stats_p->sum / node_p->stats_p->acc_cnt;
	  sprintf(meta_buf, "%d", median);
	  break;
	  
	case FLD_STATS_N_CONTRB:
	  sprintf(meta_buf, "%d", node_p->stats_p->acc_cnt);
	  break;
	  
	case FLD_STATS_TOTAL_CONTRB:
	  sprintf(meta_buf, "%d", node_p->stats_p->sum);
	  break;

	default:
	  err = Err_INVALID_TYPE;
	  printf("ERR, Invalid stats type(%d) detected in key(%s)! (%s, line=%d)\n",
		 meta, np->key, __FILE__, __LINE__);
	  return err;
	}
      }

      if (!is_first_wrt) {
	strcat(line_buf, dlmt);
	strcat(line_buf, meta_buf);
      } else {
	strcat(line_buf, meta_buf);
	is_first_wrt = 0;
      }
    }
#if 0 // Hisakazu
    if (obj->verbose) {
      printf("frmt: %s\n", line_buf);
    }
#endif
    // write to output file
    fprintf(obj->fp_w, "%s\n", line_buf);
    line_buf[0] = '\0';
  }

  return err;
}

static int metadata_acc_output_data(metadata_acc_class_t *obj, void *ptr)
{
  int err = 0;
  int total_nodes = 0;
  assert(obj != NULL);

  switch (obj->acc_type) {
  case MEDIAN_BY_ZIP:
  case MEDIAN_BY_DATE:
    if (obj->do_sort) { // not supported yet.
      if ((err = median_output_with_sort(obj, ptr)) != 0) {
	goto err_handler;
      }
    } else {  // outpt based on access order
      if ((err = median_output_with_nosort(obj, ptr)) != 0) {
	goto err_handler;
      }
    }
    break;
    
  default:
    err = Err_INVALID_TYPE;
    printf("ERR, unknown median calc type detected (%d), (%s, line=%d)\n",
	   obj->acc_type, __FILE__, __LINE__);
    goto err_handler;
  }

  return err;
  
 err_handler:
  // Put right code later --Hisakazu
  return err;
}

static metadata_acc_class_t metadata_fec_contribbyind_acc_obj = {
 meta_spec:        META_FEC_CONTRIBUTION_BY_IND,
 acc_type:         MEDIAN_BY_NONE,
 delimiter:        FLD_DELIMITER,
 input_fname_p:    NULL,
 output_fname_p:   NULL,
 arch_parm_p:      NULL,
 do_sort:          0,
 verbose:          0,
 fp_r:             NULL,
 fp_w:             NULL,
 func_init:        &metadata_acc_init,
 func_open:        &metadata_acc_open,
 func_median_data: &metadata_acc_median_data,
 func_output_data: &metadata_acc_output_data,
 func_close:       &metadata_acc_close,
};

int attach_metadata_obj(metadata_acc_class_t *metadata_acc_obj_p,
			metadata_class_t *metadata_obj_p)
{
  int err = 0;
  assert(metadata_acc_obj_p != NULL &&
	 metadata_obj_p != NULL);

  if (metadata_acc_obj_p->meta_spec !=
      metadata_obj_p->meta_spec) {

    printf("ERR, metadata type is mismatched between metadata_acc_obj and metadata_obj!\n"
	   "%d != %d (%s, line=%d)\n",
	   metadata_acc_obj_p->meta_spec, metadata_obj_p->meta_spec,
	   __FILE__, __LINE__);

    err = Err_DATA_MISMATCH;
    return err;
  }

  metadata_acc_obj_p->metadata_obj_p = metadata_obj_p;
  return err;
}

int attach_list_obj(metadata_acc_class_t *metadata_acc_obj_p,
		    list_class_t *list_obj_p)
{
  int err = 0;
  assert(metadata_acc_obj_p != NULL && list_obj_p != NULL);

  if (metadata_acc_obj_p->meta_spec != list_obj_p->meta_spec) {

    printf("ERR, metadata type is mismatched between metadata_acc_obj and list_obj!\n"
	   "%d != %d (%s, line=%d)\n",
	   metadata_acc_obj_p->meta_spec, list_obj_p->meta_spec,
	   __FILE__, __LINE__);

    err = Err_DATA_MISMATCH;
    return err;
  }

  metadata_acc_obj_p->list_obj_p = list_obj_p;
  return err;
}

int get_metadata_acc_obj(int meta_spec,
			 metadata_acc_class_t **metadata_acc_obj_pp)
{
  int err = 0;
  assert(metadata_acc_obj_pp != NULL);

  switch (meta_spec) {
  case META_FEC_CONTRIBUTION_BY_IND:
    *metadata_acc_obj_pp = &metadata_fec_contribbyind_acc_obj;
    break;

  default:
    printf("ERR, Cannot find metadata acc obj(%d) (%s, line=%d)\n",
	   meta_spec, __FILE__, __LINE__);
    err = Err_CANNOT_FIND_OBJ;
    break;
  }
  return err;
}
