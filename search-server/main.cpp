/* Решение из урока №3 (на основе optional) */
#include <iostream>
#include <limits>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <cmath>
#include <numeric>
#include <utility>



using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;
const auto EPSILON = 1e-6;


string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}


int ReadLineWithNumber() {
    int result;
    cin >> result;
    ReadLine();
    return result;
}


vector<string> SplitIntoWords(const string& text) {
    vector<string> words;
    string word;
    for (const char c : text) {
        if (c == ' ') {
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        }
        else {
            word += c;
        }
    }
    if (!word.empty()) {
        words.push_back(word);
    }
   return words;
}


struct Document {
    Document() : id(0), relevance(0), rating(0) {}

    Document(int id, double relevance, int rating)
    : id(id), relevance(relevance), rating(rating) {}

    int id;
    double relevance;
    int rating;
};

enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED
};


template <typename StringCollection>
set<string> SetStopWords(const StringCollection& words) {
    set<string> stop_words;
    for (const string& word : words) {
        if (!word.empty()) {
            stop_words.insert(word);
        }
    } return stop_words;
}


class SearchServer {
public:
    inline static constexpr int INVALID_DOCUMENT_ID = -1;


    template <typename StringCollection>
    explicit SearchServer(const StringCollection& stop_words)
    : stop_words_(SetStopWords(stop_words))
    {
        for (const string& word : stop_words_) {
            if (!IsValidWord(word) ) {
                throw invalid_argument("Документ не был добавлен, так как содержит спецсимволы");
            }
        }
    }


    explicit SearchServer(const string& stop_words)
    : SearchServer(SplitIntoWords(stop_words))
    {

    }


      // позволяющий получить идентификатор документа по его порядковому номеру
    int GetDocumentId(int index) const {
        int counter = -1;

        if (index < 0 || index > documents_.size())
        {
            throw out_of_range("Индекс переданного документа выходит за пределы допустимого диапазона"s);
        }

        for (const auto& [id, data] : documents_) {
            if (counter + 1 == index) {
                return id;
            }
            ++counter;
        }
        return INVALID_DOCUMENT_ID;
    }

    int GetDocumentCount() const {
        return documents_.size();
    }


    void AddDocument(int document_id, const string& document, DocumentStatus status, const vector<int>& ratings) {
        if (document_id < 0) {
            throw invalid_argument("id не может быть отрицательным"s);
        }
        if (documents_.count(document_id)) {
            throw invalid_argument("id уже существует"s);
        }

        const vector<string> words = SplitIntoWordsNoStop(document);



        for (const string& word : words) {
        	if (!IsValidWord(word)) {
        	    throw invalid_argument("Документ не был добавлен, так как содержит спецсимволы"s);
            }
        }


        const double inv_word_count = 1.0 / words.size();

        for (const string& word : words) {
        //если ключа нет, то сперва инициализирую нулем, для точности
        if (word_to_document_freqs_.count(word) == 0) {
            word_to_document_freqs_[word][document_id] = 0;
            word_to_document_freqs_[word][document_id] += inv_word_count;
         }

         else {
           		word_to_document_freqs_[word][document_id] += inv_word_count;
          }
         }
            documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });

    }


    template <typename DocumentPredicate>
    vector<Document> FindTopDocuments(const string& raw_query, DocumentPredicate document_predicate) const {

    	const auto query_words = ParseQuery(raw_query);


            auto matched_documents = FindAllDocuments(query_words, document_predicate);
            sort(matched_documents.begin(), matched_documents.end(),
                [](const Document& lhs, const Document& rhs) {
                    if (abs(lhs.relevance - rhs.relevance) < EPSILON) {
                        return lhs.rating > rhs.rating;
                    }
            return lhs.relevance > rhs.relevance;
                });

            if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
                matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
            }
            return matched_documents;
 }

    vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus status) const {
        return FindTopDocuments(raw_query, [&status](int document_id, DocumentStatus doc_status, int rating) {return status == doc_status; });
    }

    vector<Document> FindTopDocuments(const string& raw_query) const {
        return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
    }

    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query, int document_id) const {
        	const Query query = ParseQuery(raw_query);
            vector<string> matched_words;
            for (const string& word : query.plus_words) {
                if (word_to_document_freqs_.count(word) == 0) {
                    continue;
                }
                if (word_to_document_freqs_.at(word).count(document_id)) {
                    matched_words.push_back(word);
                }
            }
            for (const string& word : query.minus_words) {
                if (word_to_document_freqs_.count(word) == 0) {
                    continue;
                }
                if (word_to_document_freqs_.at(word).count(document_id)) {
                    matched_words.clear();
                    break;
                }
            }
            return { matched_words, documents_.at(document_id).status };
        }

