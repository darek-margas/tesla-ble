/**
 * @file message_processor.cpp
 * @brief Ordered message processing implementation
 */

#include "message_processor.h"
#include "defs.h"
#include "vehicle.h"

namespace TeslaBLE {

MessageProcessor::MessageProcessor(MessageHandler handler) : message_handler_(std::move(handler)) {}

MessageProcessor::~MessageProcessor() = default;

void MessageProcessor::queue_message(const UniversalMessage_RoutableMessage &msg) {
  std::scoped_lock lock(queue_mutex_);
  message_queue_.push(msg);

  // Limit queue size to prevent memory exhaustion
  if (message_queue_.size() > MessageProcessor::MAX_QUEUE_SIZE) {
    LOG_WARNING("Message queue size limit reached, dropping oldest messages");
    message_queue_.pop();
  }
}

size_t MessageProcessor::process_messages() {
  // Called on every loop, usually with nothing queued. It used to build a
  // local std::queue first: a std::deque allocates its map and a node (one
  // ~750-byte RoutableMessage) even when empty, so every loop did two heap
  // allocations. Under BLE + Wi-Fi load that churn eventually failed and,
  // with exceptions disabled, aborted (seen on an ESP32). Now nothing is
  // allocated: messages are taken off the queue one at a time.
  size_t to_process;
  {
    std::scoped_lock lock(queue_mutex_);
    if (message_queue_.empty()) {
      return 0;
    }
    processing_ = true;
    // Only what is queued now; messages queued meanwhile wait for the next call
    to_process = message_queue_.size();
  }

  struct ProcessingGuard {
    MessageProcessor *processor;
    ~ProcessingGuard() {
      std::scoped_lock guard(processor->queue_mutex_);
      processor->processing_ = false;
    }
  } guard{this};

  size_t processed = 0;
  for (size_t taken = 0; taken < to_process; ++taken) {
    UniversalMessage_RoutableMessage msg = UniversalMessage_RoutableMessage_init_default;
    {
      std::scoped_lock lock(queue_mutex_);
      if (message_queue_.empty()) {
        break;
      }
      msg = message_queue_.front();
      message_queue_.pop();
    }
    if (message_handler_) {
      message_handler_(msg);
      processed++;
    }
  }

  return processed;
}

bool MessageProcessor::is_processing() const {
  std::scoped_lock lock(queue_mutex_);
  return processing_;
}

size_t MessageProcessor::get_queue_size() const {
  std::scoped_lock lock(queue_mutex_);
  return message_queue_.size();
}

void MessageProcessor::clear_queue() {
  std::scoped_lock lock(queue_mutex_);
  std::queue<UniversalMessage_RoutableMessage> empty;
  message_queue_.swap(empty);
}

// Global instance - now must be initialized with handler
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
std::unique_ptr<MessageProcessor> g_message_processor = nullptr;

void initialize_message_processor(MessageProcessor::MessageHandler handler) {
  if (!g_message_processor) {
    g_message_processor = std::make_unique<MessageProcessor>(std::move(handler));
    LOG_INFO("Global message processor initialized");
  }
}

void cleanup_message_processor() {
  if (g_message_processor) {
    g_message_processor->clear_queue();
    g_message_processor.reset();
    LOG_INFO("Global message processor cleaned up");
  }
}

}  // namespace TeslaBLE
