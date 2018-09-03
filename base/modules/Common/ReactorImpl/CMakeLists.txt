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
        ReadableFd.cpp
        Logger.h
        Logger.cpp)

target_include_directories(reactorimplobject PUBLIC ../Reactor ../../Common ../../Common/ZeroMQWrapper ${LOG4CPLUS_INCLUDE_DIR})
add_library(reactorimpl SHARED $<TARGET_OBJECTS:reactorimplobject> )

target_link_libraries(reactorimpl PUBLIC zeromqwrapperimpl logging threads)

SET_TARGET_PROPERTIES( reactorimpl
        PROPERTIES INSTALL_RPATH "$ORIGIN")

install(TARGETS reactorimpl
        LIBRARY DESTINATION files/base/lib64)
