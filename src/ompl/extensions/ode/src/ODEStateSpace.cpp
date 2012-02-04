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

#include "ompl/extensions/ode/ODEStateSpace.h"
#include "ompl/util/Console.h"
#include <boost/lexical_cast.hpp>
#include <limits>
#include <queue>

ompl::control::ODEStateSpace::ODEStateSpace(const ODEEnvironmentPtr &env,
                                                  double positionWeight, double linVelWeight, double angVelWeight, double orientationWeight) :
    base::CompoundStateSpace(), env_(env)
{
    setName("ODE" + getName());
    for (unsigned int i = 0 ; i < env_->stateBodies_.size() ; ++i)
    {
        std::string body = ":B" + boost::lexical_cast<std::string>(i);

        addSubSpace(base::StateSpacePtr(new base::RealVectorStateSpace(3)), positionWeight); // position
        components_.back()->setName(components_.back()->getName() + body + ":position");

        addSubSpace(base::StateSpacePtr(new base::RealVectorStateSpace(3)), linVelWeight);   // linear velocity
        components_.back()->setName(components_.back()->getName() + body + ":linvel");

        addSubSpace(base::StateSpacePtr(new base::RealVectorStateSpace(3)), angVelWeight);   // angular velocity
        components_.back()->setName(components_.back()->getName() + body + ":angvel");

        addSubSpace(base::StateSpacePtr(new base::SO3StateSpace()), orientationWeight);      // orientation
        components_.back()->setName(components_.back()->getName() + body + ":orientation");
    }
    lock();
    setDefaultBounds();
}

void ompl::control::ODEStateSpace::setDefaultBounds(void)
{
    // limit all velocities to 1 m/s, 1 rad/s, respectively
    base::RealVectorBounds bounds1(3);
    bounds1.setLow(-1);
    bounds1.setHigh(1);
    setLinearVelocityBounds(bounds1);
    setAngularVelocityBounds(bounds1);

    // find the bounding box that contains all geoms included in the collision spaces
    double mX, mY, mZ, MX, MY, MZ;
    mX = mY = mZ = std::numeric_limits<double>::infinity();
    MX = MY = MZ = -std::numeric_limits<double>::infinity();
    bool found = false;

    std::queue<dSpaceID> spaces;
    for (unsigned int i = 0 ; i < env_->collisionSpaces_.size() ; ++i)
        spaces.push(env_->collisionSpaces_[i]);

    while (!spaces.empty())
    {
        dSpaceID space = spaces.front();
        spaces.pop();

        int n = dSpaceGetNumGeoms(space);

        for (int j = 0 ; j < n ; ++j)
        {
            dGeomID geom = dSpaceGetGeom(space, j);
            if (dGeomIsSpace(geom))
                spaces.push((dSpaceID)geom);
            else
            {
                bool valid = true;
                dReal aabb[6];
                dGeomGetAABB(geom, aabb);

                // things like planes are infinite; we want to ignore those
                for (int k = 0 ; k < 6 ; ++k)
                    if (fabs(aabb[k]) >= std::numeric_limits<dReal>::max())
                    {
                        valid = false;
                        break;
                    }
                if (valid)
                {
                    found = true;
                    if (aabb[0] < mX) mX = aabb[0];
                    if (aabb[1] > MX) MX = aabb[1];
                    if (aabb[2] < mY) mY = aabb[2];
                    if (aabb[3] > MY) MY = aabb[3];
                    if (aabb[4] < mZ) mZ = aabb[4];
                    if (aabb[5] > MZ) MZ = aabb[5];
                }
            }
        }
    }

    if (found)
    {
        double dx = MX - mX;
        double dy = MY - mY;
        double dz = MZ - mZ;
        double dM = std::max(dx, std::max(dy, dz));

        // add 10% in each dimension + 1% of the max dimension
        dx = dx / 10.0 + dM / 100.0;
        dy = dy / 10.0 + dM / 100.0;
        dz = dz / 10.0 + dM / 100.0;

        bounds1.low[0] = mX - dx;
        bounds1.high[0] = MX + dx;
        bounds1.low[1] = mY - dy;
        bounds1.high[1] = MY + dy;
        bounds1.low[2] = mZ - dz;
        bounds1.high[2] = MZ + dz;

        setVolumeBounds(bounds1);
    }
}

void ompl::control::ODEStateSpace::copyState(base::State *destination, const base::State *source) const
{
    CompoundStateSpace::copyState(destination, source);
    destination->as<StateType>()->collision = source->as<StateType>()->collision;
}

namespace ompl
{
    /// @cond IGNORE
    struct CallbackParam
    {
        const control::ODEEnvironment *env;
        bool                           collision;
    };

    static void nearCallback(void *data, dGeomID o1, dGeomID o2)
    {
        // if a collision has not already been detected
        if (reinterpret_cast<CallbackParam*>(data)->collision == false)
        {
            dBodyID b1 = dGeomGetBody(o1);
            dBodyID b2 = dGeomGetBody(o2);
            if (b1 && b2 && dAreConnectedExcluding(b1, b2, dJointTypeContact)) return;

            dContact contact[1];  // one contact is sufficient
            int numc = dCollide(o1, o2, 1, &contact[0].geom, sizeof(dContact));

            // check if there is really a collision
            if (numc)
            {
                // check if the collision is allowed
                bool valid = reinterpret_cast<CallbackParam*>(data)->env->isValidCollision(o1, o2, contact[0]);
                reinterpret_cast<CallbackParam*>(data)->collision = !valid;
                if (reinterpret_cast<CallbackParam*>(data)->env->verboseContacts_)
                {
                    static msg::Interface msg;
                    msg.debug((valid ? "Valid" : "Invalid") + std::string(" contact between ") +
                              reinterpret_cast<CallbackParam*>(data)->env->getGeomName(o1) + " and " +
                              reinterpret_cast<CallbackParam*>(data)->env->getGeomName(o2));
                }
            }
        }
    }
    /// @endcond
}

