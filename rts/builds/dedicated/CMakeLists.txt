add_definitions    (-DDEDICATED ${PIC_FLAG} -DNOT_USING_CREG)
add_definitions    (-DHEADLESS -DNO_SOUND)
remove_definitions (-DSTREFLOP_SSE -DAVI_CAPTURING)
remove_definitions (-DTRACE_SYNC -DSYNCDEBUG)
remove_definitions (-DTHREADPOOL)

# deactivate signaling-NANs for this library
remove_flag("-fsignaling-nans")

# Place executables and shared libs under "build-dir/",
# instead of under "build-dir/rts/"
# This way, we have the build-dir structure more like the install-dir one,
# which makes testing spring in the builddir easier, eg. like this:
# cd build-dir
# SPRING_DATADIR=$(pwd) ./spring
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")

set(ENGINE_SRC_ROOT_DIR "${CMAKE_SOURCE_DIR}/rts")


### Assemble libraries
list(APPEND engineDedicatedLibraries ${Boost_REGEX_LIBRARY})
list(APPEND engineDedicatedLibraries lua archives_nothreadpool 7zip prd::jsoncpp
	${SPRING_MINIZIP_LIBRARY} ZLIB::ZLIB gflags_nothreads_static Tracy::TracyClient)
list(APPEND engineDedicatedLibraries headlessStubs engineSystemNet)
if(UNIX AND NOT (CMAKE_SYSTEM_NAME MATCHES "OpenBSD"))
	list(APPEND engineDedicatedLibraries libunwind::libunwind)
endif()
list(APPEND engineDedicatedLibraries ${LZMA_LIBRARY})

### base64 from pr-downloader
list(APPEND engineDedicatedLibraries "prd::base64")

### smmalloc
list(APPEND engineDedicatedLibraries "smmalloc")

if (WIN32)
	list(APPEND engineDedicatedLibraries ${WINMM_LIBRARY})
endif (WIN32)

if    (UNIX AND NOT MINGW)
	if    (PREFER_STATIC_LIBS)
		list(APPEND engineDedicatedLibraries ${C_LIBRARY} ${MATH_LIBRARY} ${REALTIME_LIBRARY})
	endif (PREFER_STATIC_LIBS)

	# Needed for dynamically loading shared libraries (on some OS)
	list(APPEND engineDedicatedLibraries ${CMAKE_DL_LIBS})

	# Needed for backtrace* on some systems
	if(CMAKE_SYSTEM_NAME MATCHES "FreeBSD" OR CMAKE_SYSTEM_NAME MATCHES "OpenBSD")
		list(APPEND engineDedicatedLibraries execinfo)
	endif(CMAKE_SYSTEM_NAME MATCHES "FreeBSD" OR CMAKE_SYSTEM_NAME MATCHES "OpenBSD")
endif (UNIX AND NOT MINGW)

### Assemble the include dirs
include_directories(${ENGINE_SRC_ROOT_DIR}/)
include_directories(${ENGINE_SRC_ROOT_DIR}/lib/lua/include)
include_directories(${ENGINE_SRC_ROOT_DIR}/lib/7zip)
include_directories(${ENGINE_SRC_ROOT_DIR}/lib/asio/include)



### Assemble sources
if	(APPLE)
	set(sources_engine_Platform_CrashHandler
		${ENGINE_SRC_ROOT_DIR}/System/Platform/CpuID.cpp
		${ENGINE_SRC_ROOT_DIR}/System/Platform/Threading.cpp
		${ENGINE_SRC_ROOT_DIR}/System/Platform/Mac/CrashHandler.cpp)
elseif	(WIN32)
	set(sources_engine_Platform_CrashHandler
		${ENGINE_SRC_ROOT_DIR}/System/Platform/Win/seh.cpp
		${ENGINE_SRC_ROOT_DIR}/System/Platform/Win/CrashHandler.cpp)
else	()
	set(sources_engine_Platform_CrashHandler
		${ENGINE_SRC_ROOT_DIR}/System/Platform/CpuID.cpp
		${ENGINE_SRC_ROOT_DIR}/System/Platform/Threading.cpp
		${ENGINE_SRC_ROOT_DIR}/System/Platform/Linux/CrashHandler.cpp
		${ENGINE_SRC_ROOT_DIR}/System/Platform/Linux/ThreadSupport.cpp)
