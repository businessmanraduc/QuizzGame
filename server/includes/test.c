#include "libxml.h"

int main() {
    XMLDocument_t doc;
    if (XMLDocument_load(&doc, "test.xml")) {
        printf("XML Document (version=%s, encoding=%s)\n", doc.version, doc.encoding);
        XMLDocument_free(&doc);
    }

    return 0;
}