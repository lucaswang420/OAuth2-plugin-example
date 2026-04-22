/**
 * Debug version of test_main to investigate Drogon teardown crash
 *
 * This version attempts normal exit instead of std::_Exit(0)
 * to identify the root cause of the segmentation fault.
 */

#include <drogon/drogon_test.h>
#include <drogon/drogon.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <csignal>
#include <sstream>
#include <filesystem>

// For backtrace
#include <execinfo.h>
#include <cxxabi.h>

// For std::filesystem (C++17)
#if __has_include(<filesystem>)
#include <filesystem>
namespace fs = std::filesystem;
#else
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#endif

// Debug output macros
#define DEBUG_OUT(msg)                                                   \
    std::cout << "[DEBUG] " << __FILE__ << ":" << __LINE__ << " " << msg \
              << std::endl

#define DEBUG_ERR(msg)                                                   \
    std::cerr << "[ERROR] " << __FILE__ << ":" << __LINE__ << " " << msg \
              << std::endl

// Global flag to track if we're in teardown
std::atomic<bool> g_in_teardown{false};
std::atomic<bool> g_crash_occurred{false};

// Signal handler for segmentation fault
void signal_handler(int signal)
{
    g_crash_occurred = true;

    std::cerr << "\n========== CRASH DETECTED ==========" << std::endl;
    std::cerr << "Signal: " << signal << std::endl;

    if (signal == SIGSEGV)
    {
        std::cerr << "Segmentation Fault (SIGSEGV)" << std::endl;
    }
    else if (signal == SIGABRT)
    {
        std::cerr << "Abort (SIGABRT)" << std::endl;
    }
    else if (signal == SIGFPE)
    {
        std::cerr << "Floating Point Exception (SIGFPE)" << std::endl;
    }
    else
    {
        std::cerr << "Unknown signal" << std::endl;
    }

    std::cerr << "In teardown: " << (g_in_teardown.load() ? "YES" : "NO")
              << std::endl;

    // Print stack trace if available
    std::cerr << "\nAttempting to print stack trace..." << std::endl;

    void *array[25];
    size_t size = ::backtrace(array, 25);
    char **strings = ::backtrace_symbols(array, size);

    if (strings != nullptr)
    {
        std::cerr << "\nRaw stack trace (" << size << " frames):" << std::endl;
        for (size_t i = 0; i < size; i++)
        {
            std::cerr << "  " << strings[i] << std::endl;

            // Try to demangle C++ symbols
            char *demangled = nullptr;
            int status = 0;
            demangled = abi::__cxa_demangle(strings[i], nullptr, 0, &status);
            if (status == 0 && demangled != nullptr)
            {
                std::cerr << "    -> " << demangled << std::endl;
                free(demangled);
            }
        }
        free(strings);
    }

    std::cerr << "\n=====================================" << std::endl;
    std::cerr.flush();

    // Exit immediately to avoid further damage
    _exit(128 + signal);  // Exit with signal code
}

// Setup signal handlers
void setup_signal_handlers()
{
    DEBUG_OUT("Setting up signal handlers...");

    signal(SIGSEGV, signal_handler);  // Segmentation fault
    signal(SIGABRT, signal_handler);  // Abort
    signal(SIGFPE, signal_handler);   // Floating point exception
    signal(SIGILL, signal_handler);   // Illegal instruction

    DEBUG_OUT("Signal handlers installed");
}

// Teardown monitoring class
class TeardownMonitor
{
  public:
    ~TeardownMonitor()
    {
        std::cout << "\n========== TEARDOWN START ==========" << std::endl;
        std::cout << "TeardownMonitor destructor called" << std::endl;
        g_in_teardown = true;
        std::cout.flush();
    }
};

// Global monitor (constructed early, destroyed late)
TeardownMonitor g_teardown_monitor;

namespace test_config
{
const int APP_STARTUP_TIMEOUT_SECONDS = 60;
const int APP_PREWARM_MS = 500;
}  // namespace test_config

