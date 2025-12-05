#include "libxml.h"

int main() {
    XMLDocument_t doc;
    if (XMLDocument_load(&doc, "test.xml")) {
        XMLNode_t node = *doc.root;

        printf("Attributes:\n");
        for (int i = 0; i < node.attributes.count; i++) {
            XMLAttribute_t attr = node.attributes.data[i];
            printf("  %s => \"%s\"\n", attr.key, attr.value);
        }
        XMLDocument_free(&doc);
    }

    return 0;
}