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
    return FindTopDocuments(
        raw_query, [status](int document_id, DocumentStatus document_status, int rating)
        {
            return document_status == status;
        });
}


vector<Document> SearchServer::FindTopDocuments(std::string_view raw_query) const
{
    return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

std::vector<Document> SearchServer::FindTopDocuments(const std::execution::sequenced_policy& policy, std::string_view raw_query, DocumentStatus status) const
{
    return FindTopDocuments(raw_query, [status](int id_document, DocumentStatus document_status, int rating)
        {
            return document_status == status;
        });
}


std::vector<Document> SearchServer::FindTopDocuments(const std::execution::sequenced_policy& policy, std::string_view raw_query) const
{
    // Тогда будем показывать только АКТУАЛЬНЫЕ
    return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}


std::vector<Document> SearchServer::FindTopDocuments(const std::execution::parallel_policy& policy, std::string_view raw_query, DocumentStatus status) const
{
    return FindTopDocuments(policy, raw_query, [status](int id_document, DocumentStatus document_status, int rating)
        {
            return document_status == status;
        });
}

std::vector<Document> SearchServer::FindTopDocuments(const std::execution::parallel_policy& policy, std::string_view raw_query) const
{
    // Тогда будем показывать только АКТУАЛЬНЫЕ
    return FindTopDocuments(policy, raw_query, DocumentStatus::ACTUAL);
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


tuple<vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(std::string_view raw_query, int document_id) const
{
    if (!document_ids_.count(document_id))
    {
        throw out_of_range("Недействительный id документа"s);
    }

    const Query query = ParseQuerySeq(raw_query);

    vector<std::string_view> matched_words;

    bool has_minus_words = std::any_of(query.minus_words.begin(), query.minus_words.end(), [this, document_id](std::string_view word)
        {
            return word_to_document_freqs_.count(word) > 0 && word_to_document_freqs_.find(word)->second.count(document_id) > 0;
        });

    if (has_minus_words)
    {
        matched_words.clear();
        return { matched_words, documents_.at(document_id).status };
    }

    for (std::string_view word : query.plus_words)
    {
        if (word_to_document_freqs_.count(word) == 0)
        {
            continue;
        }
        if (word_to_document_freqs_.find(word)->second.count(document_id))
        {
            matched_words.push_back(word);
        }
    }

    return { matched_words, documents_.at(document_id).status };
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(const std::execution::sequenced_policy& policy, std::string_view raw_query, int document_id) const
{
    return MatchDocument(raw_query, document_id);
}

std::tuple< std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(const std::execution::parallel_policy& policy, std::string_view raw_query, int document_id) const
{

    if (!document_ids_.count(document_id))
    {
        throw out_of_range("Недействительный id документа"s);
    }

    Query query = ParseQuery(raw_query);

    vector<std::string_view> matched_words(query.plus_words.size());

    bool has_minus_words = std::any_of(std::execution::par, query.minus_words.begin(), query.minus_words.end(), [this, document_id](std::string_view word)
        {
            return word_to_document_freqs_.count(word) > 0 && word_to_document_freqs_.find(word)->second.count(document_id) > 0;
        });

    if (has_minus_words)
    {
        matched_words.clear();
        return { matched_words, documents_.at(document_id).status };
    }

    // Копирование плюс слов, без удаления дубликатов и с сохранением порядка
    auto copy_end = std::copy_if(std::execution::par, query.plus_words.begin(), query.plus_words.end(), matched_words.begin(), [this, document_id](std::string_view word)
        {
            return word_to_document_freqs_.count(word) > 0 && word_to_document_freqs_.find(word)->second.count(document_id) > 0;
        });

    matched_words.resize(std::distance(matched_words.begin(), copy_end));

    // Сортировка и удаление дубликатов
    std::sort(std::execution::par, matched_words.begin(), matched_words.end());
    matched_words.erase(std::unique(matched_words.begin(), matched_words.end()), matched_words.end());

    return { matched_words, documents_.at(document_id).status };
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

    if (documents_.count(document_id) == 0)
    {
        return;
    }

    std::vector<std::string_view> words_to_delete(word_freq_.at(document_id).size());

    std::transform(
        std::execution::seq,
        word_freq_.at(document_id).begin(), word_freq_.at(document_id).end(),
        words_to_delete.begin(),
        [](auto& word)
        {
            return word.first;
        }
    );

    std::for_each(
        std::execution::seq,
        words_to_delete.begin(), words_to_delete.end(),
        [this, document_id](auto& word_to_delete)
        {
            word_to_document_freqs_.find(word_to_delete)->second.erase(document_id);
        }
    );

    // std::set<int> document_ids_;
    document_ids_.erase(document_id);
    // std::map<int, DocumentData> documents_;
    documents_.erase(document_id);
    // std::map<int, std::map<std::string, double>> word_freq_;
    word_freq_.erase(document_id);
}

void SearchServer::RemoveDocument(const std::execution::sequenced_policy& policy, int document_id)
{
    return RemoveDocument(document_id);
}

void SearchServer::RemoveDocument(const std::execution::parallel_policy& policy, int document_id)
{

    if (documents_.count(document_id) == 0)
    {
        return;
    }

    std::vector<std::string_view> words_to_delete(word_freq_.at(document_id).size());

    std::transform(
        std::execution::par,
        word_freq_.at(document_id).begin(), word_freq_.at(document_id).end(),
        words_to_delete.begin(),
        [](auto& word)
        {
            return word.first;
        }
    );

    std::for_each(
        std::execution::par,
        words_to_delete.begin(), words_to_delete.end(),
        [this, document_id](auto& word_to_delete)
        {
            word_to_document_freqs_.find(word_to_delete)->second.erase(document_id);
        }
    );

    // std::set<int> document_ids_;
    document_ids_.erase(document_id);
    // std::map<int, DocumentData> documents_;
    documents_.erase(document_id);
    // std::map<int, std::map<std::string, double>> word_freq_;
    word_freq_.erase(document_id);
}

//private:

// Проверка - "это стоп-слово?"
bool SearchServer::IsStopWord(std::string_view word) const
{
    return stop_words_.count(word) > 0;
}

// Проверка на спец-символы
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

    int rating_sum = 0;

    for (const int rating : ratings)
    {
        rating_sum += rating;
    }

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

// Парсинг строки запроса
SearchServer::Query SearchServer::ParseQuerySeq(std::string_view text) const
{
    Query result;

    for (std::string_view word : SplitIntoWords(text))
    {
        const auto query_word = ParseQueryWord(word);
        if (!query_word.is_stop)
        {
            if (query_word.is_minus)
            {
                result.minus_words.push_back(query_word.data);
            }
            else
            {
                result.plus_words.push_back(query_word.data);
            }
        }
    }

    std::sort(result.minus_words.begin(), result.minus_words.end());
    auto lastminus = std::unique(result.minus_words.begin(), result.minus_words.end());
    result.minus_words.erase(lastminus, result.minus_words.end());

    std::sort(result.plus_words.begin(), result.plus_words.end());
    auto lastplus = std::unique(result.plus_words.begin(), result.plus_words.end());
    result.plus_words.erase(lastplus, result.plus_words.end());


    return result;
}

SearchServer::Query SearchServer::ParseQuery(std::string_view text) const
{
    std::vector<std::string_view> words = SplitIntoWords(text);
    Query result;

    std::for_each(
        words.begin(), words.end(),
        [this, &result](auto& word)
        {
            const QueryWord query_word = ParseQueryWord(word);
            if (!query_word.is_stop)
            {
                query_word.is_minus ? result.minus_words.push_back(query_word.data)
                    : result.plus_words.push_back(query_word.data);
            }
        }
    );

    return result;
}

double SearchServer::ComputeWordInverseDocumentFreq(std::string_view word) const
{
    return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.find(word)->second.size());
}
