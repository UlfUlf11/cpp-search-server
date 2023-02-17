// -------- Начало модульных тестов поисковой системы ----------



ostream& operator<< (ostream& out, const DocumentStatus& t)
{
   switch(t) {
      case DocumentStatus::ACTUAL: {
           string s = "ACTUAL"s;
           out << s;
           return out ;
           break;
         }
      case DocumentStatus::IRRELEVANT:{
           string s = "IRRELEVANT"s;
           out << s;
           return out ;
           break;
      }
      case DocumentStatus::BANNED: {
           string s = "BANNED"s;
           out << s;
           return out ;
           break;
      }

      case DocumentStatus::REMOVED: {
           string s = "REMOVED"s;
           out << s;
          return out ;
          break;
      }
   }
      return out ;
}


// Тест проверяет, что поисковая система исключает стоп-слова при добавлении документов
void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs.size(), 1);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }

    {
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("in"s).empty(),
                    "Stop words must be excluded from documents"s);
    }
}


//Добавление документов. Добавленный документ должен находиться по поисковому запросу,
//который содержит слова из документа.
void TestAddDocument()
{
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };

    // добавляем документ и проверяем что его можно найти по любому слову из документа
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("cat in the city"s);
        ASSERT_EQUAL(found_docs.size(), 1);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }
}

//Поддержка стоп-слов. Стоп-слова исключаются из текста документов.
void TestStopWords()
{
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };

    {
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT(server.FindTopDocuments("in"s).empty());
    }

}

//Поддержка минус-слов.
//Документы, содержащие минус-слова поискового запроса, не должны включаться в результаты поиска.
void TestMinusWords()
{
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };

    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("cat in the city"s);
        ASSERT_EQUAL(found_docs.size(), 1);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }

    //добавляем минус слово
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT(server.FindTopDocuments("-in"s).empty());
    }

}



//Матчинг документов. При матчинге документа по поисковому запросу должны быть возвращены все слова из поискового запроса,
//присутствующие в документе. Если есть соответствие хотя бы по одному минус-слову, должен возвращаться пустой список слов.
void TestMatchedDocuments()
{
    {
        SearchServer server;
        server.AddDocument(0, "white cat and fancy collar"s, DocumentStatus::ACTUAL, { 1, 2, -3 });
        const int document_count = server.GetDocumentCount();
        for (int document_id = 0; document_id < document_count; ++document_id)
        {
            //сверяем запрос с документом
            const auto& [words, status] = server.MatchDocument("fluffy cat"s, document_id);
            ASSERT_EQUAL(words.size(), 1);
            ASSERT_EQUAL(document_id, 0);
            ASSERT_EQUAL(status, DocumentStatus::ACTUAL);
        }
    }

    {
        SearchServer server;
        server.SetStopWords("cat"s);
        server.AddDocument(0, "white cat and fancy collar"s, DocumentStatus::ACTUAL, {1, 2, -3});
        const int document_count = server.GetDocumentCount();
        for (int document_id = 0; document_id < document_count; ++document_id)
        {
            //сверяем запрос с документом, но накладываем еще стоп слово
            const auto& [words, status] = server.MatchDocument("fluffy cat"s, document_id);
            ASSERT_EQUAL(words.size(), 0);
            ASSERT_EQUAL(document_id, 0);
            ASSERT_EQUAL(status, DocumentStatus::ACTUAL);
        }
    }

    {
        SearchServer server;
        server.AddDocument(0, "white cat and fancy collar"s, DocumentStatus::ACTUAL, { 1, 2, -3 });
        const int document_count = server.GetDocumentCount();
        for (int document_id = 0; document_id < document_count; ++document_id)
        {
            //сверяем запрос с документом, но добавляем минус слово в запрос
            const auto& [words, status] = server.MatchDocument("fluffy -cat"s, document_id);
            ASSERT_EQUAL(words.size(), 0);
            ASSERT_EQUAL(document_id, 0);
            ASSERT_EQUAL(status, DocumentStatus::ACTUAL);
        }
    }
}

