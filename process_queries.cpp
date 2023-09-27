#include "process_queries.h"
#include <execution>
#include <algorithm>
#include <functional>
std::vector<std::vector<Document>> ProcessQueries(
    const SearchServer &search_server,
    const std::vector<std::string> &queries)
{
    std::vector<std::vector<Document>> result(queries.size());
    std::transform(std::execution::par, queries.begin(), queries.end(), result.begin(),
                   [&search_server](std::string c)
                   {
                       return search_server.FindTopDocuments(c);
                   });
    return result;
}

std::vector<Document> ProcessQueriesJoined(
    const SearchServer &search_server,
    const std::vector<std::string> &queries)
{
    auto result = ProcessQueries(search_server, queries);
    std::vector<Document> last_res;
    for (auto &memb : result)
    {
        for (Document &doc : memb)
        {
            last_res.push_back(doc);
        }
    }
    return last_res;
}