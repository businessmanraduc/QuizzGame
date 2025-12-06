#ifndef LIBXML_H
#define LIBXML_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define LEXER_BUFF_SIZE 4096

//
//  Definitions
//

struct _XMLAttribute
{
    char* key;
    char* value;
};
typedef struct _XMLAttribute XMLAttribute;

void XMLAttribute_free(XMLAttribute* attr);

struct _XMLAttributeList
{
    int heap_size;
    int size;
    XMLAttribute* data;
};
typedef struct _XMLAttributeList XMLAttributeList;

void XMLAttributeList_init(XMLAttributeList* list);
void XMLAttributeList_add(XMLAttributeList* list, XMLAttribute* attr);

struct _XMLNodeList
{
    int heap_size;
    int size;
    struct _XMLNode** data;
};
typedef struct _XMLNodeList XMLNodeList;

void XMLNodeList_init(XMLNodeList* list);
void XMLNodeList_add(XMLNodeList* list, struct _XMLNode* node);
struct _XMLNode* XMLNodeList_at(XMLNodeList* list, int index);
void XMLNodeList_free(XMLNodeList* list);

struct _XMLNode
{
    char* tag;
    char* inner_text;
    struct _XMLNode* parent;
    XMLAttributeList attributes;
    XMLNodeList children;
};
typedef struct _XMLNode XMLNode;

XMLNode* XMLNode_new(XMLNode* parent);
void XMLNode_free(XMLNode* node);
XMLNode* XMLNode_child(XMLNode* parent, int index);
XMLNodeList* XMLNode_children(XMLNode* parent, const char* tag);
char* XMLNode_attr_val(XMLNode* node, char* key);
XMLAttribute* XMLNode_attr(XMLNode* node, char* key);
XMLNode* XMLNode_first_child(XMLNode* parent, const char* tag);
XMLNode* XMLNode_next_sibling(XMLNode* node);
XMLNode* XMLNode_prev_sibling(XMLNode* node);

struct _XMLDocument
{
    XMLNode* root;
    char* version;
    char* encoding;
};
typedef struct _XMLDocument XMLDocument;

enum _XMLError {
    XML_SUCCESS = 0,
    XML_ERROR_FILE,
    XML_ERROR_PARSER,
    XML_ERROR_MEMORY,
    XML_ERROR_INVALID
};
typedef enum _XMLError XMLError;

const char* XMLDocument_etos(XMLError err);
XMLError XMLDocument_load(XMLDocument* doc, const char* path);
bool XMLDocument_write(XMLDocument* doc, const char* path, int indent);
void XMLDocument_free(XMLDocument* doc);


//
//  Macros
//
#define XML_FOREACH_CHILD(parent, child) \
    for (int _i = 0; _i < (parent)->children.size && ((child) = (parent)->children.data[_i]); _i++)

#define XML_FOREACH_ATTR(node, attr) \
    for (int _j = 0; _j < (node)->attributes.size && ((attr) = &(node)->attributes.data[_j]); _j++)


#endif