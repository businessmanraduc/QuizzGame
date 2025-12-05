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
    if (attr->value)
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

struct _XMLNode* XMLNodeList_at(XMLNodeList_t* list, int idx) {
    if (list->count <= idx) {
        printf("Error - Accessing node outside of list range!\n");
        return NULL;
    }
        
    return list->data[idx];
}

void XMLNodeList_free(XMLNodeList_t* list) {
    if (list) {
        free(list->data);
        free(list);
    }
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

XMLNodeList_t* XMLNode_children(XMLNode_t* parent, const char* tag) {
    XMLNodeList_t* list = (XMLNodeList_t*) malloc(sizeof(XMLNodeList_t));
    XMLNodeList_init(list);
    for (int i = 0; i < parent->children.count; i++) {
        XMLNode_t* child = parent->children.data[i];
        if (!strcmp(child->tag, tag))
            XMLNodeList_add(list, child);
    }
    return list;
}

char* XMLNode_attr_val(XMLNode_t* node, const char* key) {
    for (int i = 0; i < node->attributes.count; i++) {
        XMLAttribute_t attr = node->attributes.data[i];
        if (!strcmp(attr.key, key))
            return attr.value;
    }
    return NULL;
}

XMLAttribute_t* XMLNode_attr(XMLNode_t* node, char* key) {
    for (int i = 0; i < node->attributes.count; i++) {
        XMLAttribute_t* attr = &node->attributes.data[i];
        if (!strcmp(attr->key, key))
            return attr;
    }
    return NULL;
}


XMLNodeIterator_t XMLNode_iterate_children(XMLNode_t* parent, const char* tag) {
    XMLNodeIterator_t iter = {parent, tag, 0};
    return iter;
}

XMLNode_t* XMLNodeIterator_next(XMLNodeIterator_t* iter) {
    while (iter->current_idx < iter->parent->children.count) {
        XMLNode_t* child = iter->parent->children.data[iter->current_idx++];
        if (iter->tag == NULL || !strcmp(child->tag, iter->tag))
            return child;
    }

    return NULL;
}


enum _TagType {
    TAG_START,
    TAG_INLINE
};
typedef enum _TagType TagType;

static int parse_attrs(char* buff, int* i, char* lex, int* lexi, XMLNode_t* curr_node) {
    printf("DEBUG parse_attrs start: i=%d, buff[i]=%c\n", *i, buff[*i]);
    XMLAttribute_t curr_attr = {0, 0};
    while (buff[*i] != '>') {
        printf("DEBUG: i=%d, char='%c' (0x%02x), lexi=%d\n", *i, buff[*i], buff[*i], *lexi);
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
                return false;
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

        // inline mode
        if (buff[*i - 1] == '/' && buff[*i] == '>') {
            printf("DEBUG: Found self-closing tag at i=%d\n", *i);
            lex[*lexi] = '\0';

            if (*lexi > 0 && lex[*lexi - 1] == '/') {
                lex[*lexi - 1] = '\0';
                (*lexi)--;
            }

            if (!curr_node->tag)
                curr_node->tag = strdup(lex);
            (*i)++;
            printf("DEBUG: Self-closing tag done, i=%d, buff[i]=%c\n", *i, buff[*i]);
            *lexi = 0;
            return TAG_INLINE;
        }
    }

    printf("DEBUG parse_attrs end: i=%d, buff[i]=%c\n", *i, buff[*i]);
    *lexi = 0;
    return TAG_START;
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
                    free(buff);
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
                    printf("Error - Closing tag %s without opening tag!\n", lex);
                    free(buff);
                    return false;
                }

                if (strcmp(curr_node->tag, lex)) { // missmatching ending tag
                    printf("Error - Missmatched tags between starting tag %s and ending tag %s!\n",
                            curr_node->tag, lex);
                    free(buff);
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

                    char* version = XMLNode_attr_val(desc, "version");
                    char* encoding = XMLNode_attr_val(desc, "encoding");
                    
                    doc->version = version ? strdup(version) : NULL;
                    doc->encoding = encoding ? strdup(encoding) : NULL;

                    XMLNode_free(desc);
                    continue;
                }
            }

            // set current node
            curr_node = XMLNode_new(curr_node);

            // start of tag
            i++;
            if (parse_attrs(buff, &i, lex, &lexi, curr_node) == TAG_INLINE) {
                curr_node = curr_node->parent;
                i++;
                continue;
            }
            
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
    if (doc->version) free(doc->version);
    if (doc->encoding) free(doc->encoding);
}

