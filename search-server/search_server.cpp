#include "search_server.h"



using namespace std;


SearchServer::SearchServer() = default;

SearchServer::SearchServer(const std::string& stop_words_text)
    : SearchServer(SplitIntoWords(stop_words_text))
{
}

SearchServer::SearchServer(std::string_view stop_words_view)
    : SearchServer(SplitIntoWords(stop_words_view))
{
}


void SearchServer::AddDocument(int document_id, std::string_view document, DocumentStatus status, const vector<int>& ratings)
{

    if ((document_id < 0) || (documents_.count(document_id) > 0))
    {
        throw invalid_argument("Invalid document_id"s);
    }

    const auto words = SplitIntoWordsNoStop(document);

    const double inv_word_count = 1.0 / words.size();

    for (std::string_view word : words)
    {
        auto string_word = all_words_.insert(std::string(word));
        word_to_document_freqs_[*string_word.first][document_id] += inv_word_count;

        //добавление частот слов по id документа
        word_freq_[document_id][*string_word.first] += inv_word_count;
    }
    documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });
    document_ids_.insert(document_id);
}


vector<Document> SearchServer::FindTopDocuments(std::string_view raw_query, DocumentStatus status) const
{
    return FindTopDocuments(std::execution::seq, 
        raw_query, [status](int document_id, DocumentStatus document_status, int rating)
        {
            return document_status == status;
        });
}


vector<Document> SearchServer::FindTopDocuments(std::string_view raw_query) const
{
    return FindTopDocuments(std::execution::seq, raw_query, DocumentStatus::ACTUAL);
}


int SearchServer::GetDocumentCount() const
{
    return documents_.size();
}


typename std::set<int>::const_iterator SearchServer::begin() const
{
    return SearchServer::document_ids_.begin();
}


typename std::set<int>::const_iterator SearchServer::end() const
{
    return SearchServer::document_ids_.end();
}


using Matched_Doc_result = std::tuple<std::vector<std::string_view>, DocumentStatus>;
Matched_Doc_result SearchServer::MatchDocument(std::string_view raw_query, int document_id) const
{
    return MatchDocument(std::execution::seq, raw_query, document_id);
}


const std::map<std::string_view, double>& SearchServer::GetWordFrequencies(int document_id) const
{

    static const std::map<std::string_view, double> empty_map;

    if (word_freq_.empty())
    {
        return empty_map;
    }

    return word_freq_.at(document_id);

}


void SearchServer::RemoveDocument(int document_id)
{
    //“еперь из обычной версии вызываетс€ последовательна€, чтобы избежать дублировани€ кода
    RemoveDocument(std::execution::seq, document_id);    
}


//private:

// ѕроверка - "это стоп-слово?"
bool SearchServer::IsStopWord(std::string_view word) const
{
    return stop_words_.count(word) > 0;
}

// ѕроверка на спец-символы
bool SearchServer::IsValidWord(std::string_view word)
{
    return none_of(word.begin(), word.end(), [](char c)
        {
            return c >= '\0' && c < ' ';
        });
}


vector<std::string_view> SearchServer::SplitIntoWordsNoStop(std::string_view text) const
{
    std::vector<std::string_view> words = SplitIntoWords(text);
    std::vector<std::string_view> words_no_stop;
    words_no_stop.reserve(words.size());

    std::for_each(
        words.begin(), words.end(),
        [this, &words_no_stop](std::string_view word)
        {
            if (stop_words_.count(word) == 0)
            {
                !IsValidWord(word) ? throw std::invalid_argument("Invalid word(s) in the adding doccument!"s)
                    : words_no_stop.push_back(word);
            }
        }
    );

    return words_no_stop;
}


int SearchServer::ComputeAverageRating(const vector<int>& ratings)
{
    if (ratings.empty())
    {
        return 0;
    }

    int rating_sum = std::accumulate(ratings.begin(), ratings.end(), 0);

    return rating_sum / static_cast<int>(ratings.size());
}


SearchServer::QueryWord SearchServer::ParseQueryWord(std::string_view text) const
{

    if (text.empty())
    {
        throw invalid_argument("Query word is empty"s);
    }

    bool is_minus = false;

    if (text[0] == '-')
    {
        is_minus = true;
        text = text.substr(1);
    }

    if (text.empty() || text[0] == '-' || !IsValidWord(text))
    {
        throw invalid_argument("Query word "s + std::string(text) + " is invalid");
    }
    return { text, is_minus, IsStopWord(text) };
}


double SearchServer::ComputeWordInverseDocumentFreq(std::string_view word) const
{
    return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.find(word)->second.size());
}
