#pragma once
#include "document.h"
#include <algorithm>
#include "read_input_functions.h"
#include "string_processing.h"
#include "log_duration.h"
#include <map>
#include <set>
#include <string>
#include <vector>
#include <stdexcept>
#include <execution>
#include <deque>
#include "concurrent_map.h"
const int MAX_RESULT_DOCUMENT_COUNT = 5;

const auto DIFF = 1e-6;

class SearchServer
{
public:
    template <typename StringContainer>
    explicit SearchServer(const StringContainer &stop_words);

    explicit SearchServer(const std::string &stop_words_text);

    explicit SearchServer(const std::set<std::string> &stop_words);

    explicit SearchServer(const std::string_view stop_words);

    std::set<int>::iterator begin();

    std::set<int>::iterator end();

    const std::map<std::string_view, double> &GetWordFrequencies(int document_id) const;

    // void DeleteDoc(std::string* word);
    void RemoveDocument(int document_id);
    void RemoveDocument(std::execution::sequenced_policy policy, int document_id);
    void RemoveDocument(std::execution::parallel_policy policy, int document_id);

    void AddDocument(int document_id, const std::string_view document, DocumentStatus status,
                     const std::vector<int> &ratings);

    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(std::execution::sequenced_policy policy, const std::string_view raw_query,
                                           DocumentPredicate document_predicate) const;

    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(std::execution::parallel_policy policy, const std::string_view raw_query,
                                           DocumentPredicate document_predicate) const;

    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(const std::string_view raw_query,
                                           DocumentPredicate document_predicate) const;

    std::vector<Document> FindTopDocuments(std::execution::sequenced_policy policy, const std::string_view raw_query, DocumentStatus status) const;

    std::vector<Document> FindTopDocuments(std::execution::parallel_policy policy, const std::string_view raw_query, DocumentStatus status) const;

    std::vector<Document> FindTopDocuments(const std::string_view raw_query, DocumentStatus status) const;

    std::vector<Document> FindTopDocuments(const std::string_view raw_query) const;

    std::vector<Document> FindTopDocuments(std::execution::sequenced_policy policy, const std::string_view raw_query) const;

    std::vector<Document> FindTopDocuments(std::execution::parallel_policy policy, const std::string_view raw_query) const;

    int GetDocumentCount() const;

    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(const std::string_view raw_query,
                                                                            int document_id) const;

    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(std::execution::sequenced_policy policy, const std::string_view raw_query,
                                                                            int document_id) const;

    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(std::execution::parallel_policy policy, const std::string_view raw_query,
                                                                            int document_id) const;

    std::set<std::string, std::less<>> GetStopWords()
    {
        return stop_words_;
    }

private:
    struct DocumentData
    {
        int rating;
        DocumentStatus status;
    };

    const std::set<std::string, std::less<>> stop_words_;
    std::map<std::string_view, std::map<int, double>> word_to_document_freqs_; // 1
    std::map<int, DocumentData> documents_;
    std::set<int> document_ids_;

    std::deque<std::string> storage;

    std::map<int, std::map<std::string_view, double>> ids_to_word_freq_; // 2

    bool IsStopWord(const std::string_view word) const;

    static bool IsValidWord(const std::string_view word);

    std::vector<std::string_view> SplitIntoWordsNoStop(const std::string_view text) const;

    static int ComputeAverageRating(const std::vector<int> &ratings);

    struct QueryWord;

    QueryWord ParseQueryWord(const std::string_view text) const;

    struct Query
    {
        std::vector<std::string_view> plus_words;
        std::vector<std::string_view> minus_words;
    };

    Query ParseQuery(const std::string_view text) const;
    Query ParseQueryWithoutDeleteCopyes(const std::string_view text) const;

    // Existence required
    double ComputeWordInverseDocumentFreq(const std::string_view word) const;

    template <typename ExecutionPolicy, typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(ExecutionPolicy &policy, const Query &query,
                                           DocumentPredicate document_predicate) const;
};