static char* escape_xml(const char* text) {
    if (!text)
        return NULL;
    
    int len = strlen(text);
    int escaped_len = 0;
    for (int i = 0; i < len; i++) {
        switch (text[i]) {
            case '&': escaped_len += 5; break;
            case '<': escaped_len += 4; break;
            case '>': escaped_len += 4; break;
            case '"': escaped_len += 6; break;
            case '\'': escaped_len += 6; break;
            default: escaped_len += 1; break;
        }
    }

    char* escaped = (char*)malloc(escaped_len + 1);
    if (!escaped)
        return NULL;

    int pos = 0;
    for (int i = 0; i < len; i++) {
        switch (text[i]) {
            case '&':
                strcpy(escaped + pos, "&amp;");
                pos += 5;
                break;
            case '<':
                strcpy(escaped + pos, "&lt;");
                pos += 4;
                break;
            case '>':
                strcpy(escaped + pos, "&gt;");
                pos += 4;
                break;
            case '"':
                strcpy(escaped + pos, "&quot;");
                pos += 6;
                break;
            case '\'':
                strcpy(escaped + pos, "&apos;");
                pos += 6;
                break;
            default:
                if (isprint((unsigned char)text[i]) ||
                    text[i] == '\n' || text[i] == '\t' || text[i] == '\r') {
                        escaped[pos++] = text[i];
                } else {
                        escaped[pos++] = '?';
                }
                break;
        }
    }
    
    escaped[pos] = '\0';
    return escaped;
}

static void save_node(FILE* file, XMLNode_t* node, int depth) {
    if (!node)
        return;

    if (!node->tag) {
        for (int i = 0; i < node->children.count; i++) {
            save_node(file, node->children.data[i], depth);
        }
        return;
    }

    for (int i = 0; i < depth; i++) {
        fprintf(file, "  ");
    }

    // Opening tag
    fprintf(file, "<%s", node->tag);

    // Attributes
    for (int i = 0; i < node->attributes.count; i++) {
        XMLAttribute_t* attr = &node->attributes.data[i];
        if (attr->key && attr->value) {
            char* escaped_value = escape_xml(attr->value);
            if (escaped_value) {
                fprintf(file, " %s=\"%s\"", attr->key, escaped_value);
                free(escaped_value);
            }
        }
    }

    // empty element
    if (node->children.count == 0 && (!node->inner_text || node->inner_text[0] == '\0')) {
        fprintf(file, " />\n");
        return;
    }

    fprintf(file, ">");

    // inner text
    if (node->inner_text && node->inner_text[0] != '\0') {
        char* escaped_text = escape_xml(node->inner_text);
        if (escaped_text) {
            fprintf(file, "%s", escaped_text);
            free(escaped_text);
        }
    }

    // children
    if (node->children.count > 0) {
        fprintf(file, "\n");
        for (int i = 0; i < node->children.count; i++) {
            save_node(file, node->children.data[i], depth + 1);
        }

        for (int i = 0; i < depth; i++)
            fprintf(file, "  ");
    }

    fprintf(file, "</%s>\n", node->tag);
}

bool XMLDocument_save(XMLDocument_t* doc, const char* path) {
    if (!doc || !path) return false;

    FILE* file = fopen(path, "w");
    if (!file) {
        printf("Error: Cannot open file '%s' for writing\n", path);
        return false;
    }

    fprintf(file, "<?xml version=\"%s\" encoding=\"%s\"?>\n",
            doc->version ? doc->version : "1.0",
            doc->encoding ? doc->encoding : "UTF-8");

    if (doc->root)
        save_node(file, doc->root, 0);

    fclose(file);
    return true;
}

static void node_out(FILE* file, XMLNode_t* node, int indent, int times) {
    for (int i = 0; i < node->children.count; i++) {
        XMLNode_t* child = node->children.data[i];
        if (times > 0)
            for (int i = 0; i < times * indent; i++)
                fprintf(file, " ");
        fprintf(file, "<%s", child->tag);
        for (int i = 0; i < child->attributes.size; i++) {
            XMLAttribute_t attr = child->attributes.data[i];
            if (!attr.value || !strcmp(attr.value, ""))
                continue;
            fprintf(file, " %s=\"%s\"", attr.key, attr.value);
        }
        if (child->children.size == 0 && !child->inner_text)
            fprintf(file, " />\n");
        else {
            fprintf(file, ">");
            if (child->children.size == 0)
                fprintf(file, "%s</%s>", child->inner_text, child->tag);
            else {
                node_out(file, child, indent, times + 1);
                if (times > 0)
                    for (int i = 0; i < times * indent; i++)
                        fprintf(file, " ");
                fprintf(file, "</%s>\n", child->tag);
            }
        }
    }
}

void XMLDocument_write(XMLDocument_t* doc, const char* path, int indent) {
    FILE* file = fopen(path, "w");
    if (!file) {
        printf("Error - could not open file at path %s\n", path);
        return;
    }

    fprintf(file, "<?xml version=\"%s\" encoding=\"%s\" ?>\n",
            (doc->version) ? doc->version : "1.0",
            (doc->encoding) ? doc->encoding : "UTF-8");

    node_out(file, doc->root, indent, 0);
    fclose(file);
}