#include "NewsletterCard.h"
#include "Style.h"
#include <WiFi.h>

NewsletterCard::NewsletterCard(lv_obj_t* parent, ConfigManager& config, EventQueue& eventQueue,
                               RssClient& rssClient, uint16_t width, uint16_t height)
    : _config(config), _eventQueue(eventQueue), _rssClient(rssClient),
      _currentState(DisplayState::IDLE), _currentArticle(nullptr), _currentPage(0), _lastRefreshTime(0) {
    
    Serial.println("NewsletterCard: Constructor called");
    
    // Initialize with hardcoded PostHog Substack feed
    initializeFeed();
    
    // Create the main card container
    _card = lv_obj_create(parent);
    if (!_card) {
        Serial.println("NewsletterCard: Failed to create card container");
        return;
    }
    
    // Set card size and style with gradient background
    lv_obj_set_size(_card, width, height);
    lv_obj_set_style_bg_color(_card, lv_color_make(20, 25, 40), 0);
    lv_obj_set_style_border_width(_card, 2, 0);
    lv_obj_set_style_border_color(_card, lv_color_make(60, 80, 120), 0);
    lv_obj_set_style_border_opa(_card, LV_OPA_50, 0);
    lv_obj_set_style_radius(_card, 8, 0);
    lv_obj_set_style_pad_all(_card, 8, 0);
    
    // Create title label with enhanced styling
    _title_label = lv_label_create(_card);
    if (_title_label) {
        lv_obj_set_style_text_color(_title_label, lv_color_make(255, 200, 100), 0);
        lv_obj_set_style_text_font(_title_label, &font_loud_noises, 0);
        lv_label_set_text(_title_label, "📧 PostHog News");
        lv_obj_align(_title_label, LV_ALIGN_TOP_MID, 0, 5);
        lv_obj_set_style_text_align(_title_label, LV_TEXT_ALIGN_CENTER, 0);
    }
    
    // Create status label with enhanced styling
    _status_label = lv_label_create(_card);
    if (_status_label) {
        lv_obj_set_style_text_color(_status_label, lv_color_make(120, 200, 255), 0);
        lv_obj_set_style_text_font(_status_label, &font_label, 0);
        lv_label_set_text(_status_label, "🔄 Loading...");
        lv_obj_align(_status_label, LV_ALIGN_TOP_MID, 0, 30);
        lv_obj_set_style_text_align(_status_label, LV_TEXT_ALIGN_CENTER, 0);
    }
    
    // Create content label with enhanced styling
    _content_label = lv_label_create(_card);
    if (_content_label) {
        lv_obj_set_style_text_color(_content_label, lv_color_make(220, 220, 220), 0);
        lv_obj_set_style_text_font(_content_label, &font_label, 0);
        lv_obj_set_style_text_line_space(_content_label, 20, 0);
        lv_obj_set_style_text_align(_content_label, LV_TEXT_ALIGN_LEFT, 0);
        lv_label_set_long_mode(_content_label, LV_LABEL_LONG_WRAP);
        lv_obj_set_size(_content_label, width - 20, height - 70);
        lv_obj_align(_content_label, LV_ALIGN_TOP_LEFT, 5, 55);
        // Add subtle background for content
        lv_obj_set_style_bg_color(_content_label, lv_color_make(30, 35, 50), 0);
        lv_obj_set_style_bg_opa(_content_label, LV_OPA_30, 0);
        lv_obj_set_style_radius(_content_label, 4, 0);
        lv_obj_set_style_pad_all(_content_label, 5, 0);
    }
    
    Serial.println("NewsletterCard: Initial display update");
    updateDisplay();
    
    // Check if WiFi is connected and fetch feed immediately
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("NewsletterCard: WiFi connected, fetching feed immediately");
        refreshFeed();
    } else {
        Serial.println("NewsletterCard: WiFi not connected, will fetch when connected");
    }
}

NewsletterCard::~NewsletterCard() {
    // LVGL objects will be cleaned up automatically when parent is deleted
}

