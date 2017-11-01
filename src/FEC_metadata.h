#ifndef FEC_METADATA_H
#define FEC_METADATA_H

typedef enum {
  META_FEC_CONTRIBUTION_BY_IND = 0,
} METADATA_SPEC;

typedef enum {
  FLD_CMTE_ID = 0,
  FLD_AMNDT_IND,
  FLD_RPT_TP,
  FLD_TRANSACTION_PGI,
  FLD_IMAGE_NUM,
  FLD_TRANSACTION_TP,
  FLD_ENTITY_TP,
  FLD_NAME,
  FLD_CITY,
  FLD_STAT,
  FLD_ZIP_CODE,            // 10
  FLD_EMPLOYER,
  FLD_OCCUPATION,
  FLD_TRANSACTION_DT,      // 13
  FLD_TRANSACTION_AMT,     // 14
  FLD_OTHER_ID,            // 15
  FLD_TRAN_ID,
  FLD_FILE_NUM,
  FLD_MEMO_CD,
  FLD_MEMO_TEXT,
  FLD_SUB_ID,
  FLD_TBL_SZ,
  FLD_TERMINATOR = 999,
} METADATA_NAME;

#define MAX_METADATA_SZ 256

typedef enum {
  DT_CHARS = 0,
  DT_NUM,
  DT_DATE
} META_DTYPE;

typedef struct metadata_info {
  METADATA_NAME name;
  META_DTYPE dtype;
  int byte_sz;         // strng len + 1(NUL char)
  int can_null;        // 0: No, Otherwise Yes
} metadata_info_t;

#define FLD_DELIMITER '|'

typedef struct metadata_class {
  int meta_spec;
  int metadata_tbl_sz;
  metadata_info_t *metadata_tbl_p;
  int (*func_init)(struct metadata_class *obj);
  int (*func_open)(struct metadata_class *obj);
  int (*func_close)(struct metadata_class *obj);
  int (*get_numof_metanames)(struct metadata_class *obj, int *n_p);
  int (*get_metadata_elem)(struct metadata_class *obj, int metadata_name,
			   int *dtype_p, int *bytesz_p, int *cannull_p);
} metadata_class_t;

extern int get_metadata_obj(int meta_spec, metadata_class_t **metadata_obj_pp);

#endif // FEC_METADATA_H
