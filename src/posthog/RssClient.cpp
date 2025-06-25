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
    // Enhanced XML parser for RSS feeds with better CDATA handling
    
    Serial.println("RssClient: Starting XML parsing...");
    Serial.printf("RssClient: XML content length: %d bytes\n", xmlContent.length());
    
    if (xmlContent.length() < 100) {
        Serial.println("RssClient: XML content too short, likely invalid");
        return false;
    }
    
    // Find the channel tag
    int channelStart = xmlContent.indexOf("<channel>");
    if (channelStart == -1) {
        Serial.println("No <channel> tag found in RSS");
        return false;
    }
    
    Serial.println("RssClient: Found channel tag, looking for items...");
    
    // Find all <item> tags using a more robust approach
    int itemCount = 0;
    int searchPos = channelStart;
    
    while (searchPos < xmlContent.length()) {
        int itemStart = xmlContent.indexOf("<item>", searchPos);
        if (itemStart == -1) {
            break; // No more items
        }
        
        int itemEnd = xmlContent.indexOf("</item>", itemStart);
        if (itemEnd == -1) {
            Serial.println("RssClient: Malformed item tag, missing closing tag");
            break;
        }
        
        itemCount++;
        Serial.printf("RssClient: Processing item %d\n", itemCount);
        
        // Extract the complete item content
        String itemContent = xmlContent.substring(itemStart, itemEnd + 7);
        
        // Parse this item
        RssItem item;
        
        // Extract title (with CDATA handling)
        String title = extractTagContent(itemContent, "title");
        if (title.length() > 0) {
            item.title = decodeHtmlEntities(title);
            Serial.printf("RssClient: Found title: %s\n", item.title.c_str());
        }
        
        // Extract GUID
        String guid = extractTagContent(itemContent, "guid");
        if (guid.length() > 0) {
            item.guid = guid;
            Serial.printf("RssClient: Found GUID: %s\n", guid.c_str());
        }
        
        // Extract content:encoded (with CDATA handling)
        String rawContent = extractTagContent(itemContent, "content:encoded");
        if (rawContent.length() > 0) {
            item.content = extractTextFromHtml(decodeHtmlEntities(rawContent));
            Serial.printf("RssClient: Found content:encoded, raw length: %d, clean length: %d\n", 
                         rawContent.length(), item.content.length());
            
            // Show content preview
            if (item.content.length() > 0) {
                String preview = item.content.substring(0, min(150, (int)item.content.length()));
                Serial.printf("RssClient: Content preview: %s...\n", preview.c_str());
            }
        } else {
            // Fallback to description if no content:encoded
            String description = extractTagContent(itemContent, "description");
            if (description.length() > 0) {
                item.content = extractTextFromHtml(decodeHtmlEntities(description));
                Serial.printf("RssClient: Using description as content, length: %d\n", item.content.length());
            }
        }
        
        // Extract other fields
        item.description = decodeHtmlEntities(extractTagContent(itemContent, "description"));
        item.link = extractTagContent(itemContent, "link");
        item.pubDate = extractTagContent(itemContent, "pubDate");
        item.isNew = (guid != _lastSeenGuid);
        
        // Only add items with required fields
        if (item.guid.length() > 0 && item.title.length() > 0) {
            _items.push_back(item);
            Serial.printf("RssClient: Successfully added item: %s\n", item.title.c_str());
        } else {
            Serial.printf("RssClient: Skipping item - missing required fields (guid: %s, title: %s)\n", 
                         item.guid.c_str(), item.title.c_str());
        }
        
        // Move to next item
        searchPos = itemEnd + 7;
        
        // Safety check to prevent infinite loops
        if (itemCount > 50) {
            Serial.println("RssClient: Safety limit reached, stopping item parsing");
            break;
        }
    }
    
    // Sort items by publication date (newest first)
    std::sort(_items.begin(), _items.end(), 
        [](const RssItem& a, const RssItem& b) {
            return a.pubDate > b.pubDate;
        });
    
    Serial.printf("RssClient: Successfully parsed %d RSS items\n", _items.size());
    
    if (_items.size() == 0) {
        Serial.println("RssClient: WARNING - No items were successfully parsed!");
        return false;
    }
    
    return true;
}

