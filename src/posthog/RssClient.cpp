#include "RssClient.h"
#include "EventQueue.h"
#include <WiFi.h>

RssClient::RssClient(EventQueue& eventQueue) 
    : _eventQueue(eventQueue), _feedUrl("") {
}

RssClient::~RssClient() {
    _httpClient.end();
}

void RssClient::setFeedUrl(const String& url) {
    _feedUrl = url;
}

bool RssClient::fetchFeed() {
    if (!isReady()) {
        Serial.println("RSS Client not ready - WiFi not connected or URL not set");
        return false;
    }
    
    Serial.printf("Fetching RSS feed from: %s\n", _feedUrl.c_str());
    
    _httpClient.begin(_feedUrl);
    _httpClient.setTimeout(10000); // 10 second timeout
    
    int httpCode = _httpClient.GET();
    
    if (httpCode != HTTP_CODE_OK) {
        Serial.printf("HTTP request failed, error: %d\n", httpCode);
        _httpClient.end();
        return false;
    }
    
    String payload = _httpClient.getString();
    _httpClient.end();
    
    if (payload.length() == 0) {
        Serial.println("Empty response received");
        return false;
    }
    
    Serial.printf("Received %d bytes of RSS data\n", payload.length());
    
    // Clear previous items
    _items.clear();
    
    // Parse the RSS XML
    if (!parseRssXml(payload)) {
        Serial.println("Failed to parse RSS XML");
        return false;
    }
    
    // Check for new items
    bool hasNew = false;
    for (auto& item : _items) {
        if (item.guid != _lastSeenGuid) {
            item.isNew = true;
            hasNew = true;
        }
    }
    
    if (hasNew) {
        Serial.printf("Found %d new RSS items\n", std::count_if(_items.begin(), _items.end(), 
            [](const RssItem& item) { return item.isNew; }));
    }
    
    return true;
}

const RssItem* RssClient::getLatestItem() const {
    if (_items.empty()) {
        return nullptr;
    }
    return &_items[0]; // Items are stored in reverse chronological order
}

bool RssClient::hasNewItems() const {
    return std::any_of(_items.begin(), _items.end(), 
        [](const RssItem& item) { return item.isNew; });
}

void RssClient::markItemsAsSeen() {
    if (!_items.empty()) {
        _lastSeenGuid = _items[0].guid;
        for (auto& item : _items) {
            item.isNew = false;
        }
    }
}

void RssClient::setLastSeenGuid(const String& guid) {
    _lastSeenGuid = guid;
}

void RssClient::clearItems() {
    _items.clear();
}

bool RssClient::isReady() const {
    return WiFi.status() == WL_CONNECTED && _feedUrl.length() > 0;
}

bool RssClient::parseRssXml(const String& xmlContent) {
    // Simple XML parser for RSS feeds
    // This is a basic implementation - could be enhanced with a proper XML library
    
    Serial.println("RssClient: Starting XML parsing...");
    
    int pos = 0;
    String tagName, tagContent;
    
    // Find the channel tag
    pos = xmlContent.indexOf("<channel>");
    if (pos == -1) {
        Serial.println("No <channel> tag found in RSS");
        return false;
    }
    
    Serial.println("RssClient: Found channel tag, looking for items...");
    
    // Find all item tags within the channel
    int itemCount = 0;
    while ((pos = findNextTag(xmlContent, pos, tagName, tagContent)) != -1) {
        if (tagName == "item") {
            itemCount++;
            Serial.printf("RssClient: Found item %d\n", itemCount);
            
            // Parse this item
            RssItem item;
            int itemStart = xmlContent.indexOf("<item>", pos - tagContent.length() - 7);
            int itemEnd = xmlContent.indexOf("</item>", itemStart);
            
            if (itemStart != -1 && itemEnd != -1) {
                String itemContent = xmlContent.substring(itemStart, itemEnd + 7);
                
                // Extract item fields
                String title, description, content, link, guid, pubDate;
                
                int fieldPos = 0;
                String fieldName, fieldContent;
                
                while ((fieldPos = findNextTag(itemContent, fieldPos, fieldName, fieldContent)) != -1) {
                    if (fieldName == "title") {
                        title = decodeHtmlEntities(fieldContent);
                        Serial.printf("RssClient: Found title: %s\n", title.c_str());
                    } else if (fieldName == "description") {
                        description = decodeHtmlEntities(fieldContent);
                    } else if (fieldName == "content:encoded") {
                        content = extractTextFromHtml(decodeHtmlEntities(fieldContent));
                        Serial.printf("RssClient: Found content:encoded, length: %d\n", fieldContent.length());
                        if (fieldContent.length() > 0) {
                            Serial.println("Content preview: " + fieldContent.substring(0, min(100, (int)fieldContent.length())));
                        }
                    } else if (fieldName == "link") {
                        link = fieldContent;
                    } else if (fieldName == "guid") {
                        guid = fieldContent;
                    } else if (fieldName == "pubDate") {
                        pubDate = fieldContent;
                    }
                }
                
                // Only add items with required fields
                if (guid.length() > 0 && title.length() > 0) {
                    item.title = title;
                    item.description = description;
                    item.content = content;
                    item.link = link;
                    item.guid = guid;
                    item.pubDate = pubDate;
                    item.isNew = (guid != _lastSeenGuid);
                    
                    _items.push_back(item);
                    Serial.printf("RssClient: Added item with title: %s\n", title.c_str());
                } else {
                    Serial.printf("RssClient: Skipping item - guid: %s, title: %s\n", guid.c_str(), title.c_str());
                }
            }
        }
    }
    
    // Sort items by publication date (newest first)
    std::sort(_items.begin(), _items.end(), 
        [](const RssItem& a, const RssItem& b) {
            return a.pubDate > b.pubDate;
        });
    
    Serial.printf("RssClient: Parsed %d RSS items\n", _items.size());
    return true;
}

