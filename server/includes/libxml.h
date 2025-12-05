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
typedef struct _XMLAttributeList XMLAttributeList; 

void XMLAttributeList_init(XMLAttributeList*);
void XMLAttributeList_add(XMLAttributeList*, XMLAttribute_t*);

struct _XMLNode {
    char* tag;
    char* inner_text;
    struct _XMLNode* parent;
    XMLAttributeList attributes;
};
typedef struct _XMLNode XMLNode_t;

XMLNode_t* XMLNode_new(XMLNode_t*);
XMLNode_t* XMLNode_free(XMLNode_t*);

struct _XMLDocument {
    XMLNode_t* root;
};
typedef struct _XMLDocument XMLDocument_t;

bool XMLDocument_load(XMLDocument_t*, const char*);
void XMLDocument_free(XMLDocument_t*);
bool XMLDocument_save(XMLDocument_t*, const char*);

#endif