template <typename StringContainer>
SearchServer::SearchServer(const StringContainer &stop_words)
    : stop_words_(MakeUniqueNonEmptyStrings(stop_words)) // Extract non-empty stop words
{
    using namespace std::string_literals;
    if (!std::all_of(stop_words_.begin(), stop_words_.end(), IsValidWord))
    {
        throw std::invalid_argument("Some of stop words are invalid"s);
    }
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(std::execution::sequenced_policy policy, const std::string_view raw_query,
                                                     DocumentPredicate document_predicate) const
{
    const auto query = ParseQuery(raw_query);

    auto matched_documents = FindAllDocuments(policy, query, document_predicate);

    sort(matched_documents.begin(), matched_documents.end(),
         [](const Document &lhs, const Document &rhs)
         {
             if (std::abs(lhs.relevance - rhs.relevance) < DIFF)
             {
                 return lhs.rating > rhs.rating;
             }
             else
             {
                 return lhs.relevance > rhs.relevance;
             }
         });
    if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT)
    {
        matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
    }

    return matched_documents;
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(std::execution::parallel_policy policy, const std::string_view raw_query,
                                                     DocumentPredicate document_predicate) const
{
    const auto query = ParseQuery(raw_query);

    auto matched_documents = FindAllDocuments(policy, query, document_predicate);

    sort(policy, matched_documents.begin(), matched_documents.end(),
         [](const Document &lhs, const Document &rhs)
         {
             if (std::abs(lhs.relevance - rhs.relevance) < DIFF)
             {
                 return lhs.rating > rhs.rating;
             }
             else
             {
                 return lhs.relevance > rhs.relevance;
             }
         });
    if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT)
    {
        matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
    }

    return matched_documents;
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(const std::string_view raw_query,
                                                     DocumentPredicate document_predicate) const
{
    std::execution::sequenced_policy policy;
    return FindTopDocuments(policy, raw_query, document_predicate);
}
// template <typename DocumentPredicate>
// std::vector<Document> SearchServer::FindAllDocuments(const Query& query,
//                                     DocumentPredicate document_predicate) const {
//     std::map<int, double> document_to_relevance;
//     for (const std::string_view word : query.plus_words) {
//         if (word_to_document_freqs_.count(word) == 0) {
//             continue;
//         }
//         const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
//         for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
//             const auto& document_data = documents_.at(document_id);
//             if (document_predicate(document_id, document_data.status, document_data.rating)) {
//                 document_to_relevance[document_id] += term_freq * inverse_document_freq;
//             }
//         }
//     }

//     for (const auto word : query.minus_words) {
//         if (word_to_document_freqs_.count(word) == 0) {
//             continue;
//         }
//         for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
//             document_to_relevance.erase(document_id);
//         }
//     }

//     std::vector<Document> matched_documents;
//     for (const auto [document_id, relevance] : document_to_relevance) {
//         matched_documents.push_back(
//             {document_id, relevance, documents_.at(document_id).rating});
//     }
//     return matched_documents;
// }

template <typename ExecutionPolicy, typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(ExecutionPolicy &policy, const Query &query,
                                                     DocumentPredicate document_predicate) const
{
    if constexpr (std::is_same_v<std::decay_t<ExecutionPolicy>, std::execution::sequenced_policy>)
    {
        std::map<int, double> document_to_relevance;
        for (const std::string_view word : query.plus_words)
        {
            if (word_to_document_freqs_.count(word) == 0)
            {
                continue;
            }
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);

            for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word))
            {
                const auto &document_data = documents_.at(document_id);
                if (document_predicate(document_id, document_data.status, document_data.rating))
                {
                    document_to_relevance[document_id] += term_freq * inverse_document_freq;
                }
            }
        }

        for (const auto word : query.minus_words)
        {
            if (word_to_document_freqs_.count(word) == 0)
            {
                continue;
            }
            for (const auto [document_id, _] : word_to_document_freqs_.at(word))
            {
                document_to_relevance.erase(document_id);
            }
        }

        std::vector<Document> matched_documents;
        for (const auto [document_id, relevance] : document_to_relevance)
        {
            matched_documents.push_back(
                {document_id, relevance, documents_.at(document_id).rating});
        }
        return matched_documents;
    }

    else
    {
        ConcurrentMap<int, double> document_to_relevance(15);
        std::for_each(std::execution::par, query.plus_words.begin(), query.plus_words.end(),
                      [&](std::string_view word)
                      {
                          if (word_to_document_freqs_.count(word) == 0)
                          {
                              return;
                          }
                          const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);

                          std::for_each(std::execution::par, word_to_document_freqs_.at(word).begin(), word_to_document_freqs_.at(word).end(),
                                        [&](auto memb)
                                        {
                                            const auto &document_data = documents_.at(memb.first);
                                            if (document_predicate(memb.first, document_data.status, document_data.rating))
                                            {
                                                document_to_relevance[memb.first].ref_to_value += memb.second * inverse_document_freq;
                                            }
                                        });
                      });

        std::vector<Document> matched_documents;
        for (const auto [document_id, relevance] : document_to_relevance.BuildOrdinaryMap())
        {
            matched_documents.push_back(
                {document_id, relevance, documents_.at(document_id).rating});
        }
        return matched_documents;
    }
}

// for (const std::string_view word : query.plus_words) {
//     if (word_to_document_freqs_.count(word) == 0) {
//         continue;
//     }
//     const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);

//     for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
//         const auto& document_data = documents_.at(document_id);
//         if (document_predicate(document_id, document_data.status, document_data.rating)) {
//             document_to_relevance[document_id] += term_freq * inverse_document_freq;
//         }
//     }
// }

// for (const auto word : query.minus_words) {
//     if (word_to_document_freqs_.count(word) == 0) {
//         continue;
//     }
//     for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
//         document_to_relevance.erase(document_id);
//     }
// }

// std::vector<Document> matched_documents;
//     for (const auto [document_id, relevance] : document_to_relevance) {
//         matched_documents.push_back(
//             {document_id, relevance, documents_.at(document_id).rating});
//     }
//     return matched_documents;
// }