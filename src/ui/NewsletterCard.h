#pragma once

#include <lvgl.h>
#include <memory>
#include <vector>
#include <functional>
#include "ConfigManager.h"
#include "EventQueue.h"
#include "posthog/RssClient.h"
#include "ui/InputHandler.h"

/**
 * @class NewsletterCard
 * @brief UI component for displaying and reading RSS newsletter content
 * 
 * Features:
 * - Displays new newsletter notifications
 * - Full article reading interface with pagination
 * - Clean text display without HTML formatting
 * - Center button navigation through content
 * - Automatic RSS feed fetching
 */
class NewsletterCard : public InputHandler {
public:
    /**
     * @brief Constructor
     * 
     * @param parent LVGL parent object to attach this card to
     * @param config Configuration manager for persistent storage
     * @param eventQueue Event queue for receiving data updates
     * @param rssClient RSS client for fetching feed data
     * @param width Card width in pixels
     * @param height Card height in pixels
     */
    NewsletterCard(lv_obj_t* parent, ConfigManager& config, EventQueue& eventQueue,
                   RssClient& rssClient, uint16_t width, uint16_t height);
    
    /**
     * @brief Destructor - safely cleans up UI resources
     */
    ~NewsletterCard();
    
    /**
     * @brief Get the underlying LVGL card object
     * @return LVGL object pointer for the main card container
     */
    lv_obj_t* getCard() const { return _card; }
    
    /**
     * @brief Handle button press events
     * @param button_index The index of the button that was pressed
     * @return true if the event was handled, false otherwise
     */
    bool handleButtonPress(uint8_t button_index) override;
    
    /**
     * @brief Update the card display based on current state
     */
    void updateDisplay();
    
    /**
     * @brief Get the current feed URL
     * @return The configured RSS feed URL or PostHog default
     */
    String getFeedUrl() const;
    
    /**
     * @brief Force a feed refresh
     */
    void refreshFeed();
    
    /**
     * @brief Handle periodic updates
     * 
     * Called periodically to check for new content and update display
     */
    void handlePeriodicUpdate();

private:
    // UI States
    enum class DisplayState {
        IDLE,           ///< No new newsletters
        NEW_NOTIFICATION, ///< New newsletter notification
        READING         ///< Reading article content
    };
    
    // Configuration and state
    ConfigManager& _config;              ///< Configuration manager reference
    EventQueue& _eventQueue;             ///< Event queue reference
    RssClient& _rssClient;               ///< RSS client reference
    DisplayState _currentState;          ///< Current display state
    const RssItem* _currentArticle;      ///< Currently displayed article
    int _currentPage;                    ///< Current page in article
    std::vector<String> _articlePages;   ///< Paginated article content
    
    // UI Elements
    lv_obj_t* _card;                     ///< Main card container
    lv_obj_t* _title_label;              ///< Title text label
    lv_obj_t* _content_label;            ///< Content text label (idle mode)
    lv_obj_t* _reading_content_label;    ///< Content text label (reading mode)
    lv_obj_t* _status_label;             ///< Status/info label
    lv_obj_t* _reading_container;        ///< Container for reading mode layout
    lv_obj_t* _idle_container;           ///< Container for idle mode layout
    lv_obj_t* _progress_bar;             ///< Progress indicator label for reading mode
    lv_obj_t* _page_indicator;           ///< Page indicator label
    
    // Constants
    static constexpr int MAX_LINES_PER_PAGE = 5;   ///< Exactly 5 lines per page for optimal reading
    static constexpr int REFRESH_INTERVAL = 300000; ///< Refresh interval (5 minutes)
    static constexpr int READING_PADDING = 4;      ///< Minimal padding for reading mode
    static constexpr int IDLE_PADDING = 4;         ///< Minimal padding for idle mode
    unsigned long _lastRefreshTime;      ///< Last refresh timestamp
    
    /**
     * @brief Handle events from the event queue
     * @param event Event containing data updates
     */
    void onEvent(const Event& event);
    
    /**
     * @brief Switch to idle state (no new newsletters)
     */
    void showIdleState();
    
    /**
     * @brief Switch to new newsletter notification state
     */
    void showNewNotificationState();
    
    /**
     * @brief Switch to reading state
     */
    void showReadingState();
    
    /**
     * @brief Paginate article content for display
     * @param content The full article content
     */
    void paginateContent(const String& content);
    
    /**
     * @brief Update the reading display with current page
     */
    void updateReadingDisplay();
    
    /**
     * @brief Check if it's time to refresh the feed
     * @return true if refresh is needed
     */
    bool shouldRefresh() const;
    
    /**
     * @brief Initialize the RSS client with configured or default feed
     */
    void initializeFeed();
    
    /**
     * @brief Strip HTML tags and decode entities from content
     * @param htmlContent The HTML content to clean
     * @return Clean plain text content
     */
    String stripHtmlAndDecodeEntities(const String& htmlContent);
    
    /**
     * @brief Remove image tags and media elements from HTML content
     * @param content The HTML content to process (modified in place)
     */
    void removeImageTags(String& content);
    
    /**
     * @brief Remove nested HTML tags and their content
     * @param content The HTML content to process (modified in place)
     * @param openTag The opening tag to match
     * @param closeTag The closing tag to match
     */
    void removeNestedTag(String& content, const String& openTag, const String& closeTag);
}; 