#include <algorithm>
#include <cstdlib>
#include <future>
#include <map>
#include <numeric>
#include <random>
#include <string>
#include <vector>
#include <mutex>

using namespace std::string_literals;

template <typename Key, typename Value>
class ConcurrentMap
{
public:
    static_assert(std::is_integral_v<Key>, "ConcurrentMap supports only integer keys"s);

    struct Access
    {
        // Access(Value &value, std::mutex &value_mutex) :guard(value_mutex), ref_to_value(value)
        // {}
        std::lock_guard<std::mutex> guard;
        Value &ref_to_value;
    };

    explicit ConcurrentMap(size_t bucket_count) : all_maps(bucket_count)
    {
    }

    Access operator[](const Key &key)
    {
        // auto & cur_map = all_maps[static_cast<uint64_t>(key) % all_maps.size()];
        // if (cur_map.first.count(key)) {
        //     return Access(cur_map.first.at(key), cur_map.second);
        // }
        // else {
        //     cur_map.first.insert({key, Value()});
        //     return Access(cur_map.first.at(key), cur_map.second);
        // }

        auto &cur_map = all_maps[static_cast<uint64_t>(key) % all_maps.size()];
        return {std::lock_guard<std::mutex>(cur_map.second), cur_map.first[key]};
    }

    std::map<Key, Value> BuildOrdinaryMap()
    {
        std::map<Key, Value> result;
        for (auto numb = 0; numb != static_cast<int>(all_maps.size()); numb++)
        {
            std::lock_guard guard(all_maps[numb].second);
            auto &cur_map = all_maps[numb].first;
            // all_maps[numb].second.lock();
            result.insert(cur_map.begin(), cur_map.end());
            // all_maps[numb].second.unlock();
        }
        return result;
    }

private:
    std::vector<std::pair<std::map<Key, Value>, std::mutex>> all_maps;
};
