#include "remove_duplicates.h"
using namespace std;
void RemoveDuplicates(SearchServer& search_server) {
    set<set<string>> unique_docs;
    vector<int> ids_of_dubl;
    for (const int memb : search_server) {
        map<string,double> doc_data = search_server.GetWordFrequencies(memb);
        if (doc_data.empty()) {
            continue;
        }

        set<string>words_of_doc;
        for (auto memb : doc_data) {
            words_of_doc.insert(memb.first);
        }

        if (unique_docs.count(words_of_doc) > 0) {
            ids_of_dubl.push_back(memb);
        }
        else {
            unique_docs.insert(words_of_doc);
        }
    }

    for (auto id : ids_of_dubl) {
        search_server.RemoveDocument(id);
        cout << "Found duplicate document id" << id << endl;
    }

}