#include "PropertyParser.h"
#include <gtest/gtest.h>
#include <vector>

// Структура для хранения данных о вызовах callback-функции
struct CallbackData {
    int callCount;
    std::vector<std::string> propertyNames;
    std::vector<std::string> propertyValues;
    std::vector<std::string> propertyMatches;
    std::vector<bool> isValidFlags;
    
    CallbackData() : callCount(0) {}
};

// Callback-функция для тестирования
void testCallback(void* data, const PropertyParser& parser) {
    CallbackData* callbackData = static_cast<CallbackData*>(data);
    callbackData->callCount++;
    callbackData->propertyNames.push_back(parser.getPropertyName());
    callbackData->propertyValues.push_back(parser.getPropertyValue());
    callbackData->propertyMatches.push_back(parser.getPropertyMatch());
    callbackData->isValidFlags.push_back(parser.isValid());
}

// Pattern matching tests
TEST(PropertyParserTest, ExactMatchPattern) {
    EXPECT_TRUE(PropertyParser::matchesPattern("com.example.MyTest", "com.example.MyTest"));
    EXPECT_FALSE(PropertyParser::matchesPattern("com.example.MyTest", "com.example.OtherTest"));
}

TEST(PropertyParserTest, WildcardStarPattern) {
    EXPECT_TRUE(PropertyParser::matchesPattern("com.example.MyTest", "com.example.*"));
    EXPECT_TRUE(PropertyParser::matchesPattern("com.example.subpackage.MyTest", "com.example.*"));
    EXPECT_FALSE(PropertyParser::matchesPattern("com.other.MyTest", "com.example.*"));
}

TEST(PropertyParserTest, WildcardQuestionPattern) {
    EXPECT_TRUE(PropertyParser::matchesPattern("MyTest", "My?est"));
    EXPECT_TRUE(PropertyParser::matchesPattern("MyTest", "??????"));
    EXPECT_FALSE(PropertyParser::matchesPattern("MyTest", "?????"));
}

TEST(PropertyParserTest, ComplexPattern) {
    EXPECT_TRUE(PropertyParser::matchesPattern("com.example.MyTest", "com.*.My?e??"));
    EXPECT_TRUE(PropertyParser::matchesPattern("com.example.subpackage.MyTest", "com.**.MyTest"));
    EXPECT_FALSE(PropertyParser::matchesPattern("com.example.MyTest", "com.*.Other?e??"));
}

TEST(PropertyParserTest, EdgeCasesPattern) {
    // Пустая строка и шаблон
    EXPECT_TRUE(PropertyParser::matchesPattern("", ""));
    EXPECT_TRUE(PropertyParser::matchesPattern("", "*"));
    EXPECT_FALSE(PropertyParser::matchesPattern("", "?"));
    
    // Только звездочки
    EXPECT_TRUE(PropertyParser::matchesPattern("anything", "***"));
    
    // Совпадение с пустым шаблоном
    EXPECT_TRUE(PropertyParser::matchesPattern("", ""));
    
    // Длинная строка
    EXPECT_TRUE(PropertyParser::matchesPattern("very.long.package.name.MyTest", "*.MyTest"));
}

// Case insensitive tests
TEST(PropertyParserTest, CaseInsensitivePatternMatching) {
    // Test case insensitive pattern matching
    EXPECT_TRUE(PropertyParser::matchesPattern("com.Example.MyTest", "com.example.*", false));
    EXPECT_TRUE(PropertyParser::matchesPattern("COM.EXAMPLE.SUBPACKAGE.MYTEST", "com.*.mytest", false));
    EXPECT_FALSE(PropertyParser::matchesPattern("com.other.MyTest", "com.example.*", false));
    
    // Test case sensitive pattern matching (default behavior)
    EXPECT_FALSE(PropertyParser::matchesPattern("com.Example.MyTest", "com.example.*"));
    EXPECT_TRUE(PropertyParser::matchesPattern("com.example.MyTest", "com.example.*"));
}

