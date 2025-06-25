# Newsletter Reader Card

The Newsletter Reader card displays the latest newsletter from an RSS feed on your DeskHog device. It automatically fetches the most recent newsletter and allows you to read the full content with pagination.

## Features

- **Always Shows Latest**: Displays the most recent newsletter from the configured RSS feed
- **Full Article Reading**: Read complete newsletter content with pagination
- **Clean Text Display**: Strips HTML formatting for easy reading on the small screen
- **Automatic Refresh**: Fetches new content every 10 minutes when WiFi is connected
- **Center Button Navigation**: Use the center button to navigate through content
- **Configurable Feed**: Set any RSS feed URL through the web interface

## How to Use

1. **Add the Card**: Use the DeskHog web interface to add a "Newsletter Reader" card
2. **Configure Feed URL**: Enter your desired RSS feed URL (defaults to PostHog Substack)
3. **Navigate**: Use the arrow buttons to navigate to the Newsletter Reader card
4. **Read Content**: Press the center button to open the latest newsletter
5. **Navigate Pages**: Press center button to advance through article pages
6. **Return to List**: After reading, you'll return to the newsletter list

## Button Controls

- **Center Button**: 
  - Opens the latest newsletter for reading
  - Advances to next page when reading
  - Returns to newsletter list when finished reading
- **Arrow Buttons**: Navigate between different cards

## Configuration

### Feed URL
Set the RSS feed URL through the web interface. The card will automatically fetch and display the latest newsletter from this feed.

### Default Settings
- **Refresh Interval**: 10 minutes
- **Default Feed**: PostHog Substack
- **Max Lines Per Page**: 8 lines
- **Max Characters Per Line**: 35 characters

## Technical Details

### RSS Parsing
- Fetches RSS XML content via HTTP
- Parses item titles, descriptions, and full content
- Extracts clean text from HTML content
- Handles HTML entity decoding

### Content Display
- Paginates long articles for easy reading
- Strips HTML formatting for clean text display
- Optimized for the 240x135 screen resolution
- Uses existing DeskHog fonts and styling

### Integration
- Uses the existing CardController system
- Integrates with ConfigManager for persistent settings
- Follows DeskHog's event-driven architecture
- Compatible with existing UI patterns

## Troubleshooting

### No Content Displayed
- Check your WiFi connection
- Verify the RSS feed URL is accessible
- Try refreshing the feed manually (center button)

### Content Not Updating
- The feed refreshes every 10 minutes automatically
- Press center button to force a manual refresh
- Check if the RSS feed has new content

### Reading Issues
- Use center button to navigate through pages
- Content is automatically paginated for the screen size
- Long articles may have multiple pages

## Development

The Newsletter Reader is built using:
- **RssClient**: Handles RSS feed fetching and parsing
- **NewsletterCard**: LVGL-based UI component
- **ConfigManager**: Persistent configuration storage
- **EventQueue**: Cross-task communication

The card follows DeskHog's established patterns for UI components and integrates seamlessly with the existing card system. 