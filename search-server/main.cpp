#include <algorithm>
#include <iostream>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <map>
#include <cmath>
using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;

string ReadLine()
{
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber()
{
    int result = 0;
    cin >> result;
    ReadLine();
    return result;
}

vector<string> SplitIntoWords(const string &text)
{
    vector<string> words;
    string word;
    for (const char c : text)
    {
        if (c == ' ')
        {
            if (!word.empty())
            {
                words.push_back(word);
                word.clear();
            }
        }
        else
        {
            word += c;
        }
    }
    if (!word.empty())
    {
        words.push_back(word);
    }

    return words;
}

struct Document
{
    int id;
    double relevance;
};

class SearchServer
{
public:


    double CountIdf(const string &word) const {
        return log(static_cast<double>(document_count_) / static_cast<double>(word_to_documents_freqs_.at(word).size())); 
    }

    void SetStopWords(const string &text)
    {
        for (const string &word : SplitIntoWords(text))
        {
            stop_words_.insert(word);
        }
    }

    void AddDocument(int document_id, const string &document)
    {
        const vector<string> words = SplitIntoWordsNoStop(document);
        ++document_count_;
        double increase = 1.0 / words.size();
        for (const auto &word : words)
        {

       
            word_to_documents_freqs_[word][document_id] += increase;
        }
    }

    vector<Document> FindTopDocuments(const string &raw_query) const
    {
        Query query_words = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query_words);

        sort(matched_documents.begin(), matched_documents.end(),
             [](const Document &lhs, const Document &rhs)
             {
                 return lhs.relevance > rhs.relevance;
             });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT)
        {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }

private:
    int document_count_ = 0;

    struct Query
    {
        set<string> plus_words;
        set<string> minus_words;
    };

    map<string, map<int, double>> word_to_documents_freqs_;

    set<string> stop_words_;

    bool IsStopWord(const string &word) const
    {
        return stop_words_.count(word) > 0;
    }

    vector<string> SplitIntoWordsNoStop(const string &text) const
    {
        vector<string> words;
        for (const string &word : SplitIntoWords(text))
        {
            if (!IsStopWord(word))
            {
                words.push_back(word);
            }
        }
        return words;
    }

    Query ParseQuery(const string &text) const
    {
        Query query;
        for (const string &word : SplitIntoWordsNoStop(text))
        {
            if ((word[0] == '-') && (stop_words_.count(word.substr(1)) == 0))
            {
                query.minus_words.insert(word.substr(1));
            }
            query.plus_words.insert(word);
        }
        return query;
    }

    vector<Document> FindAllDocuments(Query query_words) const
    {
        vector<Document> matched_documents;
        map<int, double> document_to_relevance;

        if (!query_words.plus_words.empty())
        {
            for (auto &word : query_words.plus_words)
            {
                if (word_to_documents_freqs_.count(word))
                {
                    double idf = CountIdf(word); 
                    // double idf = CountIdf(word_to_documents_.at(word).size());

                    for (const auto &[id, TIF] : word_to_documents_freqs_.at(word))
                    {
                        document_to_relevance[id] += TIF * idf;
                    }
                }
            }
        }

        for (auto &word : query_words.minus_words)
        {
            if (word_to_documents_freqs_.count(word) == 0)
            {
                continue;
            }
            else
            {
                for (auto [words, info] : word_to_documents_freqs_)
                {
                    if (words == word)
                    {
                     for (const auto [document_id, smth] : word_to_documents_freqs_.at(word))
                 {
                        document_to_relevance.erase(document_id);
                 }
                    }
                }
            }
        };

        for (auto &[id, rel] : document_to_relevance)
        {
            matched_documents.push_back({id, rel});
        }

        return matched_documents;
    }
};

SearchServer CreateSearchServer()
{
    SearchServer search_server;
    search_server.SetStopWords(ReadLine());

    int document_count = ReadLineWithNumber();
    for (int document_id = 0; document_id < document_count; ++document_id)
    {
        search_server.AddDocument(document_id, ReadLine());
    }

    return search_server;
}

int main()
{
    const SearchServer search_server = CreateSearchServer();

    const string query = ReadLine();
    for (const auto &[document_id, relevance] : search_server.FindTopDocuments(query))
    {
        cout << "{ document_id = "s << document_id << ", "
             << "relevance = "s << relevance << " }"s << endl;
    }
}