//Сортировка найденных документов по релевантности.
//Возвращаемые при поиске документов результаты должны быть отсортированы в порядке убывания релевантности.
void TestSortFindedDocumentsByRelevance()
{
    {
        SearchServer server;

        server.AddDocument(0, "white cat and fancy collar"s, DocumentStatus::ACTUAL, { 8, -3 });
        server.AddDocument(1, "fluffy cat fluffy tail"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
        server.AddDocument(2, "soigne dog expressive eyes"s, DocumentStatus::ACTUAL, { 5, -12, 2 });
        server.AddDocument(3, "soigne starling eugeny"s, DocumentStatus::BANNED, { 9 });

        const auto& documents = server.FindTopDocuments("fluffy soigne cat"s);

        int doc_size = static_cast<int>(documents.size());
        for (int i = 0; i < doc_size; ++i)
        {
        	ASSERT(documents[i].relevance > documents[i + 1].relevance);
        }
    }

}


//тест проверяет, что функция возвращает документы с использованием предикаты, задаваемой пользователем
void TestFindByPredicate()
{

    {
        SearchServer server;

        server.AddDocument(0, "white cat and fancy collar"s, DocumentStatus::ACTUAL, { 8, -3 });
        server.AddDocument(1, "fluffy cat fluffy tail"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
        server.AddDocument(2, "soigne dog expressive eyes"s, DocumentStatus::ACTUAL, { 5, -12, 2 });
        server.AddDocument(3, "soigne starling eugeny"s, DocumentStatus::BANNED, { 9 });


        const auto& found_docs = server.FindTopDocuments("fluffy soigne cat"s, [](int document_id, DocumentStatus status, int rating) { return status == DocumentStatus::BANNED; }) ;

        {
            //сверяем запрос с документом
            ASSERT_EQUAL(found_docs.size(), 1);
            const Document& doc0 = found_docs[0];
            ASSERT_EQUAL(doc0.id, 3);
        }
    }

}


void TestFindByStatus()
{
    {
        SearchServer server;

        server.AddDocument(0, "white cat and fancy collar"s, DocumentStatus::ACTUAL, { 8, -3 });
        server.AddDocument(1, "fluffy cat fluffy tail"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
        server.AddDocument(2, "soigne dog expressive eyes"s, DocumentStatus::ACTUAL, { 5, -12, 2 });
        server.AddDocument(3, "soigne starling eugeny"s, DocumentStatus::BANNED, { 9 });

        const auto& found_docs = server.FindTopDocuments("fluffy soigne cat"s, DocumentStatus::BANNED);

        {
            //сверяем запрос с документом
            ASSERT_EQUAL(found_docs.size(), 1);
            const Document& doc0 = found_docs[0];
            ASSERT_EQUAL(doc0.id, 3);
        }
    }

    {
        SearchServer server;

        server.AddDocument(0, "white cat and fancy collar"s, DocumentStatus::ACTUAL, { 8, -3 });
        server.AddDocument(1, "fluffy cat fluffy tail"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
        server.AddDocument(2, "soigne dog expressive eyes"s, DocumentStatus::ACTUAL, { 5, -12, 2 });
        server.AddDocument(3, "soigne starling eugeny"s, DocumentStatus::BANNED, { 9 });

        const auto& found_docs = server.FindTopDocuments("fluffy soigne cat"s, DocumentStatus::ACTUAL);

        {
            //сверяем запрос с документами
            ASSERT_EQUAL(found_docs.size(), 3);
        }
    }

    {
        SearchServer server;

        server.AddDocument(0, "white cat and fancy collar"s, DocumentStatus::ACTUAL, { 8, -3 });
        server.AddDocument(1, "fluffy cat fluffy tail"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
        server.AddDocument(2, "soigne dog expressive eyes"s, DocumentStatus::ACTUAL, { 5, -12, 2 });
        server.AddDocument(3, "soigne starling eugeny"s, DocumentStatus::BANNED, { 9 });



        {
            ASSERT(server.FindTopDocuments("fluffy soigne cat"s, DocumentStatus::IRRELEVANT).empty());
        }
    }

    {
        SearchServer server;

        server.AddDocument(0, "white cat and fancy collar"s, DocumentStatus::ACTUAL, { 8, -3 });
        server.AddDocument(1, "fluffy cat fluffy tail"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
        server.AddDocument(2, "soigne dog expressive eyes"s, DocumentStatus::ACTUAL, { 5, -12, 2 });
        server.AddDocument(3, "soigne starling eugeny"s, DocumentStatus::BANNED, { 9 });



        {
            ASSERT(server.FindTopDocuments("fluffy soigne cat"s, DocumentStatus::REMOVED).empty());
        }
    }

}



void TestComputeAverageRating() {
    SearchServer server;

    server.AddDocument(0, "white cat and fancy collar"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
    server.AddDocument(1, "fluffy cat fluffy tail"s, DocumentStatus::ACTUAL, { -7, -2, -7 });
    server.AddDocument(2, "soigne dog expressive eyes"s, DocumentStatus::ACTUAL, { 5, 12, -2 });

    {
        const auto found_docs = server.FindTopDocuments("white"s);
        ASSERT_EQUAL(found_docs[0].rating, (1 + 2 + 3) / 3);
    }

    {
        const auto found_docs = server.FindTopDocuments("tail"s);
        ASSERT_EQUAL(found_docs[0].rating, (-7 -2 - 7) / 3);
    }

    {
        const auto found_docs = server.FindTopDocuments("dog"s);
        ASSERT_EQUAL(found_docs[0].rating, (5 + 12 - 2) / 3);
    }
}


void TestComputeRelevance() {
    const auto EPSILON = 1e-6;
    SearchServer server;

    server.AddDocument(0, "white cat and fancy collar"s, DocumentStatus::ACTUAL, { 1, 2, -3 });

    const auto& documents = server.FindTopDocuments("fluffy soigne cat"s);

    //первых двух слов запроса в документах нет, TF слова "cat" = 0.2 , IDF = log(1)
    double relevance = 0.2 * log(1);

    {
        ASSERT(abs(documents[0].relevance - relevance) < EPSILON );
    }
}



// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer() {
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    // Не забудьте вызывать остальные тесты здесь
    RUN_TEST(TestAddDocument);
    RUN_TEST(TestStopWords);
    RUN_TEST(TestMinusWords);
    RUN_TEST(TestMatchedDocuments);
    RUN_TEST(TestSortFindedDocumentsByRelevance);
    RUN_TEST(TestFindByPredicate);
    RUN_TEST(TestFindByStatus);
    RUN_TEST(TestComputeAverageRating);
    RUN_TEST(TestComputeRelevance);
}

// --------- Окончание модульных тестов поисковой системы -----------
