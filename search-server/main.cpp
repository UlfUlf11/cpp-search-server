#include "search_server.h"
#include <string>
#include <utility>
#include <iostream>
#include <vector>
#include "read_input_functions.h" 
#include "request_queue.h"
#include "paginator.h"
#include "test_example_functions.h"


using namespace std;


ostream& operator<<(ostream &os, const Document& doc) {
    return os <<  "{ document_id = "s << doc.id << ", relevance = "s << doc.relevance << ", rating = "s << doc.rating << " }"s;
}


template <typename Iterator>
std::ostream& operator<< (std::ostream &out, const Paginator<Iterator> &p) {
    for (auto it = p.begin(); distance(it, p.end()) > 0; advance(p)) {
        out << *it;
    }
    return out;
}


template <typename Iterator>
std::ostream& operator<< (std::ostream &out, const IteratorRange<Iterator> &range) {
using namespace std;
    for (Iterator currentIt = range.begin(); currentIt < range.end(); ++currentIt) {
        out << *currentIt;
    }
    return out;
}

 
int main() {

    SearchServer search_server("and in at"s);
    RequestQueue request_queue(search_server);
    search_server.AddDocument(1, "curly cat curly tail"s, DocumentStatus::ACTUAL, {7, 2, 7});
    search_server.AddDocument(2, "curly dog and fancy collar"s, DocumentStatus::ACTUAL, {1, 2, 3});
    search_server.AddDocument(3, "big cat fancy collar "s, DocumentStatus::ACTUAL, {1, 2, 8});
    search_server.AddDocument(4, "big dog sparrow Eugene"s, DocumentStatus::ACTUAL, {1, 3, 2});
    search_server.AddDocument(5, "big dog sparrow Vasiliy"s, DocumentStatus::ACTUAL, {1, 1, 1});
    
    
    TestSearchServer();
    // Если вы видите эту строку, значит все тесты прошли успешно
    cout << "Search server testing finished"s << endl;
    
        const auto search_results = search_server.FindTopDocuments("curly dog"s);
    int page_size = 2;
    const auto pages = Paginate(search_results, page_size);
    
    // Выводим найденные документы по страницам
    for (auto page = pages.begin(); page != pages.end(); ++page) {
        cout << *page << endl;
        cout << "Page break"s << endl;
    }
    

    // 1439 запросов с нулевым результатом
    for (int i = 0; i < 1439; ++i) {
        request_queue.AddFindRequest("empty request"s);
    }
    // все еще 1439 запросов с нулевым результатом
    request_queue.AddFindRequest("curly dog"s);
    // новые сутки, первый запрос удален, 1438 запросов с нулевым результатом
    request_queue.AddFindRequest("big collar"s);
    // первый запрос удален, 1437 запросов с нулевым результатом
    request_queue.AddFindRequest("sparrow"s);
    cout << "Total empty requests: "s << request_queue.GetNoResultRequests() << endl;       
    return 0;
}