/************************************************************************/
/*                            metaicon.c                                */
/*                    (c) Nicholai Benalal 2017                         */
/*                       nadir@skydreams.org                            */
/*                                                                      */
/*       This is example code that inserts a MorphOS:icondata node      */
/*                          into and SVG file                           */
/************************************************************************/



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <clib/debug_protos.h>

#include "sxmlc.h"
#include "sxmlsearch.h"

#define LOCPRINT kprintf("%s(%d)\n", __FUNCTION__, __LINE__);

XMLNode *getnode_xp(XMLNode *node, char *xpath, BOOL relsearch)
{
	char *buffer = (char*)malloc((strlen(node->tag) + strlen(xpath)) * sizeof(char) + 1);
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

		XMLSearch *search = (XMLSearch *)malloc(sizeof(XMLSearch));

		if(search)
		{
			memset(search, 0, sizeof(XMLSearch));
			XMLNode *snode;

			if(XMLSearch_init_from_XPath(C2SX(buffer), search))
			{
				if((snode = XMLSearch_next(node, search)) != NULL && !relsearch || (relsearch && snode->father == node))
					retnode = snode;
				XMLSearch_free(search, true);
			}
			free(search);
		}
		free(buffer);
	}

	return retnode;
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

#define XINSERT_TOP 0

static int _insert_node(XMLNode*** children_array, int* len_array, XMLNode* prev, XMLNode* node)
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
			if(pt[i] == prev);
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
		XMLDoc_print(doc, (FILE*)fh, C2SX("\n"), C2SX("    "), false, 0, 4);
		Close(fh);
	}
	else
		printf("failed to save xml document: %s\n", filename);

	return ok;
}


BOOL SetIconContents(XMLNode *morphosicon)
{
	XMLNode *icondata = NULL;

	XMLNode_set_tag(morphosicon, C2SX("MorphOS:icondata"));
	XMLNode_set_attribute(morphosicon, C2SX("xmlns"), "http://www.morphos-team.net/morphos-icon-metadata/v1");
	XMLNode_set_type(morphosicon, XTAG_FATHER);

	icondata = XMLNode_alloc();
	XMLNode_set_tag(icondata, C2SX("stack"));
	XMLNode_set_type(icondata, XTAG_FATHER);
	XMLNode_set_attribute(icondata, C2SX("size"), "32768");
	XMLNode_add_child(morphosicon, icondata);

	icondata = XMLNode_alloc();
	XMLNode_set_tag(icondata, C2SX("protection"));
	XMLNode_set_type(icondata, XTAG_FATHER);
	XMLNode_set_attribute(icondata, C2SX("bits"), "----rwed");
	XMLNode_add_child(morphosicon, icondata);

	icondata = XMLNode_alloc();
	XMLNode_set_tag(icondata, C2SX("comment"));
	XMLNode_set_type(icondata, XTAG_FATHER);
	XMLNode_set_attribute(icondata, C2SX("text"), "This is a comment");
	XMLNode_add_child(morphosicon, icondata);

	for(int i = 0; i < 5; i++)
	{
		char text[8];
		sprintf(text, "(TT%d)", i);
		icondata = XMLNode_alloc();
		XMLNode_set_tag(icondata, C2SX("tooltype"));
		XMLNode_set_type(icondata, XTAG_FATHER);
		XMLNode_set_attribute(icondata, C2SX("name"), text);
		XMLNode_add_child(morphosicon, icondata);
	}

	return 0;
}

#if 1 //Unbuffered version
int main(int argc, char **argv)
{

	XMLDoc *svgdoc = NULL;
	char *inputfile, *outputfile;
	int ret = EXIT_FAILURE;

	if(argc != 3)
	{
		fprintf(stderr, "Error: incorrect syntax\nmetaicon input output\n");

		return EXIT_FAILURE;
	}

	inputfile = argv[1];
	outputfile = argv[2];

	svgdoc = (XMLDoc*)AllocMem(sizeof(XMLDoc), MEMF_PUBLIC);

	if(svgdoc)
	{
		XMLDoc_init(svgdoc);

		if(XMLDoc_parse_file_DOM(inputfile, svgdoc))
		{
			printf("loaded the SVG file\n");
			
			XMLNode *svgnode = XMLDoc_root(svgdoc);
			if(svgnode)
			{
				XMLNode *metadata = getnode_xp(svgnode, "svg/metadata", FALSE);

				XMLNode *morphosicon = NULL;

				if(strcmp(svgnode->tag, "svg") == 0)
				{
					if(metadata)
					{
						printf("found metadata node\n");
						morphosicon = getnode_xp(metadata, "MorphOS:icondata", FALSE);
						if(morphosicon)
						{
							int nodenr = getnodenum(metadata, morphosicon);
							XMLNode *snode;
							XMLSearch *search = (XMLSearch *)AllocMem(sizeof(XMLSearch), MEMF_PUBLIC);
							char *stack, *protection, *comment, *blah;
							
							printf("found morphos icon node at index %d\n", nodenr);
							//use the node information
							
							if((snode=getnode_xp(morphosicon, "/stack", TRUE)))
								stack = getstr_xattr(snode, "size");

							if((snode=getnode_xp(morphosicon, "/protection", TRUE)))
								protection = getstr_xattr(snode, "bits");
								
							if((snode=getnode_xp(morphosicon, "/comment", TRUE)))
								comment = getstr_xattr(snode, "text");

							if((snode=getnode_xp(morphosicon, "/blah", TRUE)))
								blah = getstr_xattr(snode, "blah");
								
							if(search)
							{
								memset(search, 0, sizeof(XMLSearch));
								if (XMLSearch_init_from_XPath(C2SX("tooltype"), search))
								{
									XMLNode *snode = morphosicon;
									while((snode = XMLSearch_next(snode, search)) != NULL && snode->father == morphosicon)
									{
										char *tooltype = getstr_xattr(snode, "name");
										if(tooltype)
											printf("tooltype=%s\n", tooltype);
									}
									
									XMLSearch_free(search, true);
								}
								FreeMem(search, sizeof(XMLSearch));
							}
								
							XMLNode_remove_child(metadata, nodenr, true);
						}
						else
						{
							printf("did not find a morphos icon node\n");
						}
					}
					else
					{
						printf("did not find a metadata node, creating one\n");
						metadata = XMLNode_alloc();
						XMLNode_set_tag(metadata, C2SX("metadata"));
						XMLNode_set_type(metadata, XTAG_FATHER);
						//XMLNode_set_attribute(morphosicon, C2SX("name"), C2SX(str));
						XMLNode_insert_child(svgnode, XINSERT_TOP, metadata);
					}

					printf("creating a new morphos icon node\n");
					morphosicon = XMLNode_alloc();

					SetIconContents(morphosicon);

					XMLNode_add_child(metadata, morphosicon);
					
					save_xmldoc(svgdoc, outputfile);

					ret = EXIT_SUCCESS;
				}
				else
					printf("this does not appear to be an SVG file\n");
			}
			else
				printf("Could not get the the root node for the document\n");
		}
		else
			printf("failed to parse the SVG file\n");

		XMLDoc_free(svgdoc);
		FreeMem(svgdoc, sizeof(XMLDoc));
	}

	return ret;
}