bool NewsletterCard::handleButtonPress(uint8_t button_index) {
    Serial.printf("NewsletterCard: handleButtonPress called with button_index: %d\n", button_index);
    
    // Only handle center button (button 1)
    if (button_index != 1) {
        Serial.printf("NewsletterCard: Ignoring button %d (not center button)\n", button_index);
        return false;
    }
    
    Serial.println("NewsletterCard: Center button detected, adding visual feedback...");
    
    // Add immediate visual feedback
    lv_label_set_text(_status_label, "Loading...");
    lv_obj_set_style_text_color(_status_label, lv_color_make(0, 100, 200), 0);
    
    // Small delay to make the loading feedback visible
    delay(100);
    
    switch (_currentState) {
        case DisplayState::IDLE:
            // In idle state, check if there's a newsletter available
            Serial.println("NewsletterCard: Center button pressed in IDLE state");
            if (_rssClient.getLatestItem()) {
                Serial.println("NewsletterCard: Found existing newsletter, opening...");
                // Open the latest newsletter directly
                _currentArticle = _rssClient.getLatestItem();
                String cleanedContent = stripHtmlAndDecodeEntities(_currentArticle->content);
                paginateContent(cleanedContent);
                _currentPage = 0;
                showReadingState();
            } else {
                Serial.println("NewsletterCard: No newsletter available, refreshing feed...");
                // No newsletter available, refresh the feed
                refreshFeed();
            }
            return true;
            
        case DisplayState::NEW_NOTIFICATION:
            Serial.println("NewsletterCard: Center button pressed in NEW_NOTIFICATION state");
            // In notification state, center button opens the article
            if (_rssClient.getLatestItem()) {
                _currentArticle = _rssClient.getLatestItem();
                String cleanedContent = stripHtmlAndDecodeEntities(_currentArticle->content);
                paginateContent(cleanedContent);
                _currentPage = 0;
                showReadingState();
            }
            return true;
            
        case DisplayState::READING:
            Serial.println("NewsletterCard: Center button pressed in READING state");
            Serial.printf("NewsletterCard: Current page: %d, Total pages: %d\n", _currentPage + 1, _articlePages.size());
            
            // In reading state, center button advances to next page
            if (_currentPage < _articlePages.size() - 1) {
                _currentPage++;
                Serial.printf("NewsletterCard: Advancing to page %d\n", _currentPage + 1);
                updateReadingDisplay();
            } else {
                // End of article, return to idle
                Serial.println("NewsletterCard: Reached end of article, returning to idle");
                _currentState = DisplayState::IDLE;
                _currentArticle = nullptr;
                _currentPage = 0;
                _articlePages.clear();
                updateDisplay();
            }
            return true;
    }
    
    return false;
}

void NewsletterCard::updateDisplay() {
    // Check if it's time to refresh
    if (shouldRefresh()) {
        refreshFeed();
    }
    
    // Update display based on current state
    switch (_currentState) {
        case DisplayState::IDLE:
            showIdleState();
            break;
        case DisplayState::NEW_NOTIFICATION:
            showNewNotificationState();
            break;
        case DisplayState::READING:
            updateReadingDisplay();
            break;
    }
}


void NewsletterCard::refreshFeed() {
    Serial.println("NewsletterCard: Starting feed refresh...");
    
    // Show loading state immediately
    lv_label_set_text(_status_label, "🔄 Fetching newsletter...");
    lv_obj_set_style_text_color(_status_label, lv_color_make(120, 200, 255), 0);
    
    if (!_rssClient.isReady()) {
        Serial.println("NewsletterCard: RSS client not ready - WiFi not connected");
        lv_label_set_text(_status_label, "🚫 No WiFi connection");
        lv_obj_set_style_text_color(_status_label, lv_color_make(255, 100, 100), 0);
        
        // Try to reconnect in 3 seconds
        _lastRefreshTime = millis() - REFRESH_INTERVAL + 3000;
        return;
    }
    
    Serial.println("NewsletterCard: RSS client ready, fetching PostHog Substack feed...");
    
    // Clear any old items to force fresh fetch
    _rssClient.clearItems();
    
    if (_rssClient.fetchFeed()) {
        Serial.println("NewsletterCard: Feed fetch successful");
        _lastRefreshTime = millis(); // Update refresh time on success
        
        const RssItem* latestItem = _rssClient.getLatestItem();
        if (latestItem) {
            Serial.printf("NewsletterCard: Found latest item: %s\n", latestItem->title.c_str());
            Serial.printf("NewsletterCard: Item GUID: %s\n", latestItem->guid.c_str());
            Serial.printf("NewsletterCard: Content length: %d\n", latestItem->content.length());
            
            _currentArticle = latestItem;
            
            // Always show as available (since we always want the latest)
            _currentState = DisplayState::IDLE;
            Serial.println("NewsletterCard: Newsletter available, showing idle state");
        } else {
            Serial.println("NewsletterCard: No items found in feed");
            _currentState = DisplayState::IDLE;
        }
    } else {
        Serial.println("NewsletterCard: Feed fetch failed");
        lv_label_set_text(_status_label, "⚠️ Feed fetch failed");
        lv_obj_set_style_text_color(_status_label, lv_color_make(255, 150, 0), 0);
        _currentState = DisplayState::IDLE;
        
        // Retry in 30 seconds
        _lastRefreshTime = millis() - REFRESH_INTERVAL + 30000;
    }
    
    updateDisplay();
}

