#pragma once

#include <string_view>
#include <vector>
#include <utility>
#include <map>
#include <set>
#include <tuple>
#include <cmath>
#include <algorithm>
#include <stdexcept>
#include <execution>
#include <type_traits>

#include "document.h"
#include "string_processing.h"
#include "concurrent_map.h"

//Максимальное количество топ-документов
const int MAX_RESULT_DOCUMENT_COUNT = 5;
//Допустимая по релевантности погрешнось 
const double EPSILON = 1e-6;


class SearchServer
{
public:

    SearchServer();

    //конструкторы:
    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words);
    explicit SearchServer(const std::string& stop_words_text);
    explicit SearchServer(std::string_view stop_words_view);

    //Добавляем документ в базу
    void AddDocument(int document_id, std::string_view document, DocumentStatus status, const std::vector<int>& ratings);

    //Выбираем ТОП-документы
    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(std::string_view raw_query, DocumentPredicate document_predicate) const;
    std::vector<Document> FindTopDocuments(std::string_view raw_query, DocumentStatus status) const;
    std::vector<Document> FindTopDocuments(std::string_view raw_query) const;

    // Версия FindTopDocuments, выполняющаяся последовательно или параллельно в зависимости от переданной политики
    template <class ExecutionPolicy, typename Predicate>
    std::vector<Document> FindTopDocuments(const ExecutionPolicy& policy, std::string_view raw_query, Predicate predicate) const;
    
    template <class ExecutionPolicy>
    std::vector<Document> FindTopDocuments(const ExecutionPolicy& policy, std::string_view raw_query, DocumentStatus status) const;
    
    template <class ExecutionPolicy>
    std::vector<Document> FindTopDocuments(const ExecutionPolicy& policy, std::string_view raw_query) const;


    //Узнаём количество и id документа
    int GetDocumentCount() const;

    typename std::set<int>::const_iterator begin() const;
    typename std::set<int>::const_iterator end() const;

    //Матчинг документов
    using Matched_Doc_result = std::tuple<std::vector<std::string_view>, DocumentStatus>;
    Matched_Doc_result MatchDocument(std::string_view raw_query, int document_id) const;
    Matched_Doc_result MatchDocument(const std::execution::sequenced_policy& policy, std::string_view raw_query, int document_id) const;
    Matched_Doc_result MatchDocument(const std::execution::parallel_policy& policy, std::string_view raw_query, int document_id) const;

    //Получаем частоту слов в нужном документе
    const std::map<std::string_view, double>& GetWordFrequencies(int document_id) const;

    //Удаляем документ
    void RemoveDocument(int document_id);
    void RemoveDocument(const std::execution::sequenced_policy& policy, int document_id);
    void RemoveDocument(const std::execution::parallel_policy& policy, int document_id);

private:

    //Храним информацию о документах
    struct DocumentData
    {
        int rating;
        DocumentStatus status;
    };

    //Храним парсированное слово запроса
    struct QueryWord
    {
        std::string_view data;
        bool is_minus;
        bool is_stop;
    };

    //Храним парсированный запрос
    struct Query
    {
        std::vector<std::string_view> plus_words;
        std::vector<std::string_view> minus_words;
    };

    //Храним множество стоп-слов
    const  std::set< std::string, std::less<>> stop_words_;

    //<id документа, информация о документе>
    std::map<int, DocumentData> documents_;

    //Храним id документов
    std::set<int> document_ids_;

    //Чтобы не инвалидировались ссылки при добавлении
    std::set<std::string, std::less<>> all_words_;

    //Храним документы
    //      < слово(ключ)   <  id(ключ),  TF  >>
    std::map< std::string, std::map<int, double>, std::less<>> word_to_document_freqs_;
    //    < id(ключ)    <  слово(ключ),   TF  >>
    std::map<int, std::map<std::string_view, double>> word_freq_;

    //Проверки
    bool IsStopWord(std::string_view word) const;

    static bool IsValidWord(std::string_view word);

    //Формируем вектор из строки с пробелами и вычёркиваем стоп-слова
    std::vector<std::string_view> SplitIntoWordsNoStop(std::string_view text) const;

    //Вычисляем средний рейтинг документа
    static int ComputeAverageRating(const  std::vector<int>& ratings);

    //Парсинг слова строки запроса
    QueryWord ParseQueryWord(std::string_view text) const;

    //Парсинг строки запроса
    Query ParseQuery(bool parallel, std::string_view text) const;

    //Вычисляем IDF конкретного слова из запроса
    double ComputeWordInverseDocumentFreq(std::string_view plus_word) const;

    //Найти все документы, подходящие под запрос
    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const Query& query, DocumentPredicate document_predicate) const;

    template <class ExecutionPolicy, typename Predicate>
    std::vector<Document> FindAllDocuments(const ExecutionPolicy& policy, const Query& query, Predicate predicate) const;
};


template <typename StringContainer>
SearchServer::SearchServer(const StringContainer& stop_words)
    : stop_words_(MakeUniqueNonEmptyStrings(stop_words))
{
    using namespace std;
    if (!all_of(stop_words.begin(), stop_words.end(), [](const auto& stop_word)
        {
            return IsValidWord(stop_word);
        }))
    {
        throw invalid_argument("Some of stop words are invalid"s);
    }
}