// Тесты для новой функциональности с callback-функцией
TEST(PropertyParserTest, FeedAndParseWithValidProperty) {
    CallbackData callbackData;
    PropertyParser parser(1024, false); // 1024 байт максимальный размер буфера
    
    parser.feedAndParse("name=value\n", 11, testCallback, &callbackData);
    
    EXPECT_EQ(callbackData.callCount, 1);
    EXPECT_EQ(callbackData.propertyNames.size(), 1);
    EXPECT_EQ(callbackData.propertyValues.size(), 1);
    EXPECT_EQ(callbackData.propertyMatches.size(), 1);
    EXPECT_EQ(callbackData.isValidFlags.size(), 1);
    
    EXPECT_EQ(callbackData.propertyNames[0], "name");
    EXPECT_EQ(callbackData.propertyValues[0], "value");
    EXPECT_EQ(callbackData.propertyMatches[0], "");
    EXPECT_TRUE(callbackData.isValidFlags[0]);
}

TEST(PropertyParserTest, FeedAndParseWithInvalidProperty) {
    CallbackData callbackData;
    PropertyParser parser(1024, false); // 1024 байт максимальный размер буфера
    
    parser.feedAndParse("invalid\n", 8, testCallback, &callbackData);
    
    EXPECT_EQ(callbackData.callCount, 1);
    EXPECT_EQ(callbackData.propertyNames.size(), 1);
    EXPECT_EQ(callbackData.propertyValues.size(), 1);
    EXPECT_EQ(callbackData.propertyMatches.size(), 1);
    EXPECT_EQ(callbackData.isValidFlags.size(), 1);
    
    EXPECT_EQ(callbackData.propertyNames[0], "");
    EXPECT_EQ(callbackData.propertyValues[0], "");
    EXPECT_EQ(callbackData.propertyMatches[0], "invalid");
    EXPECT_FALSE(callbackData.isValidFlags[0]);
}

TEST(PropertyParserTest, FeedAndParseMultipleProperties) {
    CallbackData callbackData;
    PropertyParser parser(1024, false); // 1024 байт максимальный размер буфера
    
    parser.feedAndParse("a=1\nb=2\nc=3\n", 12, testCallback, &callbackData);
    
    EXPECT_EQ(callbackData.callCount, 3);
    EXPECT_EQ(callbackData.propertyNames.size(), 3);
    EXPECT_EQ(callbackData.propertyValues.size(), 3);
    EXPECT_EQ(callbackData.propertyMatches.size(), 3);
    EXPECT_EQ(callbackData.isValidFlags.size(), 3);
    
    // First property
    EXPECT_EQ(callbackData.propertyNames[0], "a");
    EXPECT_EQ(callbackData.propertyValues[0], "1");
    EXPECT_EQ(callbackData.propertyMatches[0], "");
    EXPECT_TRUE(callbackData.isValidFlags[0]);
    
    // Second property
    EXPECT_EQ(callbackData.propertyNames[1], "b");
    EXPECT_EQ(callbackData.propertyValues[1], "2");
    EXPECT_EQ(callbackData.propertyMatches[1], "");
    EXPECT_TRUE(callbackData.isValidFlags[1]);
    
    // Third property
    EXPECT_EQ(callbackData.propertyNames[2], "c");
    EXPECT_EQ(callbackData.propertyValues[2], "3");
    EXPECT_EQ(callbackData.propertyMatches[2], "");
    EXPECT_TRUE(callbackData.isValidFlags[2]);
}

TEST(PropertyParserTest, FeedAndParseMixedValidAndInvalid) {
    CallbackData callbackData;
    PropertyParser parser(1024, false); // 1024 байт максимальный размер буфера
    
    parser.feedAndParse("valid=value\ninvalid\n", 20, testCallback, &callbackData);
    
    EXPECT_EQ(callbackData.callCount, 2);
    EXPECT_EQ(callbackData.propertyNames.size(), 2);
    EXPECT_EQ(callbackData.propertyValues.size(), 2);
    EXPECT_EQ(callbackData.propertyMatches.size(), 2);
    EXPECT_EQ(callbackData.isValidFlags.size(), 2);
    
    // First (valid) property
    EXPECT_EQ(callbackData.propertyNames[0], "valid");
    EXPECT_EQ(callbackData.propertyValues[0], "value");
    EXPECT_EQ(callbackData.propertyMatches[0], "");
    EXPECT_TRUE(callbackData.isValidFlags[0]);
    
    // Second (invalid) property
    EXPECT_EQ(callbackData.propertyNames[1], "");
    EXPECT_EQ(callbackData.propertyValues[1], "");
    EXPECT_EQ(callbackData.propertyMatches[1], "invalid");
    EXPECT_FALSE(callbackData.isValidFlags[1]);
}

