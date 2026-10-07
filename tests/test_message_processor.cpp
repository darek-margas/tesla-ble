#include <gtest/gtest.h>
#include <message_processor.h>

#include <vector>

using namespace TeslaBLE;

namespace {
UniversalMessage_RoutableMessage message_with_flags(uint32_t flags) {
  UniversalMessage_RoutableMessage msg = UniversalMessage_RoutableMessage_init_default;
  msg.flags = flags;
  return msg;
}
}  // namespace

TEST(MessageProcessorTest, EmptyQueueProcessesNothing) {
  int calls = 0;
  MessageProcessor processor([&](const UniversalMessage_RoutableMessage &) { calls++; });
  EXPECT_EQ(processor.process_messages(), 0U);
  EXPECT_EQ(calls, 0);
  EXPECT_FALSE(processor.is_processing());
}

TEST(MessageProcessorTest, ProcessesQueuedMessagesInOrder) {
  std::vector<uint32_t> seen;
  MessageProcessor processor([&](const UniversalMessage_RoutableMessage &msg) { seen.push_back(msg.flags); });
  for (uint32_t i = 1; i <= 3; ++i) {
    processor.queue_message(message_with_flags(i));
  }
  EXPECT_EQ(processor.process_messages(), 3U);
  EXPECT_EQ(seen, (std::vector<uint32_t>{1, 2, 3}));
  EXPECT_EQ(processor.get_queue_size(), 0U);
  EXPECT_FALSE(processor.is_processing());
}

TEST(MessageProcessorTest, MessagesQueuedWhileProcessingWaitForTheNextCall) {
  MessageProcessor *self = nullptr;
  std::vector<uint32_t> seen;
  MessageProcessor processor([&](const UniversalMessage_RoutableMessage &msg) {
    seen.push_back(msg.flags);
    if (msg.flags == 1) {
      self->queue_message(message_with_flags(99));
    }
  });
  self = &processor;
  processor.queue_message(message_with_flags(1));
  processor.queue_message(message_with_flags(2));

  EXPECT_EQ(processor.process_messages(), 2U);
  EXPECT_EQ(seen, (std::vector<uint32_t>{1, 2}));
  EXPECT_EQ(processor.get_queue_size(), 1U);

  EXPECT_EQ(processor.process_messages(), 1U);
  EXPECT_EQ(seen, (std::vector<uint32_t>{1, 2, 99}));
}

TEST(MessageProcessorTest, BacklogIsBoundedDroppingTheOldest) {
  std::vector<uint32_t> seen;
  MessageProcessor processor([&](const UniversalMessage_RoutableMessage &msg) { seen.push_back(msg.flags); });
  for (uint32_t i = 1; i <= 40; ++i) {
    processor.queue_message(message_with_flags(i));
  }
  EXPECT_LE(processor.get_queue_size(), 16U);
  processor.process_messages();
  ASSERT_FALSE(seen.empty());
  EXPECT_EQ(seen.back(), 40U) << "The newest message is kept";
  EXPECT_GT(seen.front(), 1U) << "The oldest ones are dropped";
}