String RssClient::extractTextFromHtml(const String& html) {
    String result = html;
    
    if (result.length() == 0) {
        return "";
    }
    
    // Handle paragraph tags first for better formatting
    result.replace("<p>", "\n\n");
    result.replace("</p>", "");
    result.replace("<br>", "\n");
    result.replace("<br/>", "\n");
    result.replace("<br />", "\n");
    result.replace("</div>", "\n");
    result.replace("<div>", "");
    
    // Handle headers
    result.replace("<h1>", "\n\n");
    result.replace("</h1>", "\n");
    result.replace("<h2>", "\n\n");
    result.replace("</h2>", "\n");
    result.replace("<h3>", "\n\n");
    result.replace("</h3>", "\n");
    
    // Handle lists
    result.replace("<li>", "\n• ");
    result.replace("</li>", "");
    result.replace("<ul>", "\n");
    result.replace("</ul>", "\n");
    result.replace("<ol>", "\n");
    result.replace("</ol>", "\n");
    
    // Remove all remaining HTML tags
    int start = 0;
    while ((start = result.indexOf('<')) != -1) {
        int end = result.indexOf('>', start);
        if (end != -1) {
            result.remove(start, end - start + 1);
        } else {
            // Malformed HTML, remove from start to end
            result.remove(start);
            break;
        }
    }
    
    // Replace HTML entities (do this after tag removal)
    result.replace("&nbsp;", " ");
    result.replace("&amp;", "&");
    result.replace("&lt;", "<");
    result.replace("&gt;", ">");
    result.replace("&quot;", "\"");
    result.replace("&#39;", "'");
    result.replace("&apos;", "'");
    result.replace("&hellip;", "...");
    result.replace("&mdash;", "-");
    result.replace("&ndash;", "-");
    result.replace("&lsquo;", "'");
    result.replace("&rsquo;", "'");
    result.replace("&ldquo;", "\"");
    result.replace("&rdquo;", "\"");
    
    // Clean up whitespace and formatting
    result.trim();
    
    // Replace multiple spaces with single space
    while (result.indexOf("  ") != -1) {
        result.replace("  ", " ");
    }
    
    // Replace multiple newlines with double newlines for paragraph separation
    while (result.indexOf("\n\n\n") != -1) {
        result.replace("\n\n\n", "\n\n");
    }
    
    // Remove leading/trailing newlines
    while (result.startsWith("\n")) {
        result = result.substring(1);
    }
    while (result.endsWith("\n")) {
        result = result.substring(0, result.length() - 1);
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

String RssClient::extractTagContent(const String& xmlContent, const String& tagName) {
    // Find opening tag
    String openTag = "<" + tagName;
    int tagStart = xmlContent.indexOf(openTag);
    if (tagStart == -1) {
        return ""; // Tag not found
    }
    
    // Find the end of the opening tag
    int contentStart = xmlContent.indexOf('>', tagStart);
    if (contentStart == -1) {
        return ""; // Malformed tag
    }
    contentStart++; // Move past >
    
    // Find closing tag
    String closeTag = "</" + tagName + ">";
    int contentEnd = xmlContent.indexOf(closeTag, contentStart);
    if (contentEnd == -1) {
        return ""; // No closing tag
    }
    
    // Extract content between tags
    String content = xmlContent.substring(contentStart, contentEnd);
    
    // Handle CDATA sections
    if (content.indexOf("<![CDATA[") != -1) {
        String processedContent = "";
        int pos = 0;
        
        while (pos < content.length()) {
            int cdataStart = content.indexOf("<![CDATA[", pos);
            if (cdataStart == -1) {
                // No more CDATA, add remaining content
                processedContent += content.substring(pos);
                break;
            }
            
            // Add content before CDATA
            if (cdataStart > pos) {
                processedContent += content.substring(pos, cdataStart);
            }
            
            // Find end of CDATA
            int cdataEnd = content.indexOf("]]>", cdataStart);
            if (cdataEnd == -1) {
                // Malformed CDATA, take rest as is
                processedContent += content.substring(cdataStart + 9);
                break;
            }
            
            // Extract CDATA content (skip <![CDATA[ and ]]>)
            String cdataContent = content.substring(cdataStart + 9, cdataEnd);
            processedContent += cdataContent;
            
            // Move past CDATA end
            pos = cdataEnd + 3;
        }
        
        return processedContent;
    }
    
    return content;
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