TEST(PropertyParserTest, FeedAndParseWithNoCallback) {
    PropertyParser parser(1024, false); // 1024 байт максимальный размер буфера
    
    // Should not crash and should parse correctly
    parser.feedAndParse("name=value\n", 11);
    
    // After feedAndParse with complete token, the parser state should be reset
    EXPECT_FALSE(parser.isValid());
    EXPECT_EQ(parser.getPropertyName(), "");
    EXPECT_EQ(parser.getPropertyValue(), "");
}

TEST(PropertyParserTest, FeedAndParseEmptyData) {
    CallbackData callbackData;
    PropertyParser parser(1024, false); // 1024 байт максимальный размер буфера
    
    parser.feedAndParse("", 0, testCallback, &callbackData);
    
    // No callback should be called for empty data
    EXPECT_EQ(callbackData.callCount, 0);
}

TEST(PropertyParserTest, FeedAndParsePartialData) {
    CallbackData callbackData;
    PropertyParser parser(1024, false); // 1024 байт максимальный размер буфера
    
    // Feed partial data without terminator
    parser.feedAndParse("partial=token", 13, testCallback, &callbackData);
    
    // No callback should be called because there's no complete token
    EXPECT_EQ(callbackData.callCount, 0);
    
    // Now feed the terminator
    parser.feedAndParse("\n", 1, testCallback, &callbackData);
    
    // Now callback should be called
    EXPECT_EQ(callbackData.callCount, 1);
    EXPECT_EQ(callbackData.propertyNames[0], "partial");
    EXPECT_EQ(callbackData.propertyValues[0], "token");
    EXPECT_TRUE(callbackData.isValidFlags[0]);
}

TEST(PropertyParserTest, FeedAndParseCaseInsensitive) {
    CallbackData callbackData;
    PropertyParser parser(1024, true); // 1024 байт максимальный размер буфера, case insensitive mode
    
    parser.feedAndParse("Name=Value\n", 11, testCallback, &callbackData);
    
    EXPECT_EQ(callbackData.callCount, 1);
    EXPECT_EQ(callbackData.propertyNames[0], "name"); // Should be lowercase
    EXPECT_EQ(callbackData.propertyValues[0], "Value"); // Value should remain unchanged
    EXPECT_TRUE(callbackData.isValidFlags[0]);
}

TEST(PropertyParserTest, FeedAndParseCaseInsensitivePropertyMatch) {
    CallbackData callbackData;
    PropertyParser parser(1024, true); // 1024 байт максимальный размер буфера, case insensitive mode
    
    parser.feedAndParse("InvalidString\n", 14, testCallback, &callbackData);
    
    EXPECT_EQ(callbackData.callCount, 1);
    EXPECT_EQ(callbackData.propertyNames[0], ""); // No property name for invalid tokens
    EXPECT_EQ(callbackData.propertyMatches[0], "invalidstring"); // Should be lowercase
    EXPECT_FALSE(callbackData.isValidFlags[0]);
}

// Тест для проверки обработки данных размером больше, чем размер буфера
TEST(PropertyParserTest, FeedAndParseLargeData) {
    CallbackData callbackData;
    PropertyParser parser(10, false); // Маленький буфер размером 10 байт
    
    // Данные размером больше, чем размер буфера
    std::string largeData = "a=1\nb=2\nc=3\nd=4\ne=5\nf=6\ng=7\nh=8\ni=9\nj=10\nk=11\nl=12\nm=13\nn=14\no=15\n";
    
    parser.feedAndParse(largeData.c_str(), largeData.length(), testCallback, &callbackData);
    
    // Проверим, что все свойства были обработаны
    EXPECT_EQ(callbackData.callCount, 15);
    EXPECT_EQ(callbackData.propertyNames.size(), 15);
    EXPECT_EQ(callbackData.propertyValues.size(), 15);
    EXPECT_EQ(callbackData.propertyMatches.size(), 15);
    EXPECT_EQ(callbackData.isValidFlags.size(), 15);
    
    // Проверим первые несколько свойств
    EXPECT_EQ(callbackData.propertyNames[0], "a");
    EXPECT_EQ(callbackData.propertyValues[0], "1");
    EXPECT_EQ(callbackData.propertyNames[1], "b");
    EXPECT_EQ(callbackData.propertyValues[1], "2");
    EXPECT_EQ(callbackData.propertyNames[2], "c");
    EXPECT_EQ(callbackData.propertyValues[2], "3");
}

