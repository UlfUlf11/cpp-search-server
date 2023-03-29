#pragma once

#include "search_server.h"
#include "document.h"

#include <deque>
#include <set>
#include <string>
#include <vector>


class RequestQueue {
public:
    explicit RequestQueue(const SearchServer& search_server);
      
    void PopFirstElement();
    
    // сделаем "обёртки" для всех методов поиска, чтобы сохранять результаты для нашей статистики
    template <typename DocumentPredicate>
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate);
    
    
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentStatus status);
    
    
    std::vector<Document> AddFindRequest(const std::string& raw_query);
    
    
    int GetNoResultRequests() const;
    
private:
    struct QueryResult {
        int result_num;
    };
    std::deque<QueryResult> requests_;
    
    const static int min_in_day_ = 1440;
    const SearchServer&  search_server_;
    int no_results_requests_;
};


    // сделаем "обёртки" для всех методов поиска, чтобы сохранять результаты для нашей статистики
    template <typename DocumentPredicate>
    std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate) {
        
         auto request = search_server_.FindTopDocuments(raw_query, document_predicate);
         if ( requests_.size() >= min_in_day_ ) { // Удаляем первый запрос
             PopFirstElement();
             --no_results_requests_;
         }
         if ( request.empty() ) {
             ++no_results_requests_;
         }
        requests_.push_front({static_cast<int>(requests_.size())}); //Добавляем в дэк индекс запроса
        return request;
    }