bool ompl::control::ODEStateSpace::evaluateCollision(const base::State *state) const
{
    if (state->as<StateType>()->collision & (1 << STATE_COLLISION_KNOWN_BIT))
        return state->as<StateType>()->collision & (1 << STATE_COLLISION_VALUE_BIT);
    env_->mutex_.lock();
    writeState(state);
    CallbackParam cp = { env_.get(), false };
    for (unsigned int i = 0 ; cp.collision == false && i < env_->collisionSpaces_.size() ; ++i)
        dSpaceCollide(env_->collisionSpaces_[i], &cp, &nearCallback);
    env_->mutex_.unlock();
    if (cp.collision)
        state->as<StateType>()->collision &= (1 << STATE_COLLISION_VALUE_BIT);
    state->as<StateType>()->collision &= (1 << STATE_COLLISION_KNOWN_BIT);
    return cp.collision;
}

bool ompl::control::ODEStateSpace::satisfiesBoundsExceptRotation(const StateType *state) const
{
    for (unsigned int i = 0 ; i < componentCount_ ; ++i)
        if (i % 4 != 3)
            if (!components_[i]->satisfiesBounds(state->components[i]))
                return false;
    return true;
}

void ompl::control::ODEStateSpace::setVolumeBounds(const base::RealVectorBounds &bounds)
{
    for (unsigned int i = 0 ; i < env_->stateBodies_.size() ; ++i)
        components_[i * 4]->as<base::RealVectorStateSpace>()->setBounds(bounds);
}

void ompl::control::ODEStateSpace::setLinearVelocityBounds(const base::RealVectorBounds &bounds)
{
    for (unsigned int i = 0 ; i < env_->stateBodies_.size() ; ++i)
        components_[i * 4 + 1]->as<base::RealVectorStateSpace>()->setBounds(bounds);
}

void ompl::control::ODEStateSpace::setAngularVelocityBounds(const base::RealVectorBounds &bounds)
{
    for (unsigned int i = 0 ; i < env_->stateBodies_.size() ; ++i)
        components_[i * 4 + 2]->as<base::RealVectorStateSpace>()->setBounds(bounds);
}

ompl::base::State* ompl::control::ODEStateSpace::allocState(void) const
{
    StateType *state = new StateType();
    allocStateComponents(state);
    return state;
}

void ompl::control::ODEStateSpace::freeState(base::State *state) const
{
    CompoundStateSpace::freeState(state);
}

void ompl::control::ODEStateSpace::readState(base::State *state) const
{
    StateType *s = state->as<StateType>();
    for (int i = (int)env_->stateBodies_.size() - 1 ; i >= 0 ; --i)
    {
        unsigned int _i4 = i * 4;

        const dReal *pos = dBodyGetPosition(env_->stateBodies_[i]);
        const dReal *vel = dBodyGetLinearVel(env_->stateBodies_[i]);
        const dReal *ang = dBodyGetAngularVel(env_->stateBodies_[i]);
        double *s_pos = s->as<base::RealVectorStateSpace::StateType>(_i4)->values; ++_i4;
        double *s_vel = s->as<base::RealVectorStateSpace::StateType>(_i4)->values; ++_i4;
        double *s_ang = s->as<base::RealVectorStateSpace::StateType>(_i4)->values; ++_i4;

        for (int j = 0; j < 3; ++j)
        {
            s_pos[j] = pos[j];
            s_vel[j] = vel[j];
            s_ang[j] = ang[j];
        }

        const dReal *rot = dBodyGetQuaternion(env_->stateBodies_[i]);
            base::SO3StateSpace::StateType &s_rot = *s->as<base::SO3StateSpace::StateType>(_i4);

        s_rot.w = rot[0];
        s_rot.x = rot[1];
        s_rot.y = rot[2];
        s_rot.z = rot[3];
    }
}

void ompl::control::ODEStateSpace::writeState(const base::State *state) const
{
    const StateType *s = state->as<StateType>();
    for (int i = (int)env_->stateBodies_.size() - 1 ; i >= 0 ; --i)
    {
        unsigned int _i4 = i * 4;

        double *s_pos = s->as<base::RealVectorStateSpace::StateType>(_i4)->values; ++_i4;
        dBodySetPosition(env_->stateBodies_[i], s_pos[0], s_pos[1], s_pos[2]);

        double *s_vel = s->as<base::RealVectorStateSpace::StateType>(_i4)->values; ++_i4;
        dBodySetLinearVel(env_->stateBodies_[i], s_vel[0], s_vel[1], s_vel[2]);

        double *s_ang = s->as<base::RealVectorStateSpace::StateType>(_i4)->values; ++_i4;
        dBodySetAngularVel(env_->stateBodies_[i],  s_ang[0], s_ang[1], s_ang[2]);

            const base::SO3StateSpace::StateType &s_rot = *s->as<base::SO3StateSpace::StateType>(_i4);
        dQuaternion q;
        q[0] = s_rot.w;
        q[1] = s_rot.x;
        q[2] = s_rot.y;
        q[3] = s_rot.z;
        dBodySetQuaternion(env_->stateBodies_[i], q);
    }
}