void NewsletterCard::onEvent(const Event& event) {
    // Handle any relevant events here
    // For now, we'll rely on manual refresh calls
}

void NewsletterCard::showIdleState() {
    Serial.println("NewsletterCard: Showing idle state...");
    const RssItem* latest = _rssClient.getLatestItem();
    if (latest) {
        Serial.printf("NewsletterCard: Displaying latest newsletter: %s\n", latest->title.c_str());
        // Show the latest newsletter with attractive styling
        lv_label_set_text(_title_label, "📧 Latest from PostHog");
        lv_label_set_text(_content_label, latest->title.c_str());
        lv_label_set_text(_status_label, "👆 Press CENTER to read");
        
        // Update colors for available content
        lv_obj_set_style_text_color(_title_label, lv_color_make(255, 200, 100), 0);
        lv_obj_set_style_text_color(_content_label, lv_color_make(255, 255, 255), 0);
        lv_obj_set_style_text_color(_status_label, lv_color_make(120, 200, 255), 0);
    } else {
        Serial.println("NewsletterCard: No newsletter available to display");
        // No newsletter available - encourage refresh
        lv_label_set_text(_title_label, "📧 PostHog News");
        lv_label_set_text(_content_label, "🔄 Fetching latest newsletter...\n\nMake sure WiFi is connected!");
        lv_label_set_text(_status_label, "👆 Press CENTER to refresh");
        
        // Update colors for no content state
        lv_obj_set_style_text_color(_title_label, lv_color_make(255, 200, 100), 0);
        lv_obj_set_style_text_color(_content_label, lv_color_make(180, 180, 180), 0);
        lv_obj_set_style_text_color(_status_label, lv_color_make(120, 200, 255), 0);
    }
}

void NewsletterCard::showNewNotificationState() {
    const RssItem* latest = _rssClient.getLatestItem();
    if (latest) {
        lv_label_set_text(_title_label, "🆕 NEW from PostHog!");
        lv_label_set_text(_content_label, latest->title.c_str());
        lv_label_set_text(_status_label, "✨ Press CENTER to read ✨");
        
        // Update colors to indicate new content with excitement
        lv_obj_set_style_text_color(_title_label, lv_color_make(255, 100, 100), 0);
        lv_obj_set_style_text_color(_content_label, lv_color_make(255, 255, 255), 0);
        lv_obj_set_style_text_color(_status_label, lv_color_make(100, 255, 100), 0);
    }
}

void NewsletterCard::showReadingState() {
    if (_currentArticle) {
        // Truncate long titles for better display
        String displayTitle = _currentArticle->title;
        if (displayTitle.length() > 25) {
            displayTitle = displayTitle.substring(0, 22) + "...";
        }
        lv_label_set_text(_title_label, displayTitle.c_str());
        
        // Set reading mode colors
        lv_obj_set_style_text_color(_title_label, lv_color_make(255, 255, 100), 0);
        
        // Force state change to READING
        _currentState = DisplayState::READING;
        
        updateReadingDisplay();
    }
}

