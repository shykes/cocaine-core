#include <boost/thread.hpp>

#include "cocaine/overseer.hpp"
#include "cocaine/plugin.hpp"

using namespace cocaine::engine;
using namespace cocaine::engine::drivers;
using namespace cocaine::plugin;

overseer_t::overseer_t(unique_id_t::reference id_, zmq::context_t& context, const std::string& name):
    unique_id_t(id_),
    m_context(context),
    m_messages(m_context, ZMQ_DEALER, id()),
    m_name(name),
    m_loop(),
    m_message_watcher(m_loop),
    m_message_processor(m_loop),
    m_suicide_timer(m_loop),
    m_heartbeat_timer(m_loop),
    m_lock(m_mutex)
{
    m_messages.connect("inproc://engines/" + m_name);
    m_message_watcher.set<overseer_t, &overseer_t::message>(this);
    m_message_watcher.start(m_messages.fd(), ev::READ);
        
    m_message_processor.set<overseer_t, &overseer_t::process_message>(this);
    m_message_processor.start();

    m_suicide_timer.set<overseer_t, &overseer_t::timeout>(this);
    m_suicide_timer.start(config_t::get().engine.suicide_timeout);

    m_heartbeat_timer.set<overseer_t, &overseer_t::heartbeat>(this);
    m_heartbeat_timer.start(0.0, 5.0);
}

overseer_t::~overseer_t() {
    terminate();
}

#if BOOST_VERSION >= 103500
void overseer_t::operator()(boost::shared_ptr<source_t> source) {
#else
void overseer_t::run(boost::shared_ptr<source_t> source) {
#endif
    m_source = source;

    {
        boost::lock_guard<boost::mutex> lock(m_mutex);

        m_messages.send(HEARTBEAT);

        timespec tv = { 0, 150000000 };
        nanosleep(&tv, NULL);

        m_ready.notify_one();
    }

    m_loop.loop();
}

void overseer_t::ensure() {
    m_ready.wait(m_lock);
}

void overseer_t::message(ev::io& w, int revents) {
    if(m_messages.pending() && !m_message_processor.is_active()) {
        m_message_processor.start();
    }
}

void overseer_t::process_message(ev::idle& w, int revents) {
    if(m_messages.pending()) {
        Json::Value result;
        std::string request_id;
        unsigned int code = 0;

        boost::tuple<std::string&, unsigned int&> tier(request_id, code);
        
        m_messages.recv_multi(tier);

        switch(code) {
            case INVOKE: {
                std::string task;

                m_messages.recv(task);

                try {
                    if(!m_messages.has_more()) {
                        result = m_source->invoke(task);
                    } else {
                        std::string blob;
                        m_messages.recv(blob);
                        result = m_source->invoke(task, blob.data(), blob.size());
                    }
                } catch(const std::exception& e) {
                    syslog(LOG_ERR, "worker [%s:%s]: source invocation failed - %s", 
                        m_name.c_str(), id().c_str(), e.what());
                    result["error"] = e.what();
                }

                boost::this_thread::interruption_point();
                
                m_messages.send_multi(boost::make_tuple(
                    FUTURE,
                    request_id,
                    result));

                // XXX: Damn, ZeroMQ, why are you so strange? 
                m_loop.feed_fd_event(m_messages.fd(), ev::READ);
                
                m_suicide_timer.stop();
                m_suicide_timer.start(config_t::get().engine.suicide_timeout);
                
                break;
            }
            
            case TERMINATE:
                terminate();
                return;

            default:
                syslog(LOG_DEBUG, "worker [%s:%s]: trash on channel", 
                    m_name.c_str(), id().c_str());
        }
    } else {
        m_message_processor.stop();
    }
}
 
void overseer_t::timeout(ev::timer& w, int revents) {
    m_messages.send(SUICIDE);
    terminate();
}

void overseer_t::heartbeat(ev::timer& w, int revents) {
    m_messages.send(HEARTBEAT);
}

void overseer_t::terminate() {
    m_loop.unloop();
} 

