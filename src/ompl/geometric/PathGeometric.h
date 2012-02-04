/*********************************************************************
* Software License Agreement (BSD License)
*
*  Copyright (c) 2008, Willow Garage, Inc.
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
*   * Neither the name of the Willow Garage nor the names of its
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

#ifndef OMPL_GEOMETRIC_PATH_GEOMETRIC_
#define OMPL_GEOMETRIC_PATH_GEOMETRIC_

#include "ompl/base/SpaceInformation.h"
#include "ompl/base/Path.h"
#include <vector>
#include <utility>

namespace ompl
{

    /** \brief This namespace contains code that is specific to planning under geometric constraints */
    namespace geometric
    {

        /** \brief Definition of a geometric path.

            This is the type of path computed by geometric planners. */
        class PathGeometric : public base::Path
        {
        public:

            /** \brief Construct a path instance for a given space information */
            PathGeometric(const base::SpaceInformationPtr &si) : base::Path(si)
            {
            }

            /** \brief Copy constructor */
            PathGeometric(const PathGeometric &path);

            /** \brief Construct a path instance from a single state */
            PathGeometric(const base::SpaceInformationPtr &si, const base::State *state);

            virtual ~PathGeometric(void)
            {
                freeMemory();
            }

            /** \brief Assignment operator */
            PathGeometric& operator=(const PathGeometric& other);

            /** \brief Compute the length of a geometric path (sum of lengths of segments that make up the path) */
            virtual double length(void) const;

            /** \brief Check if the path is valid */
            virtual bool check(void) const;

            /** \brief Compute a notion of smootheness for this
                path. The closer the value is to 0, the smoother the
                path. */
            double smoothness(void) const;

            /** \brief Compute the clearance of the end points along
                the path (no interpolation is performed). The clearance for the points is averaged. */
            double clearance(void) const;

            /** \brief Check if the path is valid. If it is not,
                attempts are made to fix the path by sampling around
                invalid states. Not more than \e attempts samples are
                drawn. A pair of boolean values is returned. The first
                value represents the validity of the path before any
                change was made. The second value represents the
                validity of the path after changes were attempted. If
                no changes are attempted, the both values are true.

                \note If repairing a path fails, the path may still be altered */
            std::pair<bool, bool> checkAndRepair(unsigned int attempts);

            /** \brief Print the path to a stream */
            virtual void print(std::ostream &out) const;

            /** \brief Insert a number of states in a path so that the
                path is made up of exactly \e count states. States are
                inserted uniformly (more states on longer
                segments). Changes are performed only if a path has
                less than \e count states. */
            void interpolate(unsigned int count);

            /** \brief Insert a number of states in a path so that the
                path is made up of (approximately) the states checked
                for validity when a discrete motion validator is
                used. */
            void interpolate(void);

            /** \brief Add a state at the middle of each segment */
            void subdivide(void);

            /** \brief Reverse the path */
            void reverse(void);

            /** \brief Overlay the path \e over on top of the current
                path. States are added to the current path if needed
                (by copying the last state).

                If \e over consists of states form a different
                state space than the existing path, the data from those
                states is copied over, for the corresponding
                components. If \e over is from the same state space as this path,
                and \e startIndex is 0, this function's result will be the same
                as with operator=() */
            void overlay(const PathGeometric &over, unsigned int startIndex = 0);

            /** \brief Append \e path at the end of this path.

                Let the existing path consist of states [ \e s1, \e
                s2, ..., \e sk ]. Let \e path consist of states [\e y1, ..., \e yp].

                If the existing path and \e path consist of states
                from the same state space, [\e y1, ..., \e yp] are added after \e sk.
                If they are not from the same state space, states [\e z1, ..., \e zp]
                are added, where each \e zi is a copy of \e sk that
                has components overwritten with ones in \e yi (if there are any common subspaces).
            */
            void append(const PathGeometric &path);

            /** \brief Set this path to a random segment */
            void random(void);

            /** \brief Set this path to a random valid segment. Sample \e attempts times for valid segments. Returns true on success.*/
            bool randomValid(unsigned int attempts);

            /** \brief The list of states that make up the path */
            std::vector<base::State*> states;

        protected:

            /** \brief Free the memory corresponding to the states on this path */
            void freeMemory(void);

            /** \brief Copy data to this path from another path instance */
            void copyFrom(const PathGeometric& other);
        };

    }
}

#endif
