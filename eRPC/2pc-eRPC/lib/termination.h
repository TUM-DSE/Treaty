#include <atomic>

/**
 * Terminate gracefully in case Ctrl+C is pressed.
 */
extern std::atomic<bool> ctrl_c_pressed;
