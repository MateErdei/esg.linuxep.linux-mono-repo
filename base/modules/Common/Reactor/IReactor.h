/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#ifndef EVEREST_BASE_IREACTOR_H
#define EVEREST_BASE_IREACTOR_H

#include "ICallbackListener.h"
#include "IReadable.h"
#include "IShutdownListener.h"
namespace Common
{
    namespace Reactor
    {
        /**
         * The reactor allows monitoring input data channels ( file descriptors, socket) and react to data arriving
         * by executing the callbacks.
         * The intention is to use for "Servers" that must reply on requests when they arrive.
         *
         * It uses threads to monitor the activity of the input data channels and run the callback from this background thread.
         * Hence, all the callbacks must be safe to be executed from a different thread.
         *
         */
        class IReactor
        {
        public:
            virtual ~IReactor() = default;

            /**
             * addListener used to add an implemented ICallbackListener listener object to the reactor.
             * Listeners can only be added before start is called.
             *
             * @param readable, pointer to readable (usually related to ZeroMQ socket)
             * @param callback, pointer implemented callbackListener that will be added to reactor.
             *        The call back will be executed when there is data available to be processed.
             *
             * @note It is the client responsibility to ensure readable and callback do not go out of scope during
             *       the lifetime of Reactor.
             *
             * @test Reactor thread start has not been called.
             */
            virtual void addListener(Common::ZeroMQWrapper::IReadable * readable, ICallbackListener * callback) = 0;

            /**
             * Adds a pointer to implemented shutdown listener to the reactor.
             *
             * The shutdownListener will be executed when the current process receives a SIGTERM or SIGINT.
             * After this, the IReactor will stop its own thread.
             *
             * @note: If armShutdownListener is never called, no signal handler will be defined, and the process will
             *        just terminate 'abruptly' when those signals are received. (or will be handled by a different signal handler)
             *
             * @param shutdownListener
             */
            virtual void armShutdownListener(IShutdownListener * shutdownListener) = 0;

            /**
             * Wrapper for the Reactor thread start method, once thread is running no more listeners can be added
             */
            virtual void start() = 0;

            /**
             * Wrapper for the Reactor thread Stop method.
             */
            virtual void stop() = 0;

            /**
             * Wrapper for the Reactor thread join method.
             * Use this method when the main thread wants to allow the reactor to keep running.
             * See ::FakeServerRunner for example of when it is useful.
             */
            virtual void join() = 0;
        };
        using IReactorPtr = std::unique_ptr<IReactor>;
        extern IReactorPtr createReactor();
    }
}

#endif //EVEREST_BASE_IREACTOR_H
