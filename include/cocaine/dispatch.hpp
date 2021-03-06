/*
    Copyright (c) 2011-2013 Andrey Sibiryov <me@kobology.ru>
    Copyright (c) 2011-2013 Other contributors as noted in the AUTHORS file.

    This file is part of Cocaine.

    Cocaine is free software; you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Cocaine is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef COCAINE_DISPATCH_HPP
#define COCAINE_DISPATCH_HPP

#include "cocaine/common.hpp"

#include "cocaine/rpc/slots/blocking.hpp"
#include "cocaine/rpc/slots/deferred.hpp"

#include <boost/mpl/apply.hpp>

namespace cocaine {

class dispatch_t {
    COCAINE_DECLARE_NONCOPYABLE(dispatch_t)

    public:
        dispatch_t(context_t& context, const std::string& name);

        virtual
       ~dispatch_t();

        template<class Event, class F>
        void
        on(const std::string& name, F callable);

        template<class Event>
        void
        on(std::shared_ptr<slot_concept_t> ptr);

        template<class Event>
        void
        forget();

    public:
        void
        invoke(const io::message_t& message, const api::stream_ptr_t& upstream) const;

        typedef std::map<int, std::string> dispatch_map_t;

        dispatch_map_t
        map() const;

        int
        version() const;

        std::string
        name() const;

    private:
        const std::unique_ptr<logging::log_t> m_log;

        typedef std::map<
            int,
            std::shared_ptr<slot_concept_t>
        > slot_map_t;

        slot_map_t m_slots;

        // It's mutable to enable invoke() and describe() to be const.
        mutable std::mutex m_mutex;

        // For actor's named threads feature.
        const std::string m_name;
};

namespace detail {
    template<class R>
    struct select {
        template<class Sequence>
        struct apply {
            typedef io::blocking_slot<R, Sequence> type;
        };
    };

    template<class R>
    struct select<deferred<R>> {
        template<class Sequence>
        struct apply {
            typedef io::deferred_slot<deferred<R>, Sequence> type;
        };
    };
}

template<class Event, class F>
void
dispatch_t::on(const std::string& name, F callable) {
    typedef typename io::detail::result_of<F>::type result_type;
    typedef typename io::event_traits<Event>::tuple_type tuple_type;

    typedef typename boost::mpl::apply<
        detail::select<result_type>,
        tuple_type
    >::type slot_type;

    on<Event>(std::make_shared<slot_type>(name, callable));
}

template<class Event>
void
dispatch_t::on(std::shared_ptr<slot_concept_t> ptr) {
    const int id = io::event_traits<Event>::id;

    std::lock_guard<std::mutex> guard(m_mutex);

    if(m_slots.find(id) != m_slots.end()) {
        throw cocaine::error_t("duplicate slot %d: %s", id, ptr->name());
    }

    m_slots[id] = ptr;
}

template<class Event>
void
dispatch_t::forget() {
    std::lock_guard<std::mutex> guard(m_mutex);
    m_slots.erase(io::event_traits<Event>::id);
}

} // namespace cocaine

#endif