endif	()

set(system_files
	${sources_engine_System_FileSystem}
	${sources_engine_System_Threading}
	${sources_engine_System_Log}
	${sources_engine_Platform_CrashHandler}
	${ENGINE_SRC_ROOT_DIR}/Net/AutohostInterface.cpp
	${ENGINE_SRC_ROOT_DIR}/System/Config/ConfigHandler.cpp
	${ENGINE_SRC_ROOT_DIR}/System/Config/ConfigLocater.cpp
	${ENGINE_SRC_ROOT_DIR}/System/Config/ConfigSource.cpp
	${ENGINE_SRC_ROOT_DIR}/System/Config/ConfigVariable.cpp
	${ENGINE_SRC_ROOT_DIR}/System/LoadSave/Demo.cpp
	${ENGINE_SRC_ROOT_DIR}/System/LoadSave/DemoReader.cpp
	${ENGINE_SRC_ROOT_DIR}/System/LoadSave/DemoRecorder.cpp
	${ENGINE_SRC_ROOT_DIR}/System/Log/Backend.cpp
	${ENGINE_SRC_ROOT_DIR}/System/Log/DefaultFilter.cpp
	${ENGINE_SRC_ROOT_DIR}/System/Log/DefaultFormatter.cpp
	${ENGINE_SRC_ROOT_DIR}/System/Log/FramePrefixer.cpp
	${ENGINE_SRC_ROOT_DIR}/System/Log/LogSinkHandler.cpp
	${ENGINE_SRC_ROOT_DIR}/System/Log/LogUtil.c
	${ENGINE_SRC_ROOT_DIR}/System/Log/ConsoleSink.cpp
	${ENGINE_SRC_ROOT_DIR}/System/Log/TracySink.cpp
	${ENGINE_SRC_ROOT_DIR}/System/Misc/SpringTime.cpp
	${ENGINE_SRC_ROOT_DIR}/System/Platform/errorhandler.cpp
	${ENGINE_SRC_ROOT_DIR}/System/Platform/CpuID.cpp
	${ENGINE_SRC_ROOT_DIR}/System/Platform/Misc.cpp
	${ENGINE_SRC_ROOT_DIR}/System/Platform/ScopedFileLock.cpp
	${ENGINE_SRC_ROOT_DIR}/System/Platform/Threading.cpp
	${ENGINE_SRC_ROOT_DIR}/System/Sync/SHA512.cpp
	${ENGINE_SRC_ROOT_DIR}/System/CRC.cpp
	${ENGINE_SRC_ROOT_DIR}/System/TdfParser.cpp
	${ENGINE_SRC_ROOT_DIR}/System/GlobalConfig.cpp
	${ENGINE_SRC_ROOT_DIR}/System/Info.cpp
	${ENGINE_SRC_ROOT_DIR}/System/LogOutput.cpp
	${ENGINE_SRC_ROOT_DIR}/System/TimeUtil.cpp
	${ENGINE_SRC_ROOT_DIR}/System/SafeCStrings.c
	${ENGINE_SRC_ROOT_DIR}/System/SafeVector.cpp
	${ENGINE_SRC_ROOT_DIR}/System/SpringMath.cpp
	${ENGINE_SRC_ROOT_DIR}/System/Sync/FPUCheck.cpp
	${ENGINE_SRC_ROOT_DIR}/System/UriParser.cpp
	${ENGINE_SRC_ROOT_DIR}/System/StringUtil.cpp
	${ENGINE_SRC_ROOT_DIR}/System/float3.cpp
	${ENGINE_SRC_ROOT_DIR}/System/float4.cpp
	)
if    (WIN32)
	list(APPEND system_files ${ENGINE_SRC_ROOT_DIR}/System/Platform/Win/Hardware.cpp)
	list(APPEND system_files ${ENGINE_SRC_ROOT_DIR}/System/Platform/Win/WinVersion.cpp)
	list(APPEND system_files ${ENGINE_SRC_ROOT_DIR}/System/Platform/SharedLib.cpp)
	list(APPEND system_files ${ENGINE_SRC_ROOT_DIR}/System/Platform/Win/DllLib.cpp)