template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(std::string_view raw_query, DocumentPredicate document_predicate) const
{
    using namespace std;

    //Парсинг строки запроса будет осуществляться НЕ ПАРАЛЛЕЛЬНО
    bool parallel = false;

    const Query query = ParseQuery(parallel, raw_query);

    auto matched_documents = FindAllDocuments(query, document_predicate);

    sort(matched_documents.begin(), matched_documents.end(),
        [](const Document& lhs, const Document& rhs)
        {
            if (std::abs(lhs.relevance - rhs.relevance) < EPSILON)
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


// Версия FindTopDocuments, выполняющаяся последовательно или параллельно в зависимости от переданной политики
template <class ExecutionPolicy, typename Predicate>
std::vector<Document> SearchServer::FindTopDocuments(const ExecutionPolicy& policy, std::string_view raw_query, Predicate predicate) const
{
    //Если FindTop вызвана с последовательной политикой, то просто вызываем обычную функцию
    if constexpr (std::is_same_v<std::decay_t<ExecutionPolicy>, std::execution::sequenced_policy>) {
        return FindTopDocuments(raw_query, predicate);
    }

    //Парсинг строки запроса будет осуществляться ПАРАЛЛЕЛЬНО
    bool parallel = true;

    const Query query = ParseQuery(parallel, raw_query);

    auto matched_documents = FindAllDocuments(policy, query, predicate);

    sort(std::execution::par, matched_documents.begin(), matched_documents.end(),
        [](const Document& lhs, const Document& rhs)
        {
            if (std::abs(lhs.relevance - rhs.relevance) < EPSILON)
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


template <class ExecutionPolicy>
std::vector<Document> SearchServer::FindTopDocuments(const ExecutionPolicy& policy, std::string_view raw_query, DocumentStatus status) const
{
    return FindTopDocuments(policy, raw_query, [status](int id_document, DocumentStatus document_status, int rating)
        {
            return document_status == status;
        });
}


template <class ExecutionPolicy>
std::vector<Document> SearchServer::FindTopDocuments(const ExecutionPolicy& policy, std::string_view raw_query) const
{
    // Тогда будем показывать только АКТУАЛЬНЫЕ
    return FindTopDocuments(policy, raw_query, DocumentStatus::ACTUAL);
}





//Найти все документы, подходящие под запрос
template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(const Query& query, DocumentPredicate document_predicate) const
{
    using namespace std;
    //<id, relevance> (relevance = sum(tf * idf))
    map<int, double> document_to_relevance;

    //формируем набор документов document_to_relevance с плюс-словами и их релевантностью
    for (std::string_view plus_word : query.plus_words)
    {
        if (word_to_document_freqs_.count(plus_word) == 0)
        {
            continue;
        }
        //Вычисляем IDF конкретного слова из запроса
        const double inverse_document_freq = ComputeWordInverseDocumentFreq(plus_word);

        for (const auto& [document_id, term_freq] : word_to_document_freqs_.find(plus_word)->second)
        {
            const auto& document_data = documents_.at(document_id);
            if (document_predicate(document_id, document_data.status, document_data.rating))
            {
                //Вычисляем релевантность документа с учётом вхождения каждого плюс-слова
                document_to_relevance[document_id] += term_freq * inverse_document_freq;
            }
        }
    }

    //Вычёркиваем из document_to_relevance документы с минус-словами
    for (std::string_view minus_word : query.minus_words)
    {
        if (word_to_document_freqs_.count(minus_word) == 0)
        {
            continue;
        }

        for (const auto& [document_id, _] : word_to_document_freqs_.find(minus_word)->second)
        {
            document_to_relevance.erase(document_id);
        }
    }

    //Конечный вектор с отобранными документами
    vector<Document> matched_documents;

    for (const auto& [document_id, relevance] : document_to_relevance)
    {
        matched_documents.push_back({ document_id, relevance, documents_.at(document_id).rating });
    }

    return matched_documents;
}


template <class ExecutionPolicy, typename Predicate>
std::vector<Document> SearchServer::FindAllDocuments(const ExecutionPolicy& policy, const Query& query, Predicate predicate) const
{
    //Если FindAll вызвана с последовательной политикой, то просто вызываем обычную функцию
    if constexpr (std::is_same_v<std::decay_t<ExecutionPolicy>, std::execution::sequenced_policy>) {
        return FindAllDocuments(query, predicate);
    }

    ConcurrentMap<int, double> document_to_relevance(100);
    std::for_each(policy,
        query.plus_words.begin(),
        query.plus_words.end(),
        [this, &document_to_relevance, predicate](const std::string_view& plus_word)
        {
            if (word_to_document_freqs_.count(plus_word) == 0) return;

            //Вычисляем IDF конкретного слова из запроса
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(plus_word);

            for (const auto [document_id, term_freq] : word_to_document_freqs_.find(plus_word)->second)
            {
                const auto& document_data = documents_.at(document_id);
                if (predicate(document_id, document_data.status, document_data.rating))
                {
                    document_to_relevance[document_id].ref_to_value += term_freq * inverse_document_freq;
                }
            }
        });
    std::map<int, double> all_document_to_relevance = document_to_relevance.BuildOrdinaryMap();
    std::for_each(
        policy,
        query.minus_words.begin(), query.minus_words.end(),
        [this, &all_document_to_relevance](std::string_view word)
        {
            for (const auto [document_id, _] : word_to_document_freqs_.find(word)->second)
            {
                all_document_to_relevance.erase(document_id);
            }
        }
    );


    std::vector<Document> matched_documents;
    for (const auto& [document_id, relevance] : all_document_to_relevance)
    {
        matched_documents.push_back(
            { document_id, relevance, documents_.at(document_id).rating });
    }
    return matched_documents;
}
