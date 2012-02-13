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

#include "ompl/control/PathControl.h"
#include "ompl/geometric/PathGeometric.h"
#include "ompl/base/samplers/UniformValidStateSampler.h"
#include "ompl/util/Exception.h"
#include <numeric>
#include <cmath>

ompl::control::PathControl::PathControl(const base::SpaceInformationPtr &si) : base::Path(si)
{
    if (!dynamic_cast<const SpaceInformation*>(si_.get()))
        throw Exception("Cannot create a path with controls from a space that does not support controls");
}

ompl::control::PathControl::PathControl(const PathControl &path) : base::Path(path.si_)
{
    copyFrom(path);
}

ompl::geometric::PathGeometric ompl::control::PathControl::asGeometric(void) const
{
    PathControl pc(*this);
    pc.interpolate();
    geometric::PathGeometric pg(si_);
    pg.getStates().swap(pc.states_);
    return pg;
}

ompl::control::PathControl& ompl::control::PathControl::operator=(const PathControl& other)
{
    freeMemory();
    si_ = other.si_;
    copyFrom(other);
    return *this;
}

void ompl::control::PathControl::copyFrom(const PathControl& other)
{
    states_.resize(other.states_.size());
    controls_.resize(other.controls_.size());

    for (unsigned int i = 0 ; i < states_.size() ; ++i)
        states_[i] = si_->cloneState(other.states_[i]);

    const SpaceInformation *si = static_cast<const SpaceInformation*>(si_.get());
    for (unsigned int i = 0 ; i < controls_.size() ; ++i)
        controls_[i] = si->cloneControl(other.controls_[i]);

    controlDurations_ = other.controlDurations_;
}

double ompl::control::PathControl::length(void) const
{
    return std::accumulate(controlDurations_.begin(), controlDurations_.end(), 0.0);
}

void ompl::control::PathControl::print(std::ostream &out) const
{
    const SpaceInformation *si = static_cast<const SpaceInformation*>(si_.get());
    double res = si->getPropagationStepSize();
    out << "Control path with " << states_.size() << " states" << std::endl;
    for (unsigned int i = 0 ; i < controls_.size() ; ++i)
    {
        out << "At state ";
        si_->printState(states_[i], out);
        out << "  apply control ";
        si->printControl(controls_[i], out);
        out << "  for " << (int)floor(0.5 + controlDurations_[i]/res) << " steps" << std::endl;
    }
    out << "Arrive at state ";
    si_->printState(states_[controls_.size()], out);
    out << std::endl;
}

void ompl::control::PathControl::interpolate(void)
{
    const SpaceInformation *si = static_cast<const SpaceInformation*>(si_.get());
    std::vector<base::State*> newStates;
    std::vector<Control*> newControls;
    std::vector<double> newControlDurations;

    double res = si->getPropagationStepSize();
    for (unsigned int  i = 0 ; i < controls_.size() ; ++i)
    {
        int steps = (int)floor(0.5 + controlDurations_[i] / res);
        assert(steps >= 0);
        if (steps <= 1)
        {
            newStates.push_back(states_[i]);
            newControls.push_back(controls_[i]);
            newControlDurations.push_back(controlDurations_[i]);
            continue;
        }
        std::vector<base::State*> istates;
        si->propagate(states_[i], controls_[i], steps, istates, true);
        // last state is already in the non-interpolated path
        if (!istates.empty())
        {
            si_->freeState(istates.back());
            istates.pop_back();
        }
        newStates.push_back(states_[i]);
        newStates.insert(newStates.end(), istates.begin(), istates.end());
        newControls.push_back(controls_[i]);
        newControlDurations.push_back(res);
        for (int j = 1 ; j < steps; ++j)
        {
            newControls.push_back(si->cloneControl(controls_[i]));
            newControlDurations.push_back(res);
        }
    }
    newStates.push_back(states_[controls_.size()]);
    states_.swap(newStates);
    controls_.swap(newControls);
    controlDurations_.swap(newControlDurations);
}

bool ompl::control::PathControl::check(void) const
{
    bool valid = true;
    const SpaceInformation *si = static_cast<const SpaceInformation*>(si_.get());
    double res = si->getPropagationStepSize();
    base::State *dummy = si_->allocState();
    for (unsigned int  i = 0 ; i < controls_.size() ; ++i)
    {
        unsigned int steps = (unsigned int)floor(0.5 + controlDurations_[i] / res);
        if (si->propagateWhileValid(states_[i], controls_[i], steps, dummy) != steps)
        {
            valid = false;
            break;
        }
    }
    si_->freeState(dummy);

    if (valid)
        for (unsigned int j = 0 ; j < states_.size() ; ++j)
            if (!si_->isValid(states_[j]))
                throw Exception("Internal error. This should not ever happen. Please contact the developers.");

    return valid;
}

void ompl::control::PathControl::append(const base::State *state)
{
    states_.push_back(si_->cloneState(state));
}

void ompl::control::PathControl::append(const base::State *state, const Control *control, double duration)
{
    const SpaceInformation *si = static_cast<const SpaceInformation*>(si_.get());
    states_.push_back(si->cloneState(state));
    controls_.push_back(si->cloneControl(control));
    controlDurations_.push_back(duration);
}

void ompl::control::PathControl::random(void)
{
    freeMemory();
    states_.resize(2);
    controlDurations_.resize(1);
    controls_.resize(1);

    const SpaceInformation *si = static_cast<const SpaceInformation*>(si_.get());
    states_[0] = si->allocState();
    states_[1] = si->allocState();
    controls_[0] = si->allocControl();

    base::StateSamplerPtr ss = si->allocStateSampler();
    ss->sampleUniform(states_[0]);
    ControlSamplerPtr cs = si->allocControlSampler();
    cs->sample(controls_[0], states_[0]);
    unsigned int steps = cs->sampleStepCount(si->getMinControlDuration(), si->getMaxControlDuration());
    controlDurations_[0] = steps * si->getPropagationStepSize();
    si->propagate(states_[0], controls_[0], steps, states_[1]);
}

bool ompl::control::PathControl::randomValid(unsigned int attempts)
{
    freeMemory();
    states_.resize(2);
    controlDurations_.resize(1);
    controls_.resize(1);

    const SpaceInformation *si = static_cast<const SpaceInformation*>(si_.get());
    states_[0] = si->allocState();
    states_[1] = si->allocState();
    controls_[0] = si->allocControl();

    ControlSamplerPtr cs = si->allocControlSampler();
    base::UniformValidStateSampler *uvss = new base::UniformValidStateSampler(si);
    uvss->setNrAttempts(attempts);
    bool ok = false;
    for (unsigned int i = 0 ; i < attempts ; ++i)
        if (uvss->sample(states_[0]))
        {
            cs->sample(controls_[0], states_[0]);
            unsigned int steps = cs->sampleStepCount(si->getMinControlDuration(), si->getMaxControlDuration());
            controlDurations_[0] = steps * si->getPropagationStepSize();
            if (si->propagateWhileValid(states_[0], controls_[0], steps, states_[1]) == steps)
            {
                ok = true;
                break;
            }
        }
    delete uvss;

    if (!ok)
    {
        freeMemory();
        states_.clear();
        controls_.clear();
        controlDurations_.clear();
    }
    return ok;
}

void ompl::control::PathControl::freeMemory(void)
{
    for (unsigned int i = 0 ; i < states_.size() ; ++i)
        si_->freeState(states_[i]);
    const SpaceInformation *si = static_cast<const SpaceInformation*>(si_.get());
    for (unsigned int i = 0 ; i < controls_.size() ; ++i)
        si->freeControl(controls_[i]);
}