else  ()
	list(APPEND system_files ${ENGINE_SRC_ROOT_DIR}/System/Platform/Linux/Hardware.cpp)
endif ()

set(engineDedicatedSources
	${system_files}
	${sources_engine_NetServer}
	${sources_engine_System_Log}
	${ENGINE_SRC_ROOT_DIR}/Game/ClientSetup.cpp
	${ENGINE_SRC_ROOT_DIR}/Game/GameSetup.cpp
	${ENGINE_SRC_ROOT_DIR}/Game/GameData.cpp
	${ENGINE_SRC_ROOT_DIR}/Game/Players/PlayerBase.cpp
	${ENGINE_SRC_ROOT_DIR}/Game/Players/PlayerStatistics.cpp
	${ENGINE_SRC_ROOT_DIR}/Game/GameVersion.cpp
	${ENGINE_SRC_ROOT_DIR}/Game/IVideoCapturing.cpp
	${ENGINE_SRC_ROOT_DIR}/Game/DummyVideoCapturing.cpp
	${ENGINE_SRC_ROOT_DIR}/Game/CommandMessage.cpp
	${ENGINE_SRC_ROOT_DIR}/Game/ChatMessage.cpp
	${ENGINE_SRC_ROOT_DIR}/Game/Action.cpp
	${ENGINE_SRC_ROOT_DIR}/Sim/Misc/TeamBase.cpp
	${ENGINE_SRC_ROOT_DIR}/Sim/Misc/TeamStatistics.cpp
	${ENGINE_SRC_ROOT_DIR}/Sim/Misc/AllyTeam.cpp
	${ENGINE_SRC_ROOT_DIR}/Sim/Units/CommandAI/Command.cpp ## LuaUtils::ParseCommand*
	${ENGINE_SRC_ROOT_DIR}/Lua/LuaConstEngine.cpp
	${ENGINE_SRC_ROOT_DIR}/Lua/LuaEncoding.cpp
	${ENGINE_SRC_ROOT_DIR}/Lua/LuaIO.cpp
	${ENGINE_SRC_ROOT_DIR}/Lua/LuaMathExtra.cpp
	${ENGINE_SRC_ROOT_DIR}/Lua/LuaMemPool.cpp
	${ENGINE_SRC_ROOT_DIR}/Lua/LuaParser.cpp
	${ENGINE_SRC_ROOT_DIR}/Lua/LuaUtils.cpp
	${ENGINE_SRC_ROOT_DIR}/Map/MapParser.cpp
	)


# Compile dedicated server executable
add_executable(engine-dedicated
		${ENGINE_SRC_ROOT_DIR}/Map/MapParser.cpp
		${engineDedicatedSources}
		${sources_engine_System_Log_sinkConsole}
		${sources_engine_System_Log_sinkFile}
		${sources_engine_System_Log_sinkOutputDebugString}
		${ENGINE_ICON}
		main.cpp
	)
target_link_libraries(engine-dedicated ${engineDedicatedLibraries})
if    (MINGW)
	target_link_libraries(engine-dedicated ${WS2_32_LIBRARY} ${IMAGEHLP_LIBRARY} ${IPHLPAPI_LIBRARY} ${WINMM_LIBRARY})
endif (MINGW)
if    (MSVC)
	target_link_libraries(engine-dedicated ${DBGHELP_LIBRARY} ${IPHLPAPI_LIBRARY})
endif (MSVC)
add_dependencies(engine-dedicated generateVersionFiles)

if    (MINGW)
	# To enable console output/force a console window to open
	set_target_properties(engine-dedicated PROPERTIES LINK_FLAGS "-Wl,-subsystem,console")
endif (MINGW)

install(TARGETS engine-dedicated DESTINATION ${BINDIR})

# Only build & install spring-dedicated executable & dependencies
# use cases:
# * make spring-dedicated
# * make install-spring-dedicated
create_engine_build_and_install_target(dedicated)