// Тест для проверки обработки некорректных данных размером больше, чем размер буфера
TEST(PropertyParserTest, FeedAndParseLargeInvalidData) {
    CallbackData callbackData;
    PropertyParser parser(10, false); // Маленький буфер размером 10 байт
    
    // Некорректные данные размером больше, чем размер буфера
    std::string largeInvalidData = "invalid1\ninvalid2\ninvalid3\ninvalid4\ninvalid5\ninvalid6\ninvalid7\ninvalid8\ninvalid9\ninvalid10\ninvalid11\ninvalid12\ninvalid13\ninvalid14\ninvalid15\n";
    
    parser.feedAndParse(largeInvalidData.c_str(), largeInvalidData.length(), testCallback, &callbackData);
    
    // Проверим, что все строки были обработаны
    EXPECT_EQ(callbackData.callCount, 15);
    EXPECT_EQ(callbackData.propertyNames.size(), 15);
    EXPECT_EQ(callbackData.propertyValues.size(), 15);
    EXPECT_EQ(callbackData.propertyMatches.size(), 15);
    EXPECT_EQ(callbackData.isValidFlags.size(), 15);
    
    // Проверим, что все строки являются некорректными (isValid = false)
    for (int i = 0; i < 15; i++) {
        EXPECT_FALSE(callbackData.isValidFlags[i]);
        EXPECT_EQ(callbackData.propertyNames[i], "");
        EXPECT_EQ(callbackData.propertyValues[i], "");
    }
    
    // Проверим первые несколько значений propertyMatches
    EXPECT_EQ(callbackData.propertyMatches[0], "invalid1");
    EXPECT_EQ(callbackData.propertyMatches[1], "invalid2");
    EXPECT_EQ(callbackData.propertyMatches[2], "invalid3");
}

// Тест для проверки обработки одной длинной некорректной строки размером больше, чем размер буфера
TEST(PropertyParserTest, FeedAndParseSingleLargeInvalidLine) {
    CallbackData callbackData;
    PropertyParser parser(10, false); // Маленький буфер размером 10 байт
    
    // Одна длинная некорректная строка размером больше, чем размер буфера
    std::string singleLargeInvalidLine = "this_is_a_very_long_invalid_line_that_exceeds_the_buffer_size_and_should_be_processed_correctly\n";
    
    parser.feedAndParse(singleLargeInvalidLine.c_str(), singleLargeInvalidLine.length(), testCallback, &callbackData);
    
    // Проверим, что строка была обработана частями
    // Длина строки 85 символов, размер буфера 10 символов, значит будет 9 вызовов callback
    EXPECT_EQ(callbackData.callCount, 10);
    EXPECT_EQ(callbackData.propertyNames.size(), 10);
    EXPECT_EQ(callbackData.propertyValues.size(), 10);
    EXPECT_EQ(callbackData.propertyMatches.size(), 10);
    EXPECT_EQ(callbackData.isValidFlags.size(), 10);
    
    // Проверим, что все части являются некорректными (isValid = false)
    for (int i = 0; i < 9; i++) {
        EXPECT_FALSE(callbackData.isValidFlags[i]);
        EXPECT_EQ(callbackData.propertyNames[i], "");
        EXPECT_EQ(callbackData.propertyValues[i], "");
    }
    
    // Проверим значения propertyMatches для первых нескольких частей
    EXPECT_EQ(callbackData.propertyMatches[0], "this_is_a_");
}
