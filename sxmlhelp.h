

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <clib/debug_protos.h>

#include "sxmlc.h"
#include "sxmlsearch.h"

#define LOCPRINT kprintf("%s(%d)\n", __FUNCTION__, __LINE__);

#define XINSERT_TOP 0

XMLNode *getnode_xp(XMLNode *node, char *xpath, BOOL relsearch);
int getnodenum(XMLNode *basenode, XMLNode *node);
char *getstr_xattr(XMLNode* node, char *attr);
int _insert_node(XMLNode*** children_array, int* len_array, XMLNode* prev, XMLNode* node);
int XMLNode_insert_child(XMLNode* node, XMLNode *prev, XMLNode* child);
BOOL save_xmldoc(XMLDoc *doc, const char *filename);
int getval_xp(XMLNode* node, char *xpath, int *val, BOOL relsearch);
char *getstr_xp(XMLNode* node, char *xpath, BOOL relsearch);

