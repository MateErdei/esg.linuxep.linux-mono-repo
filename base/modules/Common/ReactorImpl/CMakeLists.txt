add_library(reactorimplobject OBJECT
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

target_include_directories(reactorimplobject PUBLIC ../Reactor ../../Common ../../Common/ZeroMQWrapper )
add_library(reactorimpl SHARED $<TARGET_OBJECTS:reactorimplobject> )

target_link_libraries(reactorimpl PUBLIC zeromqwrapperimpl threads)


install(TARGETS reactorimpl
        LIBRARY DESTINATION files/base/lib64)
