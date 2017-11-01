#ifndef FEC_LIST_H
#define FEC_LIST_H

#define MAX_KEY_SZ  64

typedef struct node_data {
  char key[MAX_KEY_SZ];
  void *targp;
  struct node_data *next;
} node_data_t;

typedef struct list_class {
  int meta_spec;
  int n_nodes;
  node_data_t *tail_p;
  node_data_t *head_p;
  int (*func_init)(struct list_class *obj);
  int (*func_put_node)(struct list_class *obj, char *key_p, void *targ_node_p);
  int (*func_get_head)(struct list_class *obj, node_data_t **head_pp, int *n_nodes_p);
  int (*func_close)(struct list_class *obj);
} list_class_t;

extern int get_list_obj(int meta_spec, list_class_t **list_obj_pp);

#endif // FEC_LIST_H