int main(int argc, char **argv)
{
    using namespace drogon;

    std::cout << "========================================" << std::endl;
    std::cout << "Drogon Test - DEBUG VERSION" << std::endl;
    std::cout << "Purpose: Investigate teardown crash" << std::endl;
    std::cout << "========================================\n" << std::endl;

    // 1. Setup signal handlers FIRST
    setup_signal_handlers();

    // 2. Load configuration
    std::string configPath = "./config.json";
    if (!fs::exists(configPath))
        configPath = "../config.json";
    if (!fs::exists(configPath))
        configPath = "../../config.json";
    if (!fs::exists(configPath))
        configPath = "../../../config.json";

    if (!fs::exists(configPath))
    {
        std::cerr << "ERROR: config.json not found!" << std::endl;
        return 1;
    }

    std::cout << "Found config: " << configPath << std::endl;

    // 3. Load Drogon framework
    std::cout << "\n========== STEP 1: LOAD CONFIG ==========" << std::endl;
    app().loadConfigFile(configPath);
    DEBUG_OUT("Drogon config loaded");

    // 4. Start Drogon in background thread
    std::cout << "\n========== STEP 2: START DROGON ==========" << std::endl;

    std::promise<void> p1;
    std::future<void> f1 = p1.get_future();
    std::atomic<bool> signalingStarted{false};

    std::thread thr([&]() {
        std::cout << "[Thread] Drogon thread starting..." << std::endl;

        try
        {
            app().registerBeginningAdvice([&]() {
                std::cout << "[Drogon] App ready, signaling main thread..."
                          << std::endl;
                bool expected = false;
                if (signalingStarted.compare_exchange_strong(expected, true))
                {
                    p1.set_value();
                }
            });

            std::cout << "[Thread] Calling app().run()..." << std::endl;
            app().run();
            std::cout << "[Thread] app().run() returned" << std::endl;
        }
        catch (const std::exception &e)
        {
            std::cerr << "[Thread] Exception in app().run(): " << e.what()
                      << std::endl;
            bool expected = false;
            if (signalingStarted.compare_exchange_strong(expected, true))
            {
                p1.set_value();
            }
        }
        catch (...)
        {
            std::cerr << "[Thread] Unknown exception in app().run()"
                      << std::endl;
            bool expected = false;
            if (signalingStarted.compare_exchange_strong(expected, true))
            {
                p1.set_value();
            }
        }
    });

    // 5. Wait for Drogon to start
    std::cout << "\n========== STEP 3: WAIT FOR READY ==========" << std::endl;
    std::cout << "Waiting for Drogon to be ready..." << std::endl;

    if (f1.wait_for(
            std::chrono::seconds(test_config::APP_STARTUP_TIMEOUT_SECONDS)) !=
        std::future_status::ready)
    {
        std::cerr << "TIMEOUT: Drogon failed to start!" << std::endl;
        g_in_teardown = true;
        thr.detach();
        return 1;
    }
    f1.get();

    std::cout << "Drogon is ready!" << std::endl;

    // 6. Pre-warm
    std::cout << "\n========== STEP 4: PRE-WARM ==========" << std::endl;
    std::cout << "Pre-warming (500ms)..." << std::endl;
    std::this_thread::sleep_for(
        std::chrono::milliseconds(test_config::APP_PREWARM_MS));

    // 7. Run tests
    std::cout << "\n========== STEP 5: RUN TESTS ==========" << std::endl;
    std::cout << "Executing test::run()..." << std::endl;
    std::cout.flush();

    int status = test::run(argc, argv);

    std::cout << "test::run() completed with status: " << status << std::endl;
    std::cout.flush();

    if (status != 0)
    {
        std::cout << "\n========== TESTS FAILED ==========" << std::endl;
        std::cout << "Tests failed, skipping teardown analysis" << std::endl;
        app().getLoop()->queueInLoop([]() { app().quit(); });
        if (thr.joinable())
        {
            thr.join();
        }
        return status;
    }

    // 8. TEARDOWN - This is where the crash happens
    std::cout << "\n========== STEP 6: NORMAL TEARDOWN ==========" << std::endl;
    std::cout << "Tests passed! Attempting normal teardown..." << std::endl;
    std::cout << "This is where crashes occur on Linux..." << std::endl;
    std::cout.flush();

    g_in_teardown = true;

    // Stop Drogon event loop
    std::cout << "\n[Teardown] Stopping event loop..." << std::endl;
    std::cout.flush();

    try
    {
        app().getLoop()->queueInLoop([]() {
            std::cout << "[Teardown] Quitting event loop..." << std::endl;
            std::cout.flush();
            app().quit();
        });

        std::cout << "[Teardown] Waiting for event loop to stop..."
                  << std::endl;
        std::cout.flush();

        if (thr.joinable())
        {
            std::cout << "[Teardown] Joining Drogon thread..." << std::endl;
            std::cout.flush();
            thr.join();
            std::cout << "[Teardown] Thread joined successfully" << std::endl;
        }

        std::cout << "[Teardown] Event loop stopped" << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cerr << "[Teardown] Exception during teardown: " << e.what()
                  << std::endl;
    }

    // 9. Global destructors are about to run
    std::cout << "\n[Teardown] About to run global destructors..." << std::endl;
    std::cout << "This is where crashes typically occur:" << std::endl;
    std::cout << "  - Drogon app() destructor" << std::endl;
    std::cout << "  - Plugin destructors" << std::endl;
    std::cout << "  - Database client destructors" << std::endl;
    std::cout << "  - Thread pool destructors" << std::endl;
    std::cout.flush();

    // Enable detailed logging during destructor phase
    std::cout << "\n========== DESTRUCTOR PHASE ==========" << std::endl;
    std::cout << "Entering global destructor phase..." << std::endl;
    std::cout << "Watch for crash after this message..." << std::endl;
    std::cout.flush();

    // If we reach here, teardown succeeded!
    std::cout << "\n========== TEARDOWN SUCCESS! ==========" << std::endl;
    std::cout << "No crash occurred during teardown!" << std::endl;
    std::cout << "Normal exit successful." << std::endl;

    return 0;
}
