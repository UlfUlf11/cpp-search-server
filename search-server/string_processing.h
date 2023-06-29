#pragma once

#include <vector>
#include <string>
#include <set>

template <typename StringContainer>
std::set<std::string, std::less<>> MakeUniqueNonEmptyStrings(const StringContainer& strings)
{
    using namespace std;

    set<string, std::less<>> non_empty_strings;

    for (const auto& str : strings)
    {
        if (!str.empty())
        {
            std::string new_word = static_cast<string>(str);
            non_empty_strings.insert(new_word);
        }
    }

    return non_empty_strings;
}



std::vector<std::string_view> SplitIntoWords(std::string_view text);
