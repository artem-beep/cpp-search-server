#include "string_processing.h"

std::vector<std::string_view> SplitIntoWords(std::string_view str) {
    std::vector<std::string_view> result;
    int64_t first_n_space = str.find_first_not_of(" ");
    if (first_n_space == static_cast<int64_t>(str.npos)) {
     std::vector<std::string_view> empty_vec;
        return empty_vec;
    }
    str.remove_prefix(first_n_space);
    const int64_t pos_end = str.npos;
    
    while (str.size() != 0) {
        if (str.find_first_not_of(" ") == str.npos) {
            break;
        }
        if (str[0] == ' ') {
            int64_t first_n_space2 = str.find_first_not_of(" ");
             str.remove_prefix(first_n_space2);
        }
        int64_t space = str.find(' ');
        result.push_back(space == pos_end ? str : str.substr(0, space));
        //pos = str.find_first_not_of(" ", space);
        if (space == pos_end) {
            str.remove_prefix(str.size()); 
        }
        else {
        str.remove_prefix(space + 1);
        }
       
    }
    return result;
}
