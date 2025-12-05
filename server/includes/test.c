#include "libxml.h"

int main()
{
    XMLDocument doc;
    if (XMLDocument_load(&doc, "test.xml") == XML_SUCCESS) {
        // XMLNode* str = XMLNode_child(doc.root, 0);

        // XMLNodeList* fields = XMLNode_children(str, "field");
        // for (int i = 0; i < fields->size; i++) {
        //     XMLNode* field = XMLNodeList_at(fields, i);
        //     XMLAttribute* type = XMLNode_attr(field, "type");
        //     // type->value = strdup("");
        // }

        XMLNode* parent = XMLNode_child(doc.root, 0);
        XMLNode* node;
        XML_FOREACH_CHILD(parent, node) {
            printf("Child tag: %s\n", node->tag);
        }

        XMLDocument_write(&doc, "out.xml", 4);
        XMLDocument_free(&doc);
    } else {
        printf("didn't work?\n");
    }

    return 0;
}