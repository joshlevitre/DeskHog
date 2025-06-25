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
     * @brief Set the RSS feed URL
     * @param url The RSS feed URL to monitor
     */
    void setFeedUrl(const String& url);
    
    /**
     * @brief Get the current feed URL
     * @return The current RSS feed URL
     */
    String getFeedUrl() const { return _rssClient.getFeedUrl(); }
    
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
    lv_obj_t* _content_label;            ///< Content text label
    lv_obj_t* _status_label;             ///< Status/info label
    
    // Constants
    static constexpr int MAX_LINES_PER_PAGE = 8;  ///< Maximum lines per page
    static constexpr int MAX_CHARS_PER_LINE = 35; ///< Maximum characters per line
    static constexpr int REFRESH_INTERVAL = 600000; ///< Refresh interval (10 minutes)
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
     * @brief Load the RSS feed URL from storage
     */
    void loadFeedUrl();
    
    /**
     * @brief Save the RSS feed URL to storage
     */
    void saveFeedUrl();
    
    /**
     * @brief Strip HTML tags and decode entities from content
     * @param htmlContent The HTML content to clean
     * @return Clean plain text content
     */
    String stripHtmlAndDecodeEntities(const String& htmlContent);
}; 