#include "remove_duplicates.h"


void RemoveDuplicates(SearchServer& search_server) {
    //поле хранит наборы слов (без учета частоты)
    std::set<std::set<std::string>> words_set;
    //поле заполняется id дупликатов
    std::set<int> duplicates;
    for (const int id : search_server) {
        //получаю набор слов для id
        std::map<std::string, double> word_frequencies = search_server.GetWordFrequencies(id);
        //текущий набор слов
        std::set<std::string> words;
        //заполняю текущий набор слов
        for (const auto& word : word_frequencies) {
            words.insert(word.first);
        }
        //если такой набор слов уже есть...
        if (words_set.count(words)) {
            duplicates.insert(id);
        }
        //если набора слов нет, то...
        else {
            words_set.insert(words);
        }
    }

    for (const int duplicat_id : duplicates) {
        std::cout << "Found duplicate document id " << duplicat_id << std::endl;
        search_server.RemoveDocument(duplicat_id);
    }
}
