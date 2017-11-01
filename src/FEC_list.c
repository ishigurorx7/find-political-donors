/******************************************************************************
 * FEC_list.c
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
#include "FEC_list.h"

static int func_init(list_class_t *obj)
{
  assert(obj != NULL);
  obj->n_nodes = 0;
  obj->tail_p = NULL;
  obj->head_p = NULL;
  return 0;
}

static int func_get_head(list_class_t *obj, node_data_t **head_pp, int *n_nodes_p)
{
  assert(obj != NULL && head_pp != NULL);

  *head_pp = obj->head_p;

  if (n_nodes_p != NULL) {
    *n_nodes_p = obj->n_nodes;
  }

  return 0;
}

static int func_put_node(list_class_t *obj, char *key_p, void *targ_node_p)
{
  int err = 0;
  node_data_t *node_p, tmp_p;
  assert(obj != NULL && key_p != NULL);

  if ((node_p = (node_data_t *)malloc(sizeof(node_data_t))) == NULL) {
    err = Err_MALLOC;
    printf("ERR, Malloc failed. (%s, line=%d)\n", __FILE__, __LINE__);
    return err;
  }

  strcpy(node_p->key, key_p);
  node_p->next = NULL;

  if (targ_node_p != NULL) 
    node_p->targp = targ_node_p;
  else node_p->targp = NULL;
  
  if (obj->head_p == NULL) {
    obj->head_p = node_p;
    obj->tail_p = node_p;
  } else {
    obj->tail_p->next = node_p;
    obj->tail_p = node_p;
  }
  obj->n_nodes++;

  return err;
}

static int func_close(list_class_t *obj)
{
  int err = 0;
  node_data_t *np, *tmp_p;
  assert (obj != NULL);

  if (obj->n_nodes == 0 || obj->head_p == NULL) {
    return err;
  }

  np = obj->head_p;

  while (np != NULL) {
    tmp_p = np->next;
    free(np);
    np = tmp_p;
  }
  
  obj->head_p = NULL;
  obj->tail_p = NULL;
  obj->n_nodes = 0;

  return err;
}

static list_class_t key_acc_list_obj = {
 meta_spec: META_FEC_CONTRIBUTION_BY_IND,
 n_nodes: 0,
 tail_p: NULL,
 head_p: NULL,
 func_init:     &func_init,
 func_put_node: &func_put_node,
 func_get_head: &func_get_head,
 func_close:    &func_close,
};

int get_list_obj(int meta_spec, list_class_t **list_obj_pp)
{
  int err = 0;
  assert(list_obj_pp != NULL);

  switch (meta_spec) {
  case META_FEC_CONTRIBUTION_BY_IND:
    *list_obj_pp = &key_acc_list_obj;
    break;

  default:
    printf("ERR, Cannot find meta spec(%d) (%s, line=%d)\n",
	   meta_spec, __FILE__, __LINE__);
    err = Err_CANNOT_FIND_OBJ;
    break;
  }

  return err;

}
