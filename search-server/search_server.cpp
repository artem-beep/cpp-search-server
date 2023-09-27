#include "search_server.h"
#include <algorithm>
#include <cmath>
#include <utility>
#include <stdexcept>
#include <numeric>
#include <execution>
std::set<int>::iterator SearchServer::begin()
{
    return document_ids_.begin();
}

std::set<int>::iterator SearchServer::end()
{
    return document_ids_.end();
}

const std::map<std::string_view, double> &SearchServer::GetWordFrequencies(int document_id) const
{

    auto &res = ids_to_word_freq_.at(document_id);
    return res;
}

void SearchServer::RemoveDocument(int document_id)
{
    auto memb = GetWordFrequencies(document_id);
    for (auto [word, freq] : memb)
    {
        word_to_document_freqs_.at(word).erase(document_id);
    }
    // удаление из словаря documents_
    documents_.erase(document_id);
    // удаление из вектора document_ids_
    document_ids_.erase(document_id);
}

void SearchServer::RemoveDocument(std::execution::sequenced_policy policy, int document_id)
{
    RemoveDocument(document_id);
}

void SearchServer::RemoveDocument(std::execution::parallel_policy policy, int document_id)
{
    const auto &memb = GetWordFrequencies(document_id);
    std::vector<std::string_view> words(memb.size());
    std::transform(policy, memb.begin(), memb.end(), words.begin(), [](const auto &c)
                   { return c.first; });

    auto func = [this, &document_id](const std::string_view word)
    {
        word_to_document_freqs_.at(word).erase(document_id);
    };

    std::for_each(policy, words.begin(), words.end(), func);

    documents_.erase(document_id);
    document_ids_.erase(document_id);
}

SearchServer::SearchServer(const std::string &stop_words_text)
    : SearchServer(
          SplitIntoWords(stop_words_text)) // Invoke delegating constructor from string container
{
}

SearchServer::SearchServer(std::string_view stop_words_text)
    : SearchServer(
          SplitIntoWords(stop_words_text))
{
}

SearchServer::SearchServer(const std::set<std::string> &stop_words)
    : stop_words_(MakeUniqueNonEmptyStrings(stop_words))
{
    using namespace std::string_literals;
    if (!all_of(stop_words_.begin(), stop_words_.end(), IsValidWord))
    {
        throw std::invalid_argument("Some of stop words are invalid"s);
    }
}

void SearchServer::AddDocument(int document_id, const std::string_view document, DocumentStatus status,
                               const std::vector<int> &ratings)
{
    using namespace std::string_literals;
    if ((document_id < 0) || (documents_.count(document_id) > 0))
    {
        throw std::invalid_argument("Invalid document_id"s);
    }
    storage.emplace_back(document);
    auto words = SplitIntoWordsNoStop(storage.back());

    const double inv_word_count = 1.0 / words.size();
    for (const auto word : words)
    {
        word_to_document_freqs_[word][document_id] += inv_word_count;
        ids_to_word_freq_[document_id][word] += inv_word_count;
    }

    documents_.emplace(document_id, SearchServer::DocumentData{ComputeAverageRating(ratings), status});
    document_ids_.insert(document_id);
}

std::vector<Document> SearchServer::FindTopDocuments(std::execution::sequenced_policy policy, const std::string_view raw_query, DocumentStatus status) const
{
    return SearchServer::FindTopDocuments(policy,
                                          raw_query, [status](int document_id, DocumentStatus document_status, int rating)
                                          { return document_status == status; });
}

std::vector<Document> SearchServer::FindTopDocuments(std::execution::parallel_policy policy, const std::string_view raw_query, DocumentStatus status) const
{
    return SearchServer::FindTopDocuments(policy,
                                          raw_query, [status](int document_id, DocumentStatus document_status, int rating)
                                          { return document_status == status; });
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string_view raw_query, DocumentStatus status) const
{
    std::execution::sequenced_policy policy;
    return SearchServer::FindTopDocuments(policy,
                                          raw_query, [status](int document_id, DocumentStatus document_status, int rating)
                                          { return document_status == status; });
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string_view raw_query) const
{
    std::execution::sequenced_policy policy;
    return SearchServer::FindTopDocuments(policy, raw_query, DocumentStatus::ACTUAL);
}

std::vector<Document> SearchServer::FindTopDocuments(std::execution::sequenced_policy policy, const std::string_view raw_query) const
{
    return SearchServer::FindTopDocuments(policy, raw_query, DocumentStatus::ACTUAL);
}

std::vector<Document> SearchServer::FindTopDocuments(std::execution::parallel_policy policy, const std::string_view raw_query) const
{
    return SearchServer::FindTopDocuments(policy, raw_query, DocumentStatus::ACTUAL);
}