void NewsletterCard::paginateContent(const String& content) {
    Serial.println("NewsletterCard: Starting content pagination...");
    _articlePages.clear();
    
    if (content.length() == 0) {
        _articlePages.push_back("No content available");
        Serial.println("NewsletterCard: No content to paginate");
        return;
    }
    
    Serial.printf("NewsletterCard: Paginating content of length: %d\n", content.length());
    
    // Split content into paragraphs first, then handle line breaking
    std::vector<String> paragraphs;
    int start = 0;
    int pos = 0;
    
    // Split by double newlines (paragraph breaks)
    while (pos < content.length()) {
        int nextPara = content.indexOf("\n\n", pos);
        if (nextPara == -1) {
            nextPara = content.length();
        }
        
        String paragraph = content.substring(pos, nextPara);
        paragraph.trim();
        if (paragraph.length() > 0) {
            paragraphs.push_back(paragraph);
        }
        pos = nextPara + 2;
    }
    
    Serial.printf("NewsletterCard: Found %d paragraphs\n", paragraphs.size());
    
    // Build pages with better formatting
    String currentPage = "";
    int currentLineCount = 0;
    
    for (const String& paragraph : paragraphs) {
        // Process each paragraph word by word
        std::vector<String> words;
        int wordStart = 0;
        int wordEnd = paragraph.indexOf(' ');
        
        while (wordEnd != -1) {
            words.push_back(paragraph.substring(wordStart, wordEnd));
            wordStart = wordEnd + 1;
            wordEnd = paragraph.indexOf(' ', wordStart);
        }
        if (wordStart < paragraph.length()) {
            words.push_back(paragraph.substring(wordStart));
        }
        
        // Build lines for this paragraph
        String currentLine = "";
        for (const String& word : words) {
            // Check if adding this word would exceed line length
            if (currentLine.length() + word.length() + 1 > MAX_CHARS_PER_LINE) {
                // Add current line to page
                if (currentPage.length() > 0) {
                    currentPage += "\n";
                }
                currentPage += currentLine;
                currentLineCount++;
                
                // Check if we need a new page
                if (currentLineCount >= MAX_LINES_PER_PAGE) {
                    _articlePages.push_back(currentPage);
                    currentPage = "";
                    currentLineCount = 0;
                }
                
                currentLine = word; // Start new line with this word
            } else {
                // Add word to current line
                if (currentLine.length() > 0) {
                    currentLine += " ";
                }
                currentLine += word;
            }
        }
        
        // Add the last line of the paragraph
        if (currentLine.length() > 0) {
            if (currentPage.length() > 0) {
                currentPage += "\n";
            }
            currentPage += currentLine;
            currentLineCount++;
        }
        
        // Add paragraph break (but don't exceed page limit)
        if (currentLineCount < MAX_LINES_PER_PAGE - 1) {
            currentPage += "\n";
            currentLineCount++;
        }
        
        // Check if we need a new page after this paragraph
        if (currentLineCount >= MAX_LINES_PER_PAGE) {
            _articlePages.push_back(currentPage);
            currentPage = "";
            currentLineCount = 0;
        }
    }
    
    // Add the last page if it has content
    if (currentPage.length() > 0) {
        _articlePages.push_back(currentPage);
    }
    
    // If no pages were created, add a default message
    if (_articlePages.empty()) {
        _articlePages.push_back("Content could not be parsed");
    }
    
    Serial.printf("NewsletterCard: Created %d pages\n", _articlePages.size());
    
    // Debug: show first page preview
    if (_articlePages.size() > 0) {
        String preview = _articlePages[0].substring(0, min(100, (int)_articlePages[0].length()));
        Serial.printf("NewsletterCard: First page preview: %s...\n", preview.c_str());
    }
}

void NewsletterCard::updateReadingDisplay() {
    Serial.printf("NewsletterCard: updateReadingDisplay - page %d of %d\n", _currentPage + 1, _articlePages.size());
    
    if (_currentPage < _articlePages.size()) {
        // Set the content for current page
        lv_label_set_text(_content_label, _articlePages[_currentPage].c_str());
        
        // Create attractive page indicator with clear navigation
        String status = "📖 Page " + String(_currentPage + 1) + "/" + String(_articlePages.size());
        if (_currentPage < _articlePages.size() - 1) {
            status += "\n👆 CENTER = Next page";
        } else {
            status += "\n👆 CENTER = Exit reading";
        }
        lv_label_set_text(_status_label, status.c_str());
        
        // Update colors for reading mode
        lv_obj_set_style_text_color(_content_label, lv_color_make(240, 240, 240), 0);
        lv_obj_set_style_text_color(_status_label, lv_color_make(100, 200, 255), 0);
        
        // Debug output
        Serial.printf("NewsletterCard: Displaying page content length: %d\n", _articlePages[_currentPage].length());
        if (_articlePages[_currentPage].length() > 0) {
            String preview = _articlePages[_currentPage].substring(0, min(50, (int)_articlePages[_currentPage].length()));
            Serial.printf("NewsletterCard: Page preview: %s...\n", preview.c_str());
        }
    } else {
        Serial.println("NewsletterCard: ERROR - Current page index out of bounds!");
        lv_label_set_text(_content_label, "Page error");
        lv_label_set_text(_status_label, "Error: Invalid page");
    }
}

