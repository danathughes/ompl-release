/*
 [auto_generated]
 boost/numeric/odeint/util/state_wrapper.hpp

 [begin_description]
 State wrapper for the state type in all stepper. The state wrappers are responsible for contruction,
 destruction, copying contruction, assignment and resizing.
 [end_description]

 Copyright 2009-2011 Karsten Ahnert
 Copyright 2009-2011 Mario Mulansky

 Distributed under the Boost Software License, Version 1.0.
 (See accompanying file LICENSE_1_0.txt or
 copy at http://www.boost.org/LICENSE_1_0.txt)
 */


#ifndef OMPLEXT_BOOST_NUMERIC_ODEINT_UTIL_STATE_WRAPPER_HPP_INCLUDED
#define OMPLEXT_BOOST_NUMERIC_ODEINT_UTIL_STATE_WRAPPER_HPP_INCLUDED


#include <boost/type_traits/integral_constant.hpp>

#include <omplext_odeint/boost/numeric/odeint/util/is_resizeable.hpp>
#include <omplext_odeint/boost/numeric/odeint/util/resize.hpp>
#include <omplext_odeint/boost/numeric/odeint/util/same_size.hpp>


namespace boost {
namespace numeric {
namespace omplext_odeint {


template< class V >
struct state_wrapper
{
    typedef state_wrapper< V > state_wrapper_type;

    V m_v;
};


}
}
}



#endif // BOOST_NUMERIC_ODEINT_UTIL_STATE_WRAPPER_HPP_INCLUDED
