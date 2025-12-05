#include "libxml.h"

bool ends_with(const char* haystack, const char* needle) {
    int h_len = strlen(haystack);
    int n_len = strlen(needle);

    if (h_len < n_len) 
        return false;

    for (int i = 0; i < n_len; i++)
        if (haystack[h_len - n_len + i] != needle[i])
            return false;

    return true;
}

void XMLAttribute_free(XMLAttribute_t* attr) {
    free(attr->key);
    free(attr->value);
}

void XMLAttributeList_init(XMLAttributeList_t* list) {
    list->size = 1;
    list->count = 0;
    list->data = (XMLAttribute_t*) malloc(sizeof(XMLAttribute_t) * list->size);
}

void XMLAttributeList_add(XMLAttributeList_t* list, XMLAttribute_t* attr) {
    while (list->count >= list->size) {
        list->size *= 2;
        list->data = (XMLAttribute_t*) realloc(list->data, sizeof(XMLAttribute_t) * list->size); 
    }

    list->data[list->count++] = *attr;
}


void XMLNodeList_init(XMLNodeList_t* list) {
    list->size = 1;
    list->count = 0;
    list->data = (XMLNode_t**) malloc(sizeof(XMLNode_t*) * list->size);
}

void XMLNodeList_add(XMLNodeList_t* list, struct _XMLNode* node) {
    while (list->count >= list->size) {
        list->size *= 2;
        list->data = (XMLNode_t**) realloc(list->data, sizeof(XMLNode_t*) * list->size); 
    }

    list->data[list->count++] = node;
}


XMLNode_t* XMLNode_new(XMLNode_t* parent) {
    XMLNode_t* node = (XMLNode_t*) malloc(sizeof(XMLNode_t));
    node->parent = parent;
    node->tag = NULL;
    node->inner_text = NULL;
    XMLAttributeList_init(&node->attributes);
    XMLNodeList_init(&node->children);
    if (parent)
        XMLNodeList_add(&parent->children, node);
    return node;
}

void XMLNode_free(XMLNode_t* node) {
    if (node->tag)
        free(node->tag);
    if (node->inner_text)
        free(node->inner_text);
    for (int i = 0; i < node->attributes.count; i++)
        XMLAttribute_free(&node->attributes.data[i]);
    for (int i = 0; i < node->children.count; i++)
        XMLNode_free(node->children.data[i]);
    free(node); 
}

XMLNode_t* XMLNode_child(XMLNode_t* parent, int idx) {
    return parent->children.data[idx];
}

char* XMLNode_attr_val(XMLNode_t* node, const char* key) {
    for (int i = 0; i < node->attributes.count; i++) {
        XMLAttribute_t attr = node->attributes.data[i];
        if (!strcmp(attr.key, key))
            return attr.value;
    }
    return NULL;
}


static void parse_attrs(char* buff, int* i, char* lex, int* lexi, XMLNode_t* curr_node) {
    XMLAttribute_t curr_attr = {0, 0};
    while (buff[*i] != '>') {
        lex[(*lexi)++] = buff[(*i)++];

        // get tag name
        if (buff[*i] == ' ' && !curr_node->tag) {
            lex[*lexi] = '\0';
            curr_node->tag = strdup(lex);
            *lexi = 0;
            (*i)++;
            continue;
        }

        // ignore other spaces
        if (lex[*lexi - 1] == ' ') {
            (*lexi)--;
            continue;
        }

        // fetch attribute key
        if (buff[*i] == '=') {
            lex[*lexi] = '\0';
            curr_attr.key = strdup(lex);
            *lexi = 0;
            continue;
        }

        // fetch attribute value
        if (buff[*i] == '"') {
            if (!curr_attr.key) {
                printf("Error - Attribute value has no key!");
                return;
            }

            *lexi = 0;
            (*i)++;

            while (buff[*i] != '"')
                lex[(*lexi)++] = buff[(*i)++];
            lex[*lexi] = '\0';
            curr_attr.value = strdup(lex);
            XMLAttributeList_add(&curr_node->attributes, &curr_attr);
            curr_attr = (XMLAttribute_t){0, 0};
            *lexi = 0;
            (*i)++;
            continue;
        }
    }
}

bool XMLDocument_load(XMLDocument_t* doc, const char* path) {
    FILE* file = fopen(path, "r");
    if (file == 0) {
        printf("Error - '%s' failed to open!\n", path);
        return false;
    }

    fseek(file, 0, SEEK_END);
    int size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* buff = (char*) malloc(sizeof(char) * size + 1);
    fread(buff, 1, size, file);
    fclose(file);
    buff[size] = '\0';

    doc->root = XMLNode_new(NULL);
    char lex[256];
    int lexi = 0, i = 0;
    XMLNode_t* curr_node = doc->root;

    while (buff[i] != '\0') {
        if (buff[i] == '<') {
            lex[lexi] = '\0';

            if (lexi > 0) { // current node has inner text
                if (curr_node == NULL) {
                    printf("Error - Text %s outside of document!\n", lex);
                    return false;
                }

                curr_node->inner_text = strdup(lex);
                lexi = 0;
            }

            // end of node
            if (buff[i + 1] == '/') {
                i += 2;
                while(buff[i] != '>')
                    lex[lexi++] = buff[i++];
                lex[lexi] = '\0';
                
                if (curr_node == NULL) { // text before the root
                    printf("Error - Already at the root!\n");
                    return false;
                }

                if (strcmp(curr_node->tag, lex)) { // missmatching ending tag
                    printf("Error - Missmatched tags between starting tag %s and ending tag %s!\n",
                            curr_node->tag, lex);
                    return false;
                }

                curr_node = curr_node->parent;
                i++;
                continue;
            }

            // comment handling
            if (buff[i + 1] == '!') {
                while (buff[i] != ' ' && buff[i] != '>')
                    lex[lexi++] = buff[i++];
                lex[lexi] = '\0';

                if (strcmp(lex, "<!--") == 0) {
                    lex[lexi] = '\0';
                    while (!ends_with(lex, "-->")) {
                        lex[lexi++] = buff[i++];
                        lex[lexi] = '\0';
                    }
                    // i++;
                    continue;
                }
            }

            // declaration tags
            if (buff[i + 1] == '?') {
                while (buff[i] != ' ' && buff[i] != '>')
                    lex[lexi++] = buff[i++];
                lex[lexi] = '\0';

                if (!strcmp(lex, "<?xml")) {
                    lexi = 0;
                    XMLNode_t* desc = XMLNode_new(NULL);
                    parse_attrs(buff, &i, lex, &lexi, desc);

                    doc->version = XMLNode_attr_val(desc, "version");
                    doc->encoding = XMLNode_attr_val(desc, "encoding");
                    continue;
                }
            }

            // set current node
            curr_node = XMLNode_new(curr_node);

            // start of tag
            i++;
            parse_attrs(buff, &i, lex, &lexi, curr_node);
            
            // set tag name if none
            lex[lexi] = '\0';
            if (!curr_node->tag)
                curr_node->tag = strdup(lex);
            // reset lexer


            lexi = 0;
            i++;
            continue;
        } else {
            lex[lexi++] = buff[i++];
        }
    }

    return true;
}

void XMLDocument_free(XMLDocument_t* doc) {
    XMLNode_free(doc->root);
}

bool XMLDocument_save(XMLDocument_t* doc, const char* path) {

}
