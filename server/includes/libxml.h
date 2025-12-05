#ifndef LIBXML_H
#define LIBXML_H
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

struct _XMLAttribute {
    char* key;
    char* value;
};
typedef struct _XMLAttribute XMLAttribute_t;

void XMLAttribute_free(XMLAttribute_t*);

struct _XMLAttributeList {
    int size; // size of attribute list on the heap
    int count; // currently allocated attributes
    XMLAttribute_t* data;
};
typedef struct _XMLAttributeList XMLAttributeList_t; 

void XMLAttributeList_init(XMLAttributeList_t*);
void XMLAttributeList_add(XMLAttributeList_t*, XMLAttribute_t*);

struct _XMLNodeList {
    int size;
    int count;
    struct _XMLNode** data;
};
typedef struct _XMLNodeList XMLNodeList_t;

void XMLNodeList_init(XMLNodeList_t*);
void XMLNodeList_add(XMLNodeList_t*, struct _XMLNode*);

struct _XMLNode {
    char* tag;
    char* inner_text;
    struct _XMLNode* parent;
    XMLAttributeList_t attributes;
    XMLNodeList_t children;
};
typedef struct _XMLNode XMLNode_t;

XMLNode_t* XMLNode_new(XMLNode_t*);
void XMLNode_free(XMLNode_t*);
XMLNode_t* XMLNode_child(XMLNode_t*, int);
char* XMLNode_attr_val(XMLNode_t* node, const char* key);

struct _XMLDocument {
    XMLNode_t* root;
    char* version;
    char* encoding;
};
typedef struct _XMLDocument XMLDocument_t;

bool XMLDocument_load(XMLDocument_t*, const char*);
void XMLDocument_free(XMLDocument_t*);
bool XMLDocument_save(XMLDocument_t*, const char*);

#endif