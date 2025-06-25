#pragma once

#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>
#include <vector>
#include <memory>
#include "EventQueue.h"

/**
 * @brief Represents a single RSS feed item
 */
struct RssItem {
    String title;           ///< Article title
    String description;     ///< Article description/summary
    String content;         ///< Full article content (from content:encoded)
    String link;            ///< Article URL
    String guid;            ///< Unique identifier
    String pubDate;         ///< Publication date
    bool isNew;             ///< Whether this is a new item
    
    RssItem() : isNew(false) {}
};

/**
 * @brief RSS feed client for fetching and parsing RSS feeds
 * 
 * Currently optimized for Substack feeds but can be extended for other RSS sources.
 * Handles HTTP requests, XML parsing, and content extraction.
 */
class RssClient {
public:
    /**
     * @brief Constructor
     * @param eventQueue Reference to event queue for notifications
     */
    RssClient(EventQueue& eventQueue);
    
    /**
     * @brief Destructor
     */
    ~RssClient();
    
    /**
     * @brief Set the RSS feed URL
     * @param url The RSS feed URL to fetch from
     */
    void setFeedUrl(const String& url);
    
    /**
     * @brief Get the current feed URL
     * @return The current feed URL
     */
    String getFeedUrl() const { return _feedUrl; }
    
    /**
     * @brief Fetch and parse the RSS feed
     * @return true if successful, false otherwise
     */
    bool fetchFeed();
    
    /**
     * @brief Get the latest feed items
     * @return Vector of RSS items
     */
    const std::vector<RssItem>& getItems() const { return _items; }
    
    /**
     * @brief Get the most recent item
     * @return Pointer to the most recent item, or nullptr if no items
     */
    const RssItem* getLatestItem() const;
    
    /**
     * @brief Check if there are new items since last check
     * @return true if there are new items
     */
    bool hasNewItems() const;
    
    /**
     * @brief Mark all items as seen
     */
    void markItemsAsSeen();
    
    /**
     * @brief Get the last seen GUID
     * @return The GUID of the last seen item
     */
    String getLastSeenGuid() const { return _lastSeenGuid; }
    
    /**
     * @brief Set the last seen GUID
     * @param guid The GUID to mark as last seen
     */
    void setLastSeenGuid(const String& guid);
    
    /**
     * @brief Clear all cached items
     */
    void clearItems();
    
    /**
     * @brief Check if the RSS client is ready to fetch feeds
     * @return true if WiFi is connected and URL is set
     */
    bool isReady() const;

private:
    EventQueue& _eventQueue;                    ///< Event queue for notifications
    String _feedUrl;                           ///< RSS feed URL
    std::vector<RssItem> _items;               ///< Cached RSS items
    String _lastSeenGuid;                      ///< GUID of last seen item
    HTTPClient _httpClient;                    ///< HTTP client for requests
    
    /**
     * @brief Parse RSS XML content
     * @param xmlContent The XML content to parse
     * @return true if parsing was successful
     */
    bool parseRssXml(const String& xmlContent);
    
    /**
     * @brief Extract text content from HTML
     * @param html The HTML content to clean
     * @return Cleaned text content
     */
    String extractTextFromHtml(const String& html);
    
    /**
     * @brief Find the next XML tag
     * @param content The XML content to search
     * @param startPos Starting position
     * @param tagName Output parameter for tag name
     * @param tagContent Output parameter for tag content
     * @return Position after the tag, or -1 if not found
     */
    int findNextTag(const String& content, int startPos, String& tagName, String& tagContent);
    
    /**
     * @brief Decode HTML entities
     * @param html The HTML string to decode
     * @return Decoded string
     */
    String decodeHtmlEntities(const String& html);
    
    /**
     * @brief Extract content from XML tag with CDATA handling
     * @param xmlContent The XML content to search
     * @param tagName The tag name to extract
     * @return The extracted content
     */
    String extractTagContent(const String& xmlContent, const String& tagName);
}; 