sophos_add_library(heartbeat SHARED
	Heartbeat.cpp
	Heartbeat.h
	HeartbeatPinger.cpp
	HeartbeatPinger.h
	IHeartbeat.h
	MockHeartbeat.h
	ThreadIdConsts.h
	EXTRA_INCLUDES ${pluginapiinclude} ${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/modules
)