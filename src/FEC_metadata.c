/******************************************************************************
 * FEC_metadata.c
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
#include "misc_util.h"

// Meta data's byte size in spec plus 1 (nul car)
#define META_BYTSZ(size)  ((size) + 1)

static metadata_info_t meta_fec_contribbyind_tbl[] = {
  { FLD_CMTE_ID,         DT_CHARS, META_BYTSZ(9),   0 },  //  0
  { FLD_AMNDT_IND,       DT_CHARS, META_BYTSZ(1),   1 },  //  1
  { FLD_RPT_TP,          DT_CHARS, META_BYTSZ(3),   1 },  //  2
  { FLD_TRANSACTION_PGI, DT_CHARS, META_BYTSZ(5),   1 },  //  3
  { FLD_IMAGE_NUM,       DT_CHARS, META_BYTSZ(18),  1 },  //  4
  { FLD_TRANSACTION_TP,  DT_CHARS, META_BYTSZ(3),   1 },  //  5
  { FLD_ENTITY_TP,       DT_CHARS, META_BYTSZ(3),   1 },  //  6
  { FLD_NAME,            DT_CHARS, META_BYTSZ(200), 1 },  //  7
  { FLD_CITY,            DT_CHARS, META_BYTSZ(30),  1 },  //  8
  { FLD_STAT,            DT_CHARS, META_BYTSZ(2),   1 },  //  9
  { FLD_ZIP_CODE,        DT_CHARS, META_BYTSZ(9),   1 },  // 10
  { FLD_EMPLOYER,        DT_CHARS, META_BYTSZ(38),  1 },  // 11
  { FLD_OCCUPATION,      DT_CHARS, META_BYTSZ(38),  1 },  // 12
  { FLD_TRANSACTION_DT,  DT_DATE,  META_BYTSZ(8),   1 },  // 13
  { FLD_TRANSACTION_AMT, DT_NUM,   META_BYTSZ(14),  1 },  // 14
  { FLD_OTHER_ID,        DT_CHARS, META_BYTSZ(9),   1 },  // 15
  { FLD_TRAN_ID,         DT_CHARS, META_BYTSZ(32),  1 },  // 16
  { FLD_FILE_NUM,        DT_NUM,   META_BYTSZ(22),  1 },  // 17
  { FLD_MEMO_CD,         DT_CHARS, META_BYTSZ(1),   1 },  // 18
  { FLD_MEMO_TEXT,       DT_CHARS, META_BYTSZ(100), 1 },  // 19
  { FLD_SUB_ID,          DT_NUM,   META_BYTSZ(19),  0 }   // 20
};

static int get_numof_metanames(metadata_class_t *obj, int *n_p)
{
  assert(obj != NULL && n_p != NULL);

  *n_p = obj->metadata_tbl_sz;
  return 0;
}

static int get_metadata_elem(metadata_class_t *obj, int metadata_name,
			     int *dtype_p, int *bytesz_p, int *cannull_p)
{
  metadata_info_t *meta_info_p;
  
  assert(obj != NULL);
  assert(metadata_name >= 0 && metadata_name < FLD_TBL_SZ);

  assert(metadata_name < obj->metadata_tbl_sz);
  meta_info_p = &((obj->metadata_tbl_p)[metadata_name]);

  if (dtype_p != NULL)
    *dtype_p = meta_info_p->dtype;

  if (bytesz_p != NULL)
    *bytesz_p = meta_info_p->byte_sz;

  if (cannull_p != NULL)
    *cannull_p = meta_info_p->can_null;

  return 0;
}

static metadata_class_t metadata_fec_contribbyind_obj = {
 meta_spec:           META_FEC_CONTRIBUTION_BY_IND,
 metadata_tbl_sz:     NUM_ELEMS(meta_fec_contribbyind_tbl),
 metadata_tbl_p:      meta_fec_contribbyind_tbl,
 func_init:           NULL,
 func_open:           NULL,
 func_close:          NULL,
 get_numof_metanames: &get_numof_metanames,
 get_metadata_elem:   &get_metadata_elem
};

int get_metadata_obj(int meta_spec, metadata_class_t **metadata_obj_pp)
{
  int err = 0;
  assert(metadata_obj_pp != NULL);
  
  switch (meta_spec) {
  case META_FEC_CONTRIBUTION_BY_IND:
    *metadata_obj_pp = &metadata_fec_contribbyind_obj;
    break;

  default:
    printf("ERR, Cannot find metadata obj(%d) (%s, line=%d)\n",
	   meta_spec, __FILE__, __LINE__);
    err = Err_CANNOT_FIND_OBJ;
    break;
  }

  return err;
}

