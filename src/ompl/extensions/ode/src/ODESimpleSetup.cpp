/*********************************************************************
* Software License Agreement (BSD License)
*
*  Copyright (c) 2010, Rice University
*  All rights reserved.
*
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions
*  are met:
*
*   * Redistributions of source code must retain the above copyright
*     notice, this list of conditions and the following disclaimer.
*   * Redistributions in binary form must reproduce the above
*     copyright notice, this list of conditions and the following
*     disclaimer in the documentation and/or other materials provided
*     with the distribution.
*   * Neither the name of the Rice University nor the names of its
*     contributors may be used to endorse or promote products derived
*     from this software without specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
*  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
*  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
*  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
*  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
*  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
*  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
*  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
*  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
*  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
*  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
*  POSSIBILITY OF SUCH DAMAGE.
*********************************************************************/

/* Author: Ioan Sucan */

#include "ompl/extensions/ode/ODESimpleSetup.h"
#include "ompl/util/Exception.h"
#include <boost/thread.hpp>

ompl::control::ODESimpleSetup::ODESimpleSetup(const ControlSpacePtr &space) : SimpleSetup(space)
{
    if (!dynamic_cast<ODEControlSpace*>(space.get()))
        throw Exception("ODE Control Space needed for ODE Simple Setup");
    useEnvParams();
}

ompl::control::ODESimpleSetup::ODESimpleSetup(const base::StateSpacePtr &space) :
    SimpleSetup(ControlSpacePtr(new ODEControlSpace(space)))
{
    useEnvParams();
}

ompl::control::ODESimpleSetup::ODESimpleSetup(const ODEEnvironmentPtr &env) :
    SimpleSetup(ControlSpacePtr(new ODEControlSpace(base::StateSpacePtr(new ODEStateSpace(env)))))
{
    useEnvParams();
}

void ompl::control::ODESimpleSetup::useEnvParams(void)
{
    si_->setPropagationStepSize(getStateSpace()->as<ODEStateSpace>()->getEnvironment()->stepSize_);
    si_->setMinMaxControlDuration(getStateSpace()->as<ODEStateSpace>()->getEnvironment()->minControlSteps_,
                                  getStateSpace()->as<ODEStateSpace>()->getEnvironment()->maxControlSteps_);
    si_->setStatePropagator(StatePropagatorPtr(new ODEStatePropagator(si_)));
}

ompl::base::ScopedState<ompl::control::ODEStateSpace> ompl::control::ODESimpleSetup::getCurrentState(void) const
{
    base::ScopedState<ODEStateSpace> current(getStateSpace());
    getStateSpace()->as<ODEStateSpace>()->readState(current.get());
    return current;
}

void ompl::control::ODESimpleSetup::setCurrentState(const base::State *state)
{
    getStateSpace()->as<ODEStateSpace>()->writeState(state);
}

void ompl::control::ODESimpleSetup::setCurrentState(const base::ScopedState<> &state)
{
    getStateSpace()->as<ODEStateSpace>()->writeState(state.get());
}

void ompl::control::ODESimpleSetup::setup(void)
{
    if (!si_->getStateValidityChecker())
    {
        msg_.inform("Using default state validity checker for ODE");
        si_->setStateValidityChecker(base::StateValidityCheckerPtr(new ODEStateValidityChecker(si_)));
    }
    if (pdef_->getStartStateCount() == 0)
    {
        msg_.inform("Using the initial state of ODE as the starting state for the planner");
        pdef_->addStartState(getCurrentState());
    }
    SimpleSetup::setup();
}

void ompl::control::ODESimpleSetup::playSolutionPath(double timeFactor) const
{
    if (haveSolutionPath())
        playPath(getGoal()->getSolutionPath(), timeFactor);
}

void ompl::control::ODESimpleSetup::playPath(const base::PathPtr &path, double timeFactor) const
{
    bool ctl = false;
    if (dynamic_cast<PathControl*>(path.get()))
        ctl = true;
    else
        if (!dynamic_cast<geometric::PathGeometric*>(path.get()))
            throw Exception("Unknown type of path");

    const geometric::PathGeometric &pg = ctl ?
        static_cast<PathControl*>(path.get())->asGeometric() : *static_cast<geometric::PathGeometric*>(path.get());

    if (!pg.states.empty())
    {
        msg_.debug("Playing through %u states (%0.3f seconds)", (unsigned int)pg.states.size(),
                   timeFactor * si_->getPropagationStepSize() * (double)(pg.states.size() - 1));
        time::duration d = time::seconds(timeFactor * si_->getPropagationStepSize());
        getStateSpace()->as<ODEStateSpace>()->writeState(pg.states[0]);
        for (unsigned int i = 1 ; i < pg.states.size() ; ++i)
        {
            boost::this_thread::sleep(d);
            getStateSpace()->as<ODEStateSpace>()->writeState(pg.states[i]);
        }
    }
}

ompl::base::PathPtr ompl::control::ODESimpleSetup::simulateControl(const double* control, unsigned int steps) const
{
    Control *c = si_->allocControl();
    memcpy(c->as<ODEControlSpace::ControlType>()->values, control, sizeof(double) * getControlSpace()->getDimension());
    base::PathPtr path = simulateControl(c, steps);
    si_->freeControl(c);
    return path;
}

ompl::base::PathPtr ompl::control::ODESimpleSetup::simulateControl(const Control* control, unsigned int steps) const
{
    PathControl *p(new PathControl(si_));

    base::State *s0 = si_->allocState();
    getStateSpace()->as<ODEStateSpace>()->readState(s0);
    p->states.push_back(s0);

    base::State *s1 = si_->allocState();
    si_->propagate(s0, control, steps, s1);
    p->states.push_back(s1);

    p->controls.push_back(si_->cloneControl(control));
    p->controlDurations.push_back(steps);
    return base::PathPtr(p);
}

ompl::base::PathPtr ompl::control::ODESimpleSetup::simulate(unsigned int steps) const
{
    Control *c = si_->allocControl();
    si_->nullControl(c);
    base::PathPtr path = simulateControl(c, steps);
    si_->freeControl(c);
    return path;
}