int SearchServer::GetDocumentCount() const
{
    return documents_.size();
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(const std::string_view raw_query,
                                                                                      int document_id) const
{

    const auto query = SearchServer::ParseQuery(raw_query);

    std::vector<std::string_view> matched_words;

    if (std::any_of(query.minus_words.begin(), query.minus_words.end(), [=](auto minus_word)
                    {
        if (word_to_document_freqs_.count(minus_word)) {
            return word_to_document_freqs_.at(minus_word).count(document_id) != 0;
        }
        else {
            return false;
        } }))
    {
        return {matched_words, documents_.at(document_id).status};
    }

    for (const auto word : query.plus_words)
    {
        if (word_to_document_freqs_.count(word) == 0)
        {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id))
        {
            matched_words.push_back(word);
        }
    }
    return {matched_words, documents_.at(document_id).status};
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(std::execution::sequenced_policy policy, const std::string_view raw_query,
                                                                                      int document_id) const
{
    return MatchDocument(raw_query, document_id);
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(std::execution::parallel_policy policy, const std::string_view raw_query,
                                                                                      int document_id) const
{
    const auto query = SearchServer::ParseQueryWithoutDeleteCopyes(raw_query);

    std::vector<std::string_view> matched_words(query.plus_words.size());

    if (std::any_of(policy, query.minus_words.begin(), query.minus_words.end(), [=](auto minus_word)
                    {
        if (word_to_document_freqs_.count(minus_word)) {
            return word_to_document_freqs_.at(minus_word).count(document_id) != 0;
        }
        else {
            return false;
        } }))
    {
        return {std::vector<std::string_view>{}, documents_.at(document_id).status};
    }

    auto last1 = std::copy_if(policy, query.plus_words.begin(), query.plus_words.end(), matched_words.begin(), [=](auto plus_word)
                              {
        if (word_to_document_freqs_.count(plus_word)) {
        return word_to_document_freqs_.at(plus_word).count(document_id)!= 0;
        }
        else {
            return false;
        } });

    std::sort(matched_words.begin(), last1);
    auto last2 = std::unique(matched_words.begin(), last1);
    matched_words.erase(last2, matched_words.end());

    return {matched_words, documents_.at(document_id).status};
}

bool SearchServer::IsStopWord(const std::string_view word) const
{
    return stop_words_.count(word) > 0;
}

bool SearchServer::IsValidWord(const std::string_view word)
{
    // A valid word must not contain special characters
    return std::none_of(word.begin(), word.end(), [](char c)
                        { return c >= '\0' && c < ' '; });
}

std::vector<std::string_view> SearchServer::SplitIntoWordsNoStop(const std::string_view text) const
{
    std::vector<std::string_view> words;
    using namespace std::string_literals;
    for (const auto word : SplitIntoWords(text))
    {
        if (!IsValidWord(word))
        {
            throw std::invalid_argument("Word "s + std::string{word} + " is invalid"s);
        }
        if (!IsStopWord(word))
        {
            words.push_back(word);
        }
    }
    return words;
}

int SearchServer::ComputeAverageRating(const std::vector<int> &ratings)
{
    if (ratings.empty())
    {
        return 0;
    }
    int rating_sum = std::accumulate(ratings.begin(), ratings.end(), 0);

    return rating_sum / static_cast<int>(ratings.size());
}

struct SearchServer::QueryWord
{
    std::string_view data;
    bool is_minus;
    bool is_stop;
};

SearchServer::QueryWord SearchServer::ParseQueryWord(const std::string_view text) const
{
    using namespace std::string_literals;
    if (text.empty())
    {
        throw std::invalid_argument("Query word is empty"s);
    }
    auto word = text;
    bool is_minus = false;
    if (word[0] == '-')
    {
        is_minus = true;
        word = word.substr(1);
    }
    if (word.empty() || word[0] == '-' || !IsValidWord(word))
    {
        throw std::invalid_argument("Query word "s + std::string{text} + " is invalid");
    }

    return {word, is_minus, IsStopWord(word)};
}

SearchServer::Query SearchServer::ParseQuery(const std::string_view text) const
{
    SearchServer::Query query;
    for (const auto word : SplitIntoWords(text))
    {
        const auto query_word = SearchServer::ParseQueryWord(word);
        if (!query_word.is_stop)
        {
            if (query_word.is_minus)
            {
                query.minus_words.push_back(query_word.data);
            }
            else
            {
                query.plus_words.push_back(query_word.data);
            }
        }
    }
    std::sort(query.plus_words.begin(), query.plus_words.end());
    std::sort(query.minus_words.begin(), query.minus_words.end());

    auto last1 = std::unique(query.plus_words.begin(), query.plus_words.end());
    auto last2 = std::unique(query.minus_words.begin(), query.minus_words.end());

    query.plus_words.erase(last1, query.plus_words.end());
    query.minus_words.erase(last2, query.minus_words.end());
    return query;
}

SearchServer::Query SearchServer::ParseQueryWithoutDeleteCopyes(const std::string_view text) const
{
    SearchServer::Query query;
    for (const auto word : SplitIntoWords(text))
    {
        const auto query_word = SearchServer::ParseQueryWord(word);
        if (!query_word.is_stop)
        {
            if (query_word.is_minus)
            {
                query.minus_words.push_back(query_word.data);
            }
            else
            {
                query.plus_words.push_back(query_word.data);
            }
        }
    }
    return query;
}

// Existence required
double SearchServer::ComputeWordInverseDocumentFreq(const std::string_view word) const
{
    return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}
