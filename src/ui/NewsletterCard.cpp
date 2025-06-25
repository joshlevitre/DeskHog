#include "NewsletterCard.h"
#include "Style.h"
#include <WiFi.h>

NewsletterCard::NewsletterCard(lv_obj_t* parent, ConfigManager& config, EventQueue& eventQueue,
                               RssClient& rssClient, uint16_t width, uint16_t height)
    : _config(config), _eventQueue(eventQueue), _rssClient(rssClient),
      _currentState(DisplayState::IDLE), _currentArticle(nullptr), _lastRefreshTime(0),
      _currentCharOffset(0), _maxVisibleLines(9) {
    
    Serial.println("NewsletterCard: Constructor called");
    
    // Initialize with hardcoded PostHog Substack feed
    initializeFeed();
    
    // Create the main card container with improved styling
    _card = lv_obj_create(parent);
    if (!_card) {
        Serial.println("NewsletterCard: Failed to create card container");
        return;
    }
    
    // Set card size and enhanced styling
    lv_obj_set_size(_card, width, height);
    lv_obj_set_style_bg_color(_card, Style::backgroundColor(), 0);
    lv_obj_set_style_border_width(_card, 0, 0);
    lv_obj_set_style_radius(_card, 0, 0);
    lv_obj_set_style_pad_all(_card, 0, 0);
    
    // Create idle mode container (default visible)
    _idle_container = lv_obj_create(_card);
    if (_idle_container) {
        lv_obj_set_size(_idle_container, width, height);
        lv_obj_set_style_bg_color(_idle_container, lv_color_make(15, 20, 35), 0);
        lv_obj_set_style_border_width(_idle_container, 2, 0);
        lv_obj_set_style_border_color(_idle_container, lv_color_make(60, 80, 120), 0);
        lv_obj_set_style_border_opa(_idle_container, LV_OPA_60, 0);
        lv_obj_set_style_radius(_idle_container, 8, 0);
        lv_obj_set_style_pad_all(_idle_container, IDLE_PADDING, 0);
        
        // Create title label with enhanced styling
        _title_label = lv_label_create(_idle_container);
        if (_title_label) {
            lv_obj_set_style_text_color(_title_label, lv_color_make(255, 200, 100), 0);
            lv_obj_set_style_text_font(_title_label, Style::loudNoisesFont(), 0);
            lv_label_set_text(_title_label, "PostHog News");
            lv_obj_align(_title_label, LV_ALIGN_TOP_MID, 0, 5);
            lv_obj_set_style_text_align(_title_label, LV_TEXT_ALIGN_CENTER, 0);
        }
        
        // Create status label with enhanced styling
        _status_label = lv_label_create(_idle_container);
        if (_status_label) {
            lv_obj_set_style_text_color(_status_label, lv_color_make(120, 200, 255), 0);
            lv_obj_set_style_text_font(_status_label, Style::labelFont(), 0);
            lv_label_set_text(_status_label, "Loading...");
            lv_obj_align(_status_label, LV_ALIGN_TOP_MID, 0, 30);
            lv_obj_set_style_text_align(_status_label, LV_TEXT_ALIGN_CENTER, 0);
        }
        
        // Create content label with enhanced styling
        _content_label = lv_label_create(_idle_container);
        if (_content_label) {
            lv_obj_set_style_text_color(_content_label, lv_color_make(220, 220, 220), 0);
            lv_obj_set_style_text_font(_content_label, Style::valueFont(), 0);
            lv_obj_set_style_text_line_space(_content_label, 4, 0);
            lv_obj_set_style_text_align(_content_label, LV_TEXT_ALIGN_LEFT, 0);
            lv_label_set_long_mode(_content_label, LV_LABEL_LONG_WRAP);
            lv_obj_set_size(_content_label, width - (IDLE_PADDING * 2), height - 40);
            lv_obj_align(_content_label, LV_ALIGN_TOP_LEFT, 0, 48);
            
            // Minimal padding for maximum text display
            lv_obj_set_style_pad_all(_content_label, 2, 0);
            
            // Initialize with empty text to prevent "Text" label
            lv_label_set_text(_content_label, "");
        }
    }
    
    // Create reading mode container (initially hidden)
    _reading_container = lv_obj_create(_card);
    if (_reading_container) {
        lv_obj_set_size(_reading_container, width, height);
        lv_obj_set_style_bg_color(_reading_container, lv_color_make(10, 15, 25), 0);
        lv_obj_set_style_border_width(_reading_container, 0, 0);
        lv_obj_set_style_radius(_reading_container, 0, 0);
        lv_obj_set_style_pad_all(_reading_container, READING_PADDING, 0);
        
        // Initially hide reading container
        lv_obj_add_flag(_reading_container, LV_OBJ_FLAG_HIDDEN);
        
        // Create reading mode title (smaller, more subtle)
        lv_obj_t* reading_title = lv_label_create(_reading_container);
        if (reading_title) {
            lv_obj_set_style_text_color(reading_title, lv_color_make(180, 180, 200), 0);
            lv_obj_set_style_text_font(reading_title, Style::labelFont(), 0);
            lv_obj_align(reading_title, LV_ALIGN_TOP_LEFT, 0, 0);
            lv_obj_set_style_text_align(reading_title, LV_TEXT_ALIGN_LEFT, 0);
            // Will be updated with article title
        }
        
        // Create reading mode content with better typography
        lv_obj_t* reading_content = lv_label_create(_reading_container);
        if (reading_content) {
            lv_obj_set_style_text_color(reading_content, lv_color_make(240, 240, 240), 0);
            lv_obj_set_style_text_font(reading_content, Style::valueFont(), 0);
            lv_obj_set_style_text_line_space(reading_content, 6, 0);
            lv_obj_set_style_text_align(reading_content, LV_TEXT_ALIGN_LEFT, 0);
            lv_label_set_long_mode(reading_content, LV_LABEL_LONG_WRAP);
            
            // Use full available space to eliminate black bar
            lv_obj_set_size(reading_content, width - (READING_PADDING * 2), height - (READING_PADDING * 2) - 20);
            lv_obj_align(reading_content, LV_ALIGN_TOP_LEFT, 0, 18);
            
            // Set minimal padding to maximize text area
            lv_obj_set_style_pad_all(reading_content, 1, 0);
            lv_obj_set_style_pad_left(reading_content, 2, 0);
            lv_obj_set_style_pad_right(reading_content, 2, 0);
            
            // Initialize with empty text to prevent "Text" label
            lv_label_set_text(reading_content, "");
            
            // Store reference to reading content label
            _reading_content_label = reading_content;
        }
        
        // Progress indicator removed for cleaner UI
        
        // Navigation hint removed for cleaner UI
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
    
    Serial.println("NewsletterCard: Center button detected");
    
    switch (_currentState) {
        case DisplayState::IDLE:
            // In idle state, check if there's a newsletter available
            Serial.println("NewsletterCard: Center button pressed in IDLE state");
            if (_rssClient.getLatestItem()) {
                Serial.println("NewsletterCard: Found existing newsletter, opening...");
                // Open the latest newsletter directly
                _currentArticle = _rssClient.getLatestItem();
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
                showReadingState();
            }
            return true;
            
        case DisplayState::READING:
            Serial.println("NewsletterCard: Center button pressed in READING state");
            // In reading state, center button scrolls 45 characters forward to next word boundary
            if (_currentCharOffset + 45 < _fullArticleText.length()) {
                int newOffset = _currentCharOffset + 45;
                
                // Find the next word boundary (space, newline, or end of text)
                while (newOffset < _fullArticleText.length() && 
                       _fullArticleText.charAt(newOffset) != ' ' && 
                       _fullArticleText.charAt(newOffset) != '\n' &&
                       _fullArticleText.charAt(newOffset) != '\t') {
                    newOffset++;
                }
                
                // Skip any leading whitespace to start at the beginning of the next word
                while (newOffset < _fullArticleText.length() && 
                       (_fullArticleText.charAt(newOffset) == ' ' || 
                        _fullArticleText.charAt(newOffset) == '\n' ||
                        _fullArticleText.charAt(newOffset) == '\t')) {
                    newOffset++;
                }
                
                _currentCharOffset = newOffset;
                updateReadingDisplay();
            } else {
                // Reached end of content, return to idle state
                Serial.println("NewsletterCard: Reached end of content, returning to idle");
                _currentCharOffset = 0;
                showIdleState();
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
    lv_label_set_text(_status_label, "Fetching...");
    lv_obj_set_style_text_color(_status_label, lv_color_make(120, 200, 255), 0);
    
    if (!_rssClient.isReady()) {
        Serial.println("NewsletterCard: RSS client not ready - WiFi not connected");
        lv_label_set_text(_status_label, "No WiFi");
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
        lv_label_set_text(_status_label, "Fetch failed");
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
    
    // Switch to idle mode container
    lv_obj_clear_flag(_idle_container, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(_reading_container, LV_OBJ_FLAG_HIDDEN);
    
    const RssItem* latest = _rssClient.getLatestItem();
    if (latest) {
        Serial.printf("NewsletterCard: Displaying latest newsletter: %s\n", latest->title.c_str());
        // Show the latest newsletter with attractive styling
        lv_label_set_text(_title_label, "Latest from PostHog");
        lv_label_set_text(_content_label, latest->title.c_str());
        lv_label_set_text(_status_label, "Press CENTER to read");
        
        // Update colors for available content
        lv_obj_set_style_text_color(_title_label, lv_color_make(255, 200, 100), 0);
        lv_obj_set_style_text_color(_content_label, lv_color_make(255, 255, 255), 0);
        lv_obj_set_style_text_color(_status_label, lv_color_make(120, 200, 255), 0);
    } else {
        Serial.println("NewsletterCard: No newsletter available to display");
        // No newsletter available - encourage refresh
        lv_label_set_text(_title_label, "PostHog News");
        lv_label_set_text(_content_label, "Fetching latest newsletter...\n\nCheck WiFi connection");
        lv_label_set_text(_status_label, "Press CENTER to refresh");
        
        // Update colors for no content state
        lv_obj_set_style_text_color(_title_label, lv_color_make(255, 200, 100), 0);
        lv_obj_set_style_text_color(_content_label, lv_color_make(180, 180, 180), 0);
        lv_obj_set_style_text_color(_status_label, lv_color_make(120, 200, 255), 0);
    }
}

void NewsletterCard::showNewNotificationState() {
    // Switch to idle mode container (notification is shown in idle mode)
    lv_obj_clear_flag(_idle_container, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(_reading_container, LV_OBJ_FLAG_HIDDEN);
    
    const RssItem* latest = _rssClient.getLatestItem();
    if (latest) {
        lv_label_set_text(_title_label, "NEW from PostHog!");
        lv_label_set_text(_content_label, latest->title.c_str());
        lv_label_set_text(_status_label, "Press CENTER to read");
        
        // Update colors to indicate new content with excitement
        lv_obj_set_style_text_color(_title_label, lv_color_make(255, 100, 100), 0);
        lv_obj_set_style_text_color(_content_label, lv_color_make(255, 255, 255), 0);
        lv_obj_set_style_text_color(_status_label, lv_color_make(100, 255, 100), 0);
    }
}

void NewsletterCard::showReadingState() {
    if (_currentArticle) {
        // Switch to reading mode container
        lv_obj_add_flag(_idle_container, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(_reading_container, LV_OBJ_FLAG_HIDDEN);
        
        // Update reading mode title
        lv_obj_t* reading_title = lv_obj_get_child(_reading_container, 0);
        if (reading_title) {
            String displayTitle = _currentArticle->title;
            if (displayTitle.length() > 30) {
                displayTitle = displayTitle.substring(0, 27) + "...";
            }
            lv_label_set_text(reading_title, displayTitle.c_str());
        }
        
        // Initialize line-by-line scrolling
        String cleanedContent = stripHtmlAndDecodeEntities(_currentArticle->content);
        _fullArticleText = cleanedContent;
        _currentCharOffset = 0;
        _maxVisibleLines = MAX_LINES_PER_PAGE;
        
        // Force state change to READING
        _currentState = DisplayState::READING;
        
        updateReadingDisplay();
    }
}

void NewsletterCard::updateReadingDisplay() {
    Serial.printf("NewsletterCard: updateReadingDisplay - char offset %d\n", _currentCharOffset);
    
    if (_fullArticleText.length() > 0) {
        // Get the text starting from the current character offset
        String displayText = "";
        int startPos = _currentCharOffset;
        
        if (startPos < _fullArticleText.length()) {
            // Get the text from current offset to end, or a reasonable chunk
            int endPos = min(startPos + 500, (int)_fullArticleText.length()); // Show up to 500 chars
            displayText = _fullArticleText.substring(startPos, endPos);
        }
        
        // Set the content for current view
        lv_label_set_text(_reading_content_label, displayText.c_str());
        
        Serial.printf("NewsletterCard: Displaying text from char %d, length: %d\n", startPos, displayText.length());
    } else {
        Serial.println("NewsletterCard: ERROR - No article text available!");
        lv_label_set_text(_reading_content_label, "No content available");
    }
}

bool NewsletterCard::shouldRefresh() const {
    return (millis() - _lastRefreshTime) >= REFRESH_INTERVAL;
}

void NewsletterCard::initializeFeed() {
    // Get RSS URL from config, use PostHog Substack as default
    String rssUrl = _config.getConfigValue("newsletter_rss_url");
    if (rssUrl.length() == 0) {
        rssUrl = "https://posthog.substack.com/feed";
    }
    Serial.printf("NewsletterCard: Initializing with RSS feed: %s\n", rssUrl.c_str());
    _rssClient.setFeedUrl(rssUrl);
    Serial.printf("NewsletterCard: Feed URL set to: %s\n", _rssClient.getFeedUrl().c_str());
}

String NewsletterCard::getFeedUrl() const {
    String rssUrl = _config.getConfigValue("newsletter_rss_url");
    if (rssUrl.length() == 0) {
        rssUrl = "https://posthog.substack.com/feed";
    }
    return rssUrl;
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
    
    // Remove any remaining script and style content
    removeNestedTag(cleanedContent, "<script", "</script>");
    removeNestedTag(cleanedContent, "<style", "</style>");
    removeNestedTag(cleanedContent, "<noscript", "</noscript>");
    
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
    removeNestedTag(content, "<div class=\"image\"", "</div>");
    removeNestedTag(content, "<div class=\"img\"", "</div>");
    removeNestedTag(content, "<picture", "</picture>");
    removeNestedTag(content, "<source", ">");
    removeNestedTag(content, "<figcaption", "</figcaption>");
    
    // Remove all img tags (including self-closing ones) - more aggressive approach
    int imgStart = 0;
    while ((imgStart = content.indexOf("<img", imgStart)) != -1) {
        int imgEnd = content.indexOf(">", imgStart);
        if (imgEnd != -1) {
            // Remove image completely instead of replacing with placeholder
            content = content.substring(0, imgStart) + content.substring(imgEnd + 1);
        } else {
            // If no closing tag found, remove from img start to end
            content = content.substring(0, imgStart);
            break;
        }
    }
    
    // Remove any remaining img tags with different casing
    imgStart = 0;
    while ((imgStart = content.indexOf("<IMG", imgStart)) != -1) {
        int imgEnd = content.indexOf(">", imgStart);
        if (imgEnd != -1) {
            content = content.substring(0, imgStart) + content.substring(imgEnd + 1);
        } else {
            content = content.substring(0, imgStart);
            break;
        }
    }
    
    // Remove SVG elements
    removeNestedTag(content, "<svg", "</svg>");
    removeNestedTag(content, "<SVG", "</SVG>");
    
    // Remove script and style tags
    removeNestedTag(content, "<script", "</script>");
    removeNestedTag(content, "<style", "</style>");
    
    // Remove other media-related tags
    removeNestedTag(content, "<video", "</video>");
    removeNestedTag(content, "<audio", "</audio>");
    removeNestedTag(content, "<iframe", "</iframe>");
    removeNestedTag(content, "<canvas", "</canvas>");
    removeNestedTag(content, "<embed", "</embed>");
    removeNestedTag(content, "<object", "</object>");
    removeNestedTag(content, "<param", ">");
    
    // Remove social media embeds and widgets
    removeNestedTag(content, "<div class=\"twitter-tweet\"", "</div>");
    removeNestedTag(content, "<div class=\"instagram-media\"", "</div>");
    removeNestedTag(content, "<div class=\"fb-post\"", "</div>");
    removeNestedTag(content, "<div class=\"youtube-embed\"", "</div>");
    
    // Remove any remaining divs with image-related classes
    removeNestedTag(content, "<div class=\"image\"", "</div>");
    removeNestedTag(content, "<div class=\"img\"", "</div>");
    removeNestedTag(content, "<div class=\"photo\"", "</div>");
    removeNestedTag(content, "<div class=\"media\"", "</div>");
    removeNestedTag(content, "<div class=\"embed\"", "</div>");
    
    // Remove data URLs and base64 encoded content that might be images
    int dataStart = 0;
    while ((dataStart = content.indexOf("data:image/", dataStart)) != -1) {
        int dataEnd = content.indexOf("\"", dataStart);
        if (dataEnd != -1) {
            // Remove the entire data URL
            content = content.substring(0, dataStart) + content.substring(dataEnd + 1);
        } else {
            // If no closing quote, remove from data start to end
            content = content.substring(0, dataStart);
            break;
        }
    }
    
    // Remove any remaining image-related attributes
    removeAttribute(content, "src=\"data:image/");
    removeAttribute(content, "background=\"data:image/");
    removeAttribute(content, "style=\"background-image:");
    
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

void NewsletterCard::removeAttribute(String& content, const String& attribute) {
    int attrStart = 0;
    while ((attrStart = content.indexOf(attribute, attrStart)) != -1) {
        int attrEnd = content.indexOf(" ", attrStart);
        if (attrEnd != -1) {
            content.remove(attrStart, attrEnd - attrStart + 1);
        } else {
            content.remove(attrStart);
        }
    }
} 