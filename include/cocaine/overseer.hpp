#ifndef COCAINE_OVERSEER_HPP
#define COCAINE_OVERSEER_HPP

#include <boost/thread.hpp>

#include "cocaine/common.hpp"
#include "cocaine/forwards.hpp"
#include "cocaine/networking.hpp"

namespace cocaine { namespace engine {

// Thread manager
class overseer_t:
    public boost::noncopyable,
    public unique_id_t
{
    public:
        overseer_t(unique_id_t::reference id,
                   zmq::context_t& context,
                   const std::string& name);
        ~overseer_t();

        // Thread entry point 
#if BOOST_VERSION >= 103500
        void operator()(boost::shared_ptr<plugin::source_t> source);
#else
        void run(boost::shared_ptr<plugin::source_t> source);
#endif

        void ensure();

    private:
        // Event loop callback handling and dispatching
        void message(ev::io& w, int revents);
        void process_message(ev::idle& w, int revents);
        void timeout(ev::timer& w, int revents);
        void heartbeat(ev::timer& w, int revents);

        void terminate();

    private:
        // Messaging
        zmq::context_t& m_context;
        lines::channel_t m_messages;
        
        // App name & engine endpoint
        std::string m_name;
        
        // Event loop
        ev::dynamic_loop m_loop;
        ev::io m_message_watcher;
        ev::idle m_message_processor;
        ev::timer m_suicide_timer, m_heartbeat_timer;

        // Initialization interlocking
        boost::mutex m_mutex;
        boost::unique_lock<boost::mutex> m_lock;
        boost::condition_variable m_ready;
        
        // Data source
        boost::shared_ptr<plugin::source_t> m_source;
};

}}

#endif
