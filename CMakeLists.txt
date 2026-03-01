cmake_minimum_required(VERSION 3.16)
project(webserv LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Debug-Modus Option
option(DEBUG_MODE "Enable debug output" OFF)
if(DEBUG_MODE)
    add_definitions(-DDEBUG)
endif()

# Quelldateien
set(SOURCES
    src/main.cpp
    # Config Parser
    src/configParser/configParser.cpp
    src/configParser/grammarValidation.cpp
    src/configParser/parseGlobal.cpp
    src/configParser/parseServer.cpp
    src/configParser/parseLocation.cpp
    src/configParser/print.cpp
    # HTTP
    src/httpRequest/httpRequest.cpp
    src/httpResponse/httpResponse.cpp
    src/httpResponse/requesthandler.cpp
    # Server & Networking
    src/server/server.cpp
    src/poller/poller.cpp
    src/router/router.cpp
    # CGI
    src/cgi/cgiManager.cpp
    # Utils
    src/utils/utils.cpp
    # Bot Detection
    src/botDetection/botDetection.cpp
    src/captchaBypass/captchaBypass.cpp
)

# Executable erstellen
add_executable(${PROJECT_NAME} ${SOURCES})

# Include-Verzeichnis
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Compiler-Warnungen
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Werror)
endif()

# Windows-spezifische Einstellungen
if(WIN32)
    add_definitions(-D_WIN32_WINNT=0x0601)
    target_link_libraries(${PROJECT_NAME} PRIVATE ws2_32)
    
    # Hinweis: Einige POSIX-Funktionen (fork, pipe, etc.) 
    # sind unter Windows nicht verfügbar und erfordern Anpassungen
    message(WARNING "Windows-Build: CGI und einige POSIX-Features funktionieren möglicherweise nicht vollständig")
endif()

# Installation (optional)
install(TARGETS ${PROJECT_NAME} DESTINATION bin)