private:
    struct Query {
        set<string> plus_words;
        set<string> minus_words;
    };

    struct DocumentData
    {
        int rating;
        DocumentStatus status;
    };

    set<string> stop_words_;
    map<string, map<int, double>> word_to_document_freqs_;
    map<int, DocumentData> documents_;


    template <typename Collection>
    vector<string> SplitIntoWords(const Collection text) const {
        vector<string> words;
        string word;
        for (const auto element : text) {
            if (element == ' ') {
                if (!word.empty()) {
                    words.push_back(word);
                    word.clear();
                }
            }
            else {
                word += element;
            }
        }
        if (!word.empty()) {
            words.push_back(word);
            word.clear();
        }

        return words;
    }


    static bool IsValidWord(const string& word) {
        return none_of(word.begin(), word.end(), [](char c) {
            return c >= '\0' && c < ' ';
            });
    }


    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }


    static int ComputeAverageRating(const vector<int>& ratings) {
            if (ratings.empty()) {
                return 0;
            }
            int rating_sum = 0;
            for (const int rating : ratings) {
                rating_sum += rating;
            }
            return rating_sum / static_cast<int>(ratings.size());
        }


    vector<string> SplitIntoWordsNoStop(const string& text) const {
        vector<string> words;
        for (const string& word : SplitIntoWords(text)) {
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }

        return words;
    }


Query ParseQuery(const string& text) const {
	Query query_words;

	for (const string& word : SplitIntoWordsNoStop(text)) {
        if (!IsValidWord(word)) {
            throw invalid_argument("В словах поискового запроса есть недопустимые символы"s);
        }

        if (word[word.size() - 1] == '-')  {
           	throw invalid_argument("Отсутствует текст после символа «минус» в поисковом запросе"s);
        }

        if (word[0] == '-' && word[1] == '-') {
           throw invalid_argument("Запрос содержит более чем один минус перед словами"s);
                    }

        if (word[0] != '-') {
            query_words.plus_words.insert(word);
        }

            string final_minus_word = word.substr(1);
            query_words.minus_words.insert(final_minus_word);
	}

    return query_words;

}


double ComputeWordInverseDocumentFreq(const string& word) const {
            return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
        }


        template <typename Function>
        vector<Document> FindAllDocuments(const Query& query, Function function) const {
            map<int, double> document_to_relevance;
            for (const string& word : query.plus_words) {
                if (word_to_document_freqs_.count(word) == 0) {
                    continue;
                }
                const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
                for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
                    const auto& document = documents_.at(document_id);
                    if (function(document_id, document.status, document.rating)) {
                        document_to_relevance[document_id] += term_freq * inverse_document_freq;
                    }
                }
            }

            for (const string& word : query.minus_words) {
                if (word_to_document_freqs_.count(word) == 0) {
                    continue;
                }
                for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
                    document_to_relevance.erase(document_id);
                }
            }

            vector<Document> matched_documents;
            for (const auto [document_id, relevance] : document_to_relevance) {
                matched_documents.push_back(
                    { document_id, relevance, documents_.at(document_id).rating });
            }
            return matched_documents;
        }

};

void PrintDocument(const Document& document) {
	cout << "{ "s
    << "document_id = "s << document.id << ", "s
	<< "relevance = "s << document.relevance << ", "s
	<< "rating = "s << document.rating << " }"s << endl;
}


int main() {
    SearchServer search_server("и в на"s);
    // Явно игнорируем результат метода AddDocument, чтобы избежать предупреждения
    // о неиспользуемом результате его вызова
    (void) search_server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, {7, 2, 7});
    search_server.AddDocument(1, "пушистый пёс и модный ошейник"s, DocumentStatus::ACTUAL, {1, 2});

    search_server.AddDocument(-1, "пушистый пёс и модный ошейник"s, DocumentStatus::ACTUAL, {1, 2});

    search_server.AddDocument(3, "большой пёс скво\x12рец"s, DocumentStatus::ACTUAL, {1, 3, 2});

    const auto documents = search_server.FindTopDocuments("--пушистый"s);
        for (const Document& document : documents) {
            PrintDocument(document);
        }
}
