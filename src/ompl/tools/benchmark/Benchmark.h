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

#ifndef OMPL_TOOLS_BENCHMARK_BENCHMARK_
#define OMPL_TOOLS_BENCHMARK_BENCHMARK_

#include "ompl/geometric/SimpleSetup.h"
#include "ompl/control/SimpleSetup.h"

namespace ompl
{

    /** \brief Benchmark a set of planners on a problem instance */
    class Benchmark
    {
    public:

        /** \brief This structure contains information about the
            activity of a benchmark instance.  If the instance is
            running, it is possible to find out information such
            as which planner is currently being tested or how much */
        struct Status
        {
            Status(void)
            {
                running = false;
                activeRun = 0;
                progressPercentage = 0.0;
            }

            /// Flag indicating whether benchmarking is running
            bool         running;

            /// The name of the planner currently being tested
            std::string  activePlanner;

            /// The number of the run currently being executed
            unsigned int activeRun;

            /// Total progress (0 to 100)
            double       progressPercentage;
        };

        /** \brief The data collected from a run of a planner is
            stored as key-value pairs. */
        typedef std::map<std::string, std::string> RunProperties;

        /** \brief Signature of function that can be called before a planner execution is started */
        typedef boost::function1<void, const base::PlannerPtr&> PreSetupEvent;

        /** \brief Signature of function that can be called after a planner execution is completed */
        typedef boost::function2<void, const base::PlannerPtr&, RunProperties&> PostSetupEvent;

        /** \brief The data collected after running a planner multiple times */
        struct PlannerExperiment
        {
            /// The name of the planner
            std::string                name;

            /// Data collected for each run
            std::vector<RunProperties> runs;

            /// Some common properties for all the runs
            RunProperties              common;
        };

        /** \brief This structure holds experimental data for a set of planners */
        struct CompleteExperiment
        {
            /** \brief The name of the experiment */
            std::string                    name;

            /// The collected experimental data; each element of the array (an experiment) corresponds to a planner
            std::vector<PlannerExperiment> planners;

            /// The maximum allowed time for planner computation during the experiment (seconds)
            double                         maxTime;

            /// The maximum allowed memory for planner computation during the experiment (MB)
            double                         maxMem;

            /// The point in time when the experiment was started
            time::point                    startTime;

            /// The amount of time spent to collect the information in this structure (seconds)
            double                         totalDuration;

            /// The output of SimpleSetup::print() before the experiment was started
            std::string                    setupInfo;

            /// The random seed that was used at the start of the benchmark program
            boost::uint32_t                seed;

            /// Hostname that identifies the machine the benchmark ran on
            std::string                    host;
        };

        /** \brief Constructor needs the SimpleSetup instance needed for planning. Optionally, the experiment name (\e name) can be specified */
        Benchmark(geometric::SimpleSetup &setup, const std::string &name = std::string()) : gsetup_(&setup), csetup_(NULL), msg_("Benchmark")
        {
            exp_.name = name;
        }

        /** \brief Constructor needs the SimpleSetup instance needed for planning. Optionally, the experiment name (\e name) can be specified */
        Benchmark(control::SimpleSetup &setup, const std::string &name = std::string()) : gsetup_(NULL), csetup_(&setup), msg_("Benchmark")
        {
            exp_.name = name;
        }

        virtual ~Benchmark(void)
        {
        }

        /** \brief Set the name of the experiment */
        void setExperimentName(const std::string &name)
        {
            exp_.name = name;
        }

        /** \brief Get the name of the experiment */
        const std::string& getExperimentName(void) const
        {
            return exp_.name;
        }

        /** \brief Set the planner to use. If the planner is not
            set, an attempt is made to use the planner
            allocator. If no planner allocator is available
            either, a default planner is set. */
        void addPlanner(const base::PlannerPtr &planner)
        {
            if (planner && planner->getSpaceInformation().get() !=
                (gsetup_ ? gsetup_->getSpaceInformation().get() : csetup_->getSpaceInformation().get()))
                throw Exception("Planner instance does not match space information");
            planners_.push_back(planner);
        }

        /** \brief Set the planner allocator to use. This is only
            used if no planner has been set. This is optional -- a default
            planner will be used if no planner is otherwise specified. */
        void addPlannerAllocator(const base::PlannerAllocator &pa)
        {
            planners_.push_back(pa(gsetup_ ? gsetup_->getSpaceInformation() : csetup_->getSpaceInformation()));
        }

        /** \brief Clear the set of planners to be benchmarked */
        void clearPlanners(void)
        {
            planners_.clear();
        }

        /// Set the event to be called before the run of a planner
        void setPreRunEvent(const PreSetupEvent &event)
        {
            preRun_ = event;
        }

        /// Set the event to be called after the run of a planner
        void setPostRunEvent(const PostSetupEvent &event)
        {
            postRun_ = event;
        }

        /** \brief Benchmark the added planners on the defined problem. Repeated calls clear previously gathered data.
            \param maxTime the maximum amount of time a planner is allowed to run (seconds)
            \param maxMem the maximum amount of memory a planner is allowed to use (MB)
            \param runCount the number of times to run each planner
            \param displayProgress flag indicating whether progress is to be displayed or not

            \note The values returned for memory consumption may
            be misleading. Memory allocators often free memory in
            a lazy fashion, so the returned values for memory
            consumption indicate the increase in memory usage for
            each run. Since not all the memory for the previous
            run was freed, the increase in usage may be close to
            0. To get correct averages for memory usage, use \e
            runCount = 1 and run the process multiple times.
        */
        virtual void benchmark(double maxTime, double maxMem, unsigned int runCount, bool displayProgress = false);

        /** \brief Get the status of the benchmarking code. This function can be called in a separate thread to check how much progress has been made */
        const Status& getStatus(void) const
        {
            return status_;
        }

        /** \brief Return all the experiment data that would be
            written to the results file. The data should not be
            changed, but it could be useful to quickly extract cartain
            statistics. */
        const CompleteExperiment& getRecordedExperimentData(void) const
        {
            return exp_;
        }

        /** \brief Save the results of the benchmark to a stream. */
        virtual bool saveResultsToStream(std::ostream &out = std::cout) const;

        /** \brief Save the results of the benchmark to a file. */
        bool saveResultsToFile(const char *filename) const;

        /** \brief Save the results of the benchmark to a file. The name of the file is the current date and time. */
        bool saveResultsToFile(void) const;

    protected:

        /** \brief The instance of the problem to benchmark (if geometric planning) */
        geometric::SimpleSetup       *gsetup_;

        /** \brief The instance of the problem to benchmark (if planning with controls) */
        control::SimpleSetup         *csetup_;

        /// The set of planners to be tested
        std::vector<base::PlannerPtr> planners_;

        /// The collected experimental data (for all planners)
        CompleteExperiment            exp_;

        /// The current status of this benchmarking instance
        Status                        status_;

        /// Event to be called before the run of a planner
        PreSetupEvent                 preRun_;

        /// Event to be called after the run of a planner
        PostSetupEvent                postRun_;

        /// Interface for console output
        msg::Interface                msg_;

    };

}
#endif
