add_library(reactorimpl SHARED
        ../Reactor/ICallbackListener.h
        ../Reactor/IShutdownListener.h
        ../Reactor/IReactor.h
        GenericCallbackListener.cpp
        GenericCallbackListener.h
        GenericShutdownListener.cpp
        GenericShutdownListener.h
        ReactorImpl.h
        ReactorImpl.cpp
        ReadableFd.h
        ReadableFd.cpp)

target_include_directories(reactorimpl PUBLIC ../Reactor ../../Common ../../Common/ZeroMQWrapper )
target_link_libraries(reactorimpl PUBLIC zeromqwrapperimpl threads)