String RssClient::extractTextFromHtml(const String& html) {
    String result = html;
    
    // Remove HTML tags
    int start = 0;
    while ((start = result.indexOf('<')) != -1) {
        int end = result.indexOf('>', start);
        if (end != -1) {
            result.remove(start, end - start + 1);
        } else {
            break;
        }
    }
    
    // Replace common HTML entities
    result.replace("&nbsp;", " ");
    result.replace("&amp;", "&");
    result.replace("&lt;", "<");
    result.replace("&gt;", ">");
    result.replace("&quot;", "\"");
    result.replace("&#39;", "'");
    
    // Clean up whitespace
    result.trim();
    
    // Remove excessive whitespace
    while (result.indexOf("  ") != -1) {
        result.replace("  ", " ");
    }
    
    return result;
}

int RssClient::findNextTag(const String& content, int startPos, String& tagName, String& tagContent) {
    int openPos = content.indexOf('<', startPos);
    if (openPos == -1) return -1;
    
    int closePos = content.indexOf('>', openPos);
    if (closePos == -1) return -1;
    
    String fullTag = content.substring(openPos + 1, closePos);
    
    // Check if it's a closing tag
    if (fullTag.startsWith("/")) {
        tagName = fullTag.substring(1);
        tagContent = "";
        return closePos + 1;
    }
    
    // Extract tag name (before any attributes)
    int spacePos = fullTag.indexOf(' ');
    if (spacePos != -1) {
        tagName = fullTag.substring(0, spacePos);
    } else {
        tagName = fullTag;
    }
    
    // Find the closing tag
    String closingTag = "</" + tagName + ">";
    int contentStart = closePos + 1;
    int contentEnd = content.indexOf(closingTag, contentStart);
    
    if (contentEnd != -1) {
        tagContent = content.substring(contentStart, contentEnd);
        
        // Check if content contains CDATA sections and extract them
        if (tagContent.indexOf("<![CDATA[") != -1) {
            String processedContent = "";
            int cdataPos = 0;
            int lastPos = 0;
            
            while ((cdataPos = tagContent.indexOf("<![CDATA[", lastPos)) != -1) {
                // Add any content before CDATA
                if (cdataPos > lastPos) {
                    processedContent += tagContent.substring(lastPos, cdataPos);
                }
                
                // Find end of CDATA
                int cdataEnd = tagContent.indexOf("]]>", cdataPos);
                if (cdataEnd != -1) {
                    // Extract CDATA content
                    String cdataContent = tagContent.substring(cdataPos + 9, cdataEnd);
                    processedContent += cdataContent;
                    lastPos = cdataEnd + 3;
                } else {
                    // Malformed CDATA, skip to end
                    lastPos = cdataPos + 9;
                }
            }
            
            // Add any remaining content after last CDATA
            if (lastPos < tagContent.length()) {
                processedContent += tagContent.substring(lastPos);
            }
            
            tagContent = processedContent;
        }
        
        return contentEnd + closingTag.length();
    } else {
        // Self-closing tag
        tagContent = "";
        return closePos + 1;
    }
}

String RssClient::decodeHtmlEntities(const String& html) {
    String result = html;
    
    // Common HTML entities
    result.replace("&amp;", "&");
    result.replace("&lt;", "<");
    result.replace("&gt;", ">");
    result.replace("&quot;", "\"");
    result.replace("&#39;", "'");
    result.replace("&nbsp;", " ");
    result.replace("&apos;", "'");
    result.replace("&hellip;", "...");
    result.replace("&mdash;", "—");
    result.replace("&ndash;", "–");
    result.replace("&lsquo;", "'");
    result.replace("&rsquo;", "'");
    result.replace("&ldquo;", "\"");
    result.replace("&rdquo;", "\"");
    
    return result;
} 