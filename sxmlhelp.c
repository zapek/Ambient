

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <clib/debug_protos.h>

#include "sxmlc.h"
#include "sxmlsearch.h"
#include "sxmlhelp.h"
#include <proto/exec.h>

#define LOCPRINT kprintf("%s(%d)\n", __FUNCTION__, __LINE__);

static int _attribute_matches(XMLAttribute* to_test, XMLAttribute* pattern)
{
	if (to_test == NULL && pattern == NULL)
		return true;

	if (to_test == NULL || pattern == NULL)
		return false;
	
	/* No test on name => match */
	if (pattern->name == NULL || pattern->name[0] == NULC)
		return true;

	/* Test on name fails => no match */
	if (!regstrcmp(to_test->name, pattern->name))
		return false;

	/* No test on value => match */
	if (pattern->value == NULL)
		return true;

	/* Test on value according to pattern "equal" attribute */
	return regstrcmp(to_test->value, pattern->value) == pattern->active ? true : false;
}

int getval_xp(XMLNode* node, char *xpath, int *val, BOOL relsearch)
{
	BOOL ret = FALSE;

	char *buffer = (char*)AllocVec((strlen(node->tag) + strlen(xpath)) * sizeof(char) + 1, MEMF_PUBLIC);

	if(buffer)
	{
		if(relsearch)
		{
			strcpy(buffer, node->tag);
			strcat(buffer, xpath);
		}
		else
			strcpy(buffer, xpath);

		XMLSearch *search = (XMLSearch *)AllocVec(sizeof(XMLSearch), MEMF_PUBLIC);

		if(search)
		{
			memset(search, 0, sizeof(XMLSearch));
			XMLNode *snode;
			char *str = NULL;

			if(XMLSearch_init_from_XPath(C2SX(buffer), search))
			{
				if((snode = XMLSearch_next(node, search)) != NULL && (!relsearch || (relsearch && snode->father == node)))
				{
					int i, j;

					/* Check attributes */
					for(; search->next != NULL; search = search->next) ;

					if(search->attributes != NULL)
					{
						for(i = 0; i < search->n_attributes; i++)
						{
							for(j = 0; j < snode->n_attributes; j++)
							{
								if(!snode->attributes[j].active)
									continue;
								if(_attribute_matches(&snode->attributes[j], &search->attributes[i]))
								{
									str = snode->attributes[j].value;
									break;
								}
							}
							if(j >= snode->n_attributes)  /* All attributes where scanned without a successful match */
								break;
						}
					}

					if(str)
					{
						*val = atoi(str);
						ret = 1;
					}
				}
				XMLSearch_free(search, TRUE);
			}
			FreeVec(search);
		}
		FreeVec(buffer);
	}

	return ret;
}

XMLNode *getnode_xp(XMLNode *node, char *xpath, BOOL relsearch)
{
	char *buffer = (char*)AllocVec((strlen(node->tag) + strlen(xpath)) * sizeof(char) + 1, MEMF_PUBLIC);
	XMLNode *retnode = NULL;

	if(buffer)
	{
		if(relsearch)
		{
			strcpy(buffer, node->tag);
			strcat(buffer, xpath);
		}
		else
			strcpy(buffer, xpath);

		XMLSearch *search = (XMLSearch *)AllocVec(sizeof(XMLSearch), MEMF_PUBLIC);

		if(search)
		{
			memset(search, 0, sizeof(XMLSearch));
			XMLNode *snode;

			if(XMLSearch_init_from_XPath(C2SX(buffer), search))
			{
				if((snode = XMLSearch_next(node, search)) != NULL && (!relsearch || (relsearch && snode->father == node)))
					retnode = snode;
				XMLSearch_free(search, true);
			}
			FreeVec(search);
		}
		FreeVec(buffer);
	}

	return retnode;
}

char *getstr_xp(XMLNode* node, char *xpath, BOOL relsearch)
{
	char *str = NULL;
	
	char *buffer = (char*)AllocVec((strlen(node->tag)+strlen(xpath))*sizeof(char) + 1, MEMF_PUBLIC);
	
	if(buffer)
	{
		if(relsearch)
		{
			strcpy(buffer, node->tag);
			strcat(buffer, xpath);
		}
		else
			strcpy(buffer, xpath);
			
		XMLSearch *search = (XMLSearch *)AllocVec(sizeof(XMLSearch), MEMF_PUBLIC);
		
		if(search)
		{
			memset(search, 0, sizeof(XMLSearch));
			XMLNode *snode;
			
			if (XMLSearch_init_from_XPath(C2SX(buffer), search))
			{		
				if((snode = XMLSearch_next(node, search)) != NULL && (!relsearch || (relsearch && snode->father == node)))
				{
					int i, j;

					for(; search->next != NULL; search = search->next) ;

					if(search->attributes != NULL)
					{
						for(i = 0; i < search->n_attributes; i++)
						{
							for(j = 0; j < snode->n_attributes; j++)
							{
								if(!snode->attributes[j].active)
									continue;
								if(_attribute_matches(&snode->attributes[j], &search->attributes[i]))
								{
									str = snode->attributes[j].value;
									break;
								}
							}
							if(j >= snode->n_attributes)  /* All attributes where scanned without a successful match */
								break;
						}
					}
				}

				XMLSearch_free(search, true);
			}
			FreeVec(search);
		}
		FreeVec(buffer);
	}

	return str;
}

int getnodenum(XMLNode *basenode, XMLNode *node)
{
	int ret = -1;
	int i;

	for(i = 0; i < basenode->n_children; i++)
	{
		if(basenode->children[i] == node)
		{
			ret = i;
			break;
		}
	}

	return ret;
}

char *getstr_xattr(XMLNode* node, char *attr)
{
	char *str = NULL;
	int i;
	
	for (i = 0; i < node->n_attributes; i++) 
	{
		if(strcmp(node->attributes[i].name, attr) == 0)
		{
			str = node->attributes[i].value;
			break;
		}
	}

	return str;
}

int _insert_node(XMLNode*** children_array, int* len_array, XMLNode* prev, XMLNode* node)
{
	XMLNode** pt = (XMLNode**)__realloc(*children_array, (*len_array+1) * sizeof(XMLNode*));
	
	if (pt == NULL)
		return -1;
		
	int i;
	
	if(prev == XINSERT_TOP)
		i = 0;
	else
	{		
		for(i = 0; i < *len_array; i++)
		{
			if(pt[i] == prev)
				break;
		}
	}
		
	for(int j = *len_array; j >= i; j--)
	{
		pt[j+1] = pt[j];
	}
		
	pt[i] = node;
	*children_array = pt;
	
	return (*len_array)++;
}

int XMLNode_insert_child(XMLNode* node, XMLNode *prev, XMLNode* child)
{
	if (node == NULL || child == NULL || node->init_value != XML_INIT_DONE || child->init_value != XML_INIT_DONE || (prev != XINSERT_TOP && prev->init_value != XML_INIT_DONE))
		return false;
	
	if (_insert_node(&node->children, &node->n_children, prev, child) >= 0) {
		node->tag_type = XTAG_FATHER;
		child->father = node;
		return true;
	} else
		return false;
}

BOOL save_xmldoc(XMLDoc *doc, const char *filename)
{
	BOOL ok = FALSE;
	
	BPTR fh = Open(filename, MODE_NEWFILE);

	if(fh)
	{
		ok = XMLDoc_print(doc, (FILE*)fh, C2SX("\n"), C2SX("    "), false, 0, 4) ? TRUE : FALSE;
		Close(fh);
	}
	else
		kprintf("failed to save xml document: %s\n", filename);

	return ok;
}
