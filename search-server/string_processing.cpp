#include "string_processing.h"

std::vector<std::string_view> SplitIntoWords(std::string_view text)
{
    using namespace std;

    vector<string_view> result;

    text.remove_prefix(min(text.find_first_not_of(" "), text.size()));

    const int64_t pos_end = text.npos;

    while (!text.empty())
    {
        text.remove_prefix(text.find_first_not_of(" "));
        int64_t space = text.find(' ');
        result.push_back(space == pos_end ? text.substr(0) : text.substr(0, space));

        if (space == pos_end)
        {
            break;
        }
        else
        {
            text.remove_prefix(space + 1);
            int64_t n = text.find_first_not_of(" ");

            if (n == pos_end)
            {
                break;
            }
        }
    }
    return result;
}
