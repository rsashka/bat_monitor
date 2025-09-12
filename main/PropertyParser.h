#ifndef PROPERTY_PARSER_H
#define PROPERTY_PARSER_H

#include <vector>
#include <string>
#include <cstddef>

// Forward declaration for callback function
class PropertyParser;

// Callback function type: takes a void pointer and a reference to the parser object
typedef void (*PropertyParserCallback)(void*, const PropertyParser&);

class PropertyParser {
public:
    PropertyParser(size_t maxBufferSize, bool caseInsensitive = false);
    
    // Feed data to the parser and immediately try to parse with callback
    void feedAndParse(const char* data, size_t length, PropertyParserCallback callback = nullptr, void* callbackData = nullptr);
    
    // Parse next token
    bool parseNext();
    
    // Get parsing results
    bool isValid() const;
    const std::string& getPropertyName() const;
    const std::string& getPropertyValue() const;
    const std::string& getPropertyMatch() const;
    
    // Clear parser state
    void reset();
    
    // Pattern matching functionality
    static bool matchesPattern(const std::string& str, const std::string& pattern, bool caseSensitive = true);

private:
    std::vector<char> m_buffer;
    std::string m_propertyName;
    std::string m_propertyValue;
    std::string m_propertyMatch;
    bool m_isValid;
    bool m_caseInsensitive;
    
    bool findTokenBoundary(size_t& start, size_t& end);
    bool parseToken(const std::vector<char>& token);
};

#endif // PROPERTY_PARSER_H
