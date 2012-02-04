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

#ifndef OMPL_EXTENSION_ODE_CONTROL_SPACE_
#define OMPL_EXTENSION_ODE_CONTROL_SPACE_

#include "ompl/control/spaces/RealVectorControlSpace.h"
#include "ompl/extensions/ode/ODEStateSpace.h"

namespace ompl
{

    namespace control
    {

        /** \brief Representation of controls applied in ODE
            environments. This is an array of double values. */
        class ODEControlSpace : public RealVectorControlSpace
        {
        public:

            /** \brief Construct a representation of controls passed
                to ODE. If \e stateSpace does not cast to an
                ODEStateSpace, an exception is thrown. */
            ODEControlSpace(const base::StateSpacePtr &stateSpace);

            virtual ~ODEControlSpace(void)
            {
            }

            /** \brief Get the ODE environment this state space corresponds to */
            const ODEEnvironmentPtr& getEnvironment(void) const
            {
                return stateSpace_->as<ODEStateSpace>()->getEnvironment();
            }

        };
    }

}


#endif
