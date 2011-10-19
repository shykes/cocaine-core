#ifndef COCAINE_OVERSEER_HPP
#define COCAINE_OVERSEER_HPP

#include "cocaine/common.hpp"
#include "cocaine/forwards.hpp"
#include "cocaine/networking.hpp"

namespace cocaine { namespace engine {

// Thread manager
class overseer_t:
    public boost::noncopyable,
    public helpers::unique_id_t
{
    public:
        overseer_t(zmq::context_t& context,
                   helpers::unique_id_t::type engine_id,
                   boost::shared_ptr<plugin::source_t> source);
        ~overseer_t();

        // Thread entry point 
        void run();
        
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
        
        // Event loop
        ev::dynamic_loop m_loop;
        ev::io m_message_watcher;
        ev::idle m_message_processor;
        ev::timer m_suicide_timer, m_heartbeat_timer;

        // Data source
        boost::shared_ptr<plugin::source_t> m_source;
};

}}

#endif