#else //Buffered version which loads the whole SVG to memory before parsing

int main(int argc, char **argv)
{

	XMLDoc *svgdoc = NULL;
	BPTR lock;
	char *inputfile, *outputfile;
	int ret = EXIT_FAILURE;

	if(argc != 3)
	{
		fprintf(stderr, "Error: incorrect syntax\nmetaicon input output\n");

		return EXIT_FAILURE;
	}

	inputfile = argv[1];
	outputfile = argv[2];

	lock = Open(inputfile, MODE_OLDFILE);

	if(lock == 0)
	{
		printf("Could not get a lock on '%s'\n", inputfile);

		return EXIT_FAILURE;
	}

	svgdoc = (XMLDoc*)AllocMem(sizeof(XMLDoc), MEMF_PUBLIC);

	if(svgdoc)
	{
		XMLDoc_init(svgdoc);
		struct FileInfoBlock *fib = (struct FileInfoBlock *)AllocDosObject(DOS_FIB, NULL);

		if(fib && ExamineFH(lock, fib))
		{
			char *fdata = fib->fib_Size > 0 ? AllocMem(fib->fib_Size + 1, MEMF_PUBLIC) : NULL;

			if(fdata && (Read(lock, fdata, fib->fib_Size) == fib->fib_Size))
			{

				fdata[fib->fib_Size] = '\0';
				if(XMLDoc_parse_buffer_DOM(fdata, inputfile, svgdoc))
				{
					printf("loaded the SVG file\n");
					if(fdata) FreeMem(fdata, fib->fib_Size + 1);

					XMLNode *svgnode = XMLDoc_root(svgdoc);
					XMLNode *metadata = getnode_xp(XMLDoc_root(svgdoc), "svg/metadata", FALSE);
					XMLNode *morphosicon = NULL;

					if(svgnode && strcmp(svgnode->tag, "svg") == 0)
					{
						if(metadata)
						{
							printf("found metadata node\n");
							morphosicon = getnode_xp(metadata, "MorphOS:icondata", FALSE);
							if(morphosicon)
							{
								int nodenr = getnodenum(metadata, morphosicon);
								printf("found morphos icon node at index %d\n", nodenr);
								//use the node information
								XMLNode_remove_child(metadata, nodenr, true);
							}
							else
							{
								printf("did not find a morphos icon node\n");
							}
						}
						else
						{
							printf("did not find a metadata node, creating one\n");
							metadata = XMLNode_alloc();
							XMLNode_set_tag(metadata, C2SX("metadata"));
							XMLNode_set_type(metadata, XTAG_FATHER);
							//XMLNode_set_attribute(morphosicon, C2SX("name"), C2SX(str));
							XMLNode_insert_child(svgnode, XINSERT_TOP, metadata);
						}

						printf("creating a new morphos icon node\n");
						morphosicon = XMLNode_alloc();

						SetIconContents(morphosicon);

						XMLNode_add_child(metadata, morphosicon);
						
						save_xmldoc(svgdoc, outputfile);

						ret = EXIT_SUCCESS;
					}
					else
						printf("this does not appear to be an SVG file\n");
				}
			}
		}
		else
		{
			printf("Could not Examine '%s'\n", inputfile);
		}

		XMLDoc_free(svgdoc);
		FreeMem(svgdoc, sizeof(XMLDoc));
		if(fib) FreeDosObject(DOS_FIB, fib);
	}
	else
	{
		fprintf(stderr, "Failed to allocate memory for xml document\n");
	}

	Close(lock);

	return ret;
}
#endif