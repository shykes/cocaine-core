/*
    Copyright (c) 2011-2012 Andrey Sibiryov <me@kobology.ru>
    Copyright (c) 2011-2012 Other contributors as noted in the AUTHORS file.

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

#ifndef COCAINE_ISOLATE_API_HPP
#define COCAINE_ISOLATE_API_HPP

#include <boost/thread/mutex.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/weak_ptr.hpp>

#include "cocaine/common.hpp"
#include "cocaine/repository.hpp"

#include "cocaine/helpers/json.hpp"

namespace cocaine { namespace api {

class handle_t {
    public:
        virtual
        ~handle_t() {
            // Empty.
        }

        virtual
        void
        terminate() = 0;
};

class isolate_t:
    public boost::noncopyable
{
    public:
        virtual
        ~isolate_t() {
            // Empty.
        }
        
        virtual
        std::unique_ptr<handle_t>
        spawn(const std::string& path,
              const std::map<std::string, std::string>& args,
              const std::map<std::string, std::string>& environment) = 0;

    protected:
        isolate_t(context_t&,
                  const std::string&, /* name */
                  const Json::Value&  /* args */)
        {
           // Empty. 
        }
};

template<>
struct category_traits<isolate_t> {
    typedef boost::shared_ptr<isolate_t> ptr_type;

    typedef boost::tuple<
        const std::string&,
        const Json::Value&
    > args_type;
    
    template<class T>
    struct default_factory:
        public factory<isolate_t>
    {
        virtual
        ptr_type
        get(context_t& context,
            const args_type& args)
        {
            boost::lock_guard<boost::mutex> lock(m_mutex);

            const std::string& name(boost::get<0>(args));
            typename instance_map_t::iterator it(m_instances.find(name));
            
            ptr_type instance;
            
            if(it != m_instances.end()) {
                instance = it->second.lock();
            }

            if(!instance) {
                instance =  boost::make_shared<T>(
                    boost::ref(context),
                    name,
                    boost::get<1>(args)
                );

                m_instances.emplace(name, instance);
            }
                
            return instance;
        }

    private:
#if BOOST_VERSION >= 103600
        typedef boost::unordered_map<
#else
        typedef std::map<
#endif
            std::string,
            boost::weak_ptr<isolate_t>
        > instance_map_t;

        instance_map_t m_instances;
        boost::mutex m_mutex;
    };
};

}} // namespace cocaine::api

#endif
