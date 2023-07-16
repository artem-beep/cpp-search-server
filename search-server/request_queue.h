#pragma once
#include "search_server.h"
#include "document.h"

#include <deque>
using namespace std;
class RequestQueue {
public:
    explicit RequestQueue(const SearchServer& search_server): temp_server_(search_server) {

    }
    
    template <typename DocumentPredicate>
    vector<Document> AddFindRequest(const string& raw_query, DocumentPredicate document_predicate) {
       auto result = temp_server_.FindTopDocuments(raw_query,document_predicate);
       if (result.size() > 0){
        requests_.push_back({raw_query, 1});
       }
       else {
        requests_.push_back({raw_query,0});
       }
       if (requests_.size() > min_in_day_) {
        CutDeque();
    }
    return result;
    }
    
    vector<Document> AddFindRequest(const string& raw_query, DocumentStatus status);

    vector<Document> AddFindRequest(const string& raw_query);

    int GetNoResultRequests() const;
private:
     struct QueryResult {
        string query;
        bool sucsess;
    };
    
    deque<QueryResult> requests_;
    const static int min_in_day_ = 1440;
    const SearchServer& temp_server_;

    void CutDeque ();

};