bool NewsletterCard::shouldRefresh() const {
    return (millis() - _lastRefreshTime) >= REFRESH_INTERVAL;
}

void NewsletterCard::initializeFeed() {
    // Always use PostHog Substack feed - no configuration needed
    Serial.println("NewsletterCard: Initializing with PostHog Substack feed");
    _rssClient.setFeedUrl("https://posthog.substack.com/feed");
    Serial.printf("NewsletterCard: Feed URL set to: %s\n", _rssClient.getFeedUrl().c_str());
}

void NewsletterCard::handlePeriodicUpdate() {
    // Check if it's time to refresh and update display
    if (shouldRefresh()) {
        refreshFeed();
    }
    
    // Update display based on current state
    updateDisplay();
}

String NewsletterCard::stripHtmlAndDecodeEntities(const String& htmlContent) {
    Serial.println("NewsletterCard: Starting HTML stripping...");
    Serial.printf("NewsletterCard: Original content length: %d\n", htmlContent.length());
    
    String cleanedContent = htmlContent;
    
    // Remove image containers and complex media elements first
    removeImageTags(cleanedContent);
    
    // First, decode common HTML entities
    cleanedContent.replace("&amp;", "&");
    cleanedContent.replace("&lt;", "<");
    cleanedContent.replace("&gt;", ">");
    cleanedContent.replace("&quot;", "\"");
    cleanedContent.replace("&#39;", "'");
    cleanedContent.replace("&apos;", "'");
    cleanedContent.replace("&nbsp;", " ");
    cleanedContent.replace("&mdash;", "-");
    cleanedContent.replace("&ndash;", "-");
    cleanedContent.replace("&lsquo;", "'");
    cleanedContent.replace("&rsquo;", "'");
    cleanedContent.replace("&ldquo;", "\"");
    cleanedContent.replace("&rdquo;", "\"");
    cleanedContent.replace("&hellip;", "...");
    cleanedContent.replace("&copy;", "(c)");
    cleanedContent.replace("&reg;", "(R)");
    cleanedContent.replace("&trade;", "(TM)");
    
    Serial.println("NewsletterCard: HTML entities decoded");
    
    // Handle structural tags with better formatting
    cleanedContent.replace("<h1>", "\n\n== ");
    cleanedContent.replace("</h1>", " ==\n");
    cleanedContent.replace("<h2>", "\n\n-- ");
    cleanedContent.replace("</h2>", " --\n");
    cleanedContent.replace("<h3>", "\n\n* ");
    cleanedContent.replace("</h3>", " *\n");
    cleanedContent.replace("<h4>", "\n\n+ ");
    cleanedContent.replace("</h4>", " +\n");
    cleanedContent.replace("<h5>", "\n\n> ");
    cleanedContent.replace("</h5>", " <\n");
    cleanedContent.replace("<h6>", "\n\n~ ");
    cleanedContent.replace("</h6>", " ~\n");
    
    // Handle paragraph and line breaks
    cleanedContent.replace("<p>", "\n\n");
    cleanedContent.replace("</p>", "");
    cleanedContent.replace("<br>", "\n");
    cleanedContent.replace("<br/>", "\n");
    cleanedContent.replace("<br />", "\n");
    
    // Handle lists with better formatting
    cleanedContent.replace("<ul>", "\n");
    cleanedContent.replace("</ul>", "\n");
    cleanedContent.replace("<ol>", "\n");
    cleanedContent.replace("</ol>", "\n");
    cleanedContent.replace("<li>", "\n• ");
    cleanedContent.replace("</li>", "");
    
    // Handle divs and spans
    cleanedContent.replace("<div>", "\n");
    cleanedContent.replace("</div>", "\n");
    cleanedContent.replace("<span>", "");
    cleanedContent.replace("</span>", "");
    
    // Handle emphasis tags
    cleanedContent.replace("<strong>", "*");
    cleanedContent.replace("</strong>", "*");
    cleanedContent.replace("<b>", "*");
    cleanedContent.replace("</b>", "*");
    cleanedContent.replace("<em>", "_");
    cleanedContent.replace("</em>", "_");
    cleanedContent.replace("<i>", "_");
    cleanedContent.replace("</i>", "_");
    
    // Remove all remaining HTML tags
    int tagStart = cleanedContent.indexOf('<');
    while (tagStart != -1) {
        int tagEnd = cleanedContent.indexOf('>', tagStart);
        if (tagEnd != -1) {
            // Remove the tag
            cleanedContent.remove(tagStart, tagEnd - tagStart + 1);
            // Look for next tag
            tagStart = cleanedContent.indexOf('<', tagStart);
        } else {
            // Malformed HTML, remove from tag start to end
            cleanedContent.remove(tagStart);
            break;
        }
    }
    
    Serial.println("NewsletterCard: HTML tags removed");
    
    // Clean up whitespace and formatting
    // Replace multiple spaces with single space
    while (cleanedContent.indexOf("  ") != -1) {
        cleanedContent.replace("  ", " ");
    }
    
    // Clean up excessive newlines but preserve paragraph structure
    while (cleanedContent.indexOf("\n\n\n\n") != -1) {
        cleanedContent.replace("\n\n\n\n", "\n\n");
    }
    while (cleanedContent.indexOf("\n\n\n") != -1) {
        cleanedContent.replace("\n\n\n", "\n\n");
    }
    
    // Trim leading/trailing whitespace
    cleanedContent.trim();
    
    Serial.printf("NewsletterCard: Cleaned content length: %d\n", cleanedContent.length());
    Serial.println("NewsletterCard: HTML stripping complete");
    
    // Debug: Show first 300 characters of cleaned content
    if (cleanedContent.length() > 0) {
        String preview = cleanedContent.substring(0, min(300, (int)cleanedContent.length()));
        Serial.printf("NewsletterCard: Content preview: %s\n", preview.c_str());
    } else {
        Serial.println("NewsletterCard: Warning - cleaned content is empty!");
    }
    
    return cleanedContent;
}

void NewsletterCard::removeImageTags(String& content) {
    Serial.println("NewsletterCard: Removing image tags and media elements...");
    
    // Remove complex image containers and figures
    removeNestedTag(content, "<figure", "</figure>");
    removeNestedTag(content, "<div class=\"captioned-image-container\"", "</div>");
    removeNestedTag(content, "<div class=\"image-container\"", "</div>");
    removeNestedTag(content, "<picture", "</picture>");
    removeNestedTag(content, "<source", ">");
    
    // Remove all img tags (including self-closing ones)
    int imgStart = 0;
    while ((imgStart = content.indexOf("<img", imgStart)) != -1) {
        int imgEnd = content.indexOf(">", imgStart);
        if (imgEnd != -1) {
            // Replace image with placeholder text
            content = content.substring(0, imgStart) + "[IMAGE]" + content.substring(imgEnd + 1);
            imgStart += 7; // Length of "[IMAGE]"
        } else {
            break;
        }
    }
    
    // Remove SVG elements
    removeNestedTag(content, "<svg", "</svg>");
    
    // Remove script and style tags
    removeNestedTag(content, "<script", "</script>");
    removeNestedTag(content, "<style", "</style>");
    
    // Remove other media-related tags
    removeNestedTag(content, "<video", "</video>");
    removeNestedTag(content, "<audio", "</audio>");
    removeNestedTag(content, "<iframe", "</iframe>");
    removeNestedTag(content, "<canvas", "</canvas>");
    
    Serial.println("NewsletterCard: Image and media removal complete");
}

void NewsletterCard::removeNestedTag(String& content, const String& openTag, const String& closeTag) {
    int tagStart = 0;
    while ((tagStart = content.indexOf(openTag, tagStart)) != -1) {
        // Find the end of the opening tag
        int openEnd = content.indexOf(">", tagStart);
        if (openEnd == -1) break;
        
        // Check if it's a self-closing tag
        if (content.charAt(openEnd - 1) == '/') {
            // Self-closing tag
            content.remove(tagStart, openEnd - tagStart + 1);
            continue;
        }
        
        // Find the matching closing tag
        int closeStart = content.indexOf(closeTag, openEnd);
        if (closeStart != -1) {
            int closeEnd = content.indexOf(">", closeStart);
            if (closeEnd != -1) {
                // Remove the entire tag and its content
                content.remove(tagStart, closeEnd - tagStart + 1);
            } else {
                // Malformed closing tag
                content.remove(tagStart, closeStart - tagStart + closeTag.length());
            }
        } else {
            // No closing tag found, remove just the opening tag
            content.remove(tagStart, openEnd - tagStart + 1);
        }
    }
} 