#include "document.h"


ostream& operator<<(ostream& out, Document doc) {
    out << "document_id = "s << doc.id << ", " << "relevance = "s << doc.relevance << ", " << "rating = "s << doc.rating;
    return out;
}