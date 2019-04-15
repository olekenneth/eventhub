#ifndef EVENTHUB_TOPIC_MANAGER_HPP
#define EVENTHUB_TOPIC_MANAGER_HPP

#include <memory>
#include <string>
#include <unordered_map>
#include <mutex>
#include "common.hpp"
#include "topic.hpp"
#include "connection.hpp"

namespace eventhub {
  class topic_manager {
    typedef std::unordered_map<std::string, std::unique_ptr<topic> > topic_list_t;

    public:
      void subscribe_connection(std::shared_ptr<io::connection>& conn, const std::string& topic_filter);
      void publish(const std::string& topic_name, const std::string& data);
      void garbage_collect();
      
      static bool is_valid_topic_filter(const std::string& filter_name);
      static bool is_filter_matched(const std::string& filter_name, const string& topic_name);
      static const std::string uri_decode(const std::string& str);

    private:
      topic_list_t _topic_list;
      std::mutex _topic_list_lock;
  };
}

#endif
