#include "OAuth2CleanupService.h"

namespace oauth2
{

OAuth2CleanupService::OAuth2CleanupService(IOAuth2Storage *storage)
    : storage_(storage)
{
}

OAuth2CleanupService::~OAuth2CleanupService()
{
    stop();
}

void OAuth2CleanupService::start(double intervalSeconds)
{
    if (running_)
        return;

    // Guard: only start if the event loop is available
    auto loop = drogon::app().getLoop();
    if (!loop)
    {
        LOG_WARN << "Event loop not available, OAuth2 Cleanup Service will "
                    "not start";
        return;
    }

    running_ = true;
    LOG_INFO << "Starting OAuth2 Cleanup Service with interval: "
             << intervalSeconds << "s";

    timerId_ = loop->runEvery(intervalSeconds, [this]() { runCleanup(); });
}

void OAuth2CleanupService::stop()
{
    // Simply mark as stopped. Timers registered with the event loop will
    // not be explicitly invalidated here. This is safe because:
    // 1. If called during plugin shutdown (loop still running), the timer
    //    interval is 3600s - it won't fire before the loop exits.
    // 2. If called after the loop has stopped, the timer won't fire anyway.
    // 3. The runCleanup() callback always checks running_ first, so even
    //    if the timer somehow fires, it will be a no-op.
    // Explicitly calling invalidateTimer here can crash on Windows when
    // called from a non-loop thread after the loop is stopping, because
    // the internal wakeup() mechanism uses IOCP handles that may be closing.
    if (running_ && timerId_ > 0)
    {
        LOG_INFO << "Stopping OAuth2 Cleanup Service";
        timerId_ = 0;
    }
    running_ = false;
}

void OAuth2CleanupService::runCleanup()
{
    if (!running_ || !storage_)
        return;

    LOG_DEBUG << "Running periodic data cleanup...";
    try
    {
        storage_->deleteExpiredData();
    }
    catch (const std::exception &e)
    {
        LOG_ERROR << "Error during OAuth2 cleanup: " << e.what();
    }
    catch (...)
    {
        LOG_ERROR << "Unknown error during OAuth2 cleanup";
    }
}

}  // namespace oauth2
