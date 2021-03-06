/*
 [auto_generated]
 boost/numeric/odeint/integrate/integrate_times.hpp

 [begin_description]
 Integration of ODEs with observation at user defined points
 [end_description]

 Copyright 2009-2011 Karsten Ahnert
 Copyright 2009-2011 Mario Mulansky

 Distributed under the Boost Software License, Version 1.0.
 (See accompanying file LICENSE_1_0.txt or
 copy at http://www.boost.org/LICENSE_1_0.txt)
 */


#ifndef OMPLEXT_BOOST_NUMERIC_ODEINT_INTEGRATE_INTEGRATE_TIMES_HPP_INCLUDED
#define OMPLEXT_BOOST_NUMERIC_ODEINT_INTEGRATE_INTEGRATE_TIMES_HPP_INCLUDED

#include <boost/type_traits/is_same.hpp>

#include <boost/range.hpp>

#include <omplext_odeint/boost/numeric/odeint/stepper/stepper_categories.hpp>
#include <omplext_odeint/boost/numeric/odeint/integrate/null_observer.hpp>
#include <omplext_odeint/boost/numeric/odeint/integrate/detail/integrate_times.hpp>

namespace boost {
namespace numeric {
namespace omplext_odeint {


/*
 * the two overloads are needed in order to solve the forwarding problem
 */
template< class Stepper , class System , class State , class TimeIterator , class Time , class Observer >
size_t integrate_times(
        Stepper stepper , System system , State &start_state ,
        TimeIterator times_start , TimeIterator times_end , Time dt ,
        Observer observer )
{
    return detail::integrate_times(
            stepper , system , start_state ,
            times_start , times_end , dt ,
            observer , typename Stepper::stepper_category() );
}

template< class Stepper , class System , class State , class TimeIterator , class Time , class Observer >
size_t integrate_times(
        Stepper stepper , System system , const State &start_state ,
        TimeIterator times_start , TimeIterator times_end , Time dt ,
        Observer observer )
{
    return detail::integrate_times(
            stepper , system , start_state ,
            times_start , times_end , dt ,
            observer , typename Stepper::stepper_category() );
}

/*
 * Range Version:
 * the two overloads are needed in order to solve the forwarding problem
 */
template< class Stepper , class System , class State , class TimeRange , class Time , class Observer >
size_t integrate_times(
        Stepper stepper , System system , State &start_state ,
        const TimeRange &times , Time dt ,
        Observer observer )
{
    return integrate_times(
            stepper , system , start_state ,
            boost::begin( times ) , boost::end( times ) , dt , observer );
}

template< class Stepper , class System , class State , class TimeRange , class Time , class Observer >
size_t integrate_times(
        Stepper stepper , System system , const State &start_state ,
        const TimeRange &times , Time dt ,
        Observer observer )
{
    return integrate_times(
            stepper , system , start_state ,
            boost::begin( times ) , boost::end( times ) , dt , observer );
}

} // namespace omplext_odeint
} // namespace numeric
} // namespace boost



#endif // BOOST_NUMERIC_ODEINT_INTEGRATE_INTEGRATE_TIMES_HPP_INCLUDED
