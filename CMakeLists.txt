cmake_minimum_required(VERSION 3.20)
cmake_policy(SET CMP0002 NEW)

# Qt politikaları
if(POLICY CMP0020)
    cmake_policy(SET CMP0020 NEW)  # Otomatik MOC için gerekli
endif()
if(POLICY CMP0071)
    cmake_policy(SET CMP0071 NEW)  # Qt AUTOMOC davranışı için
endif()

# Proje bilgileri
project(digicomassigment 
    VERSION 1.0.0
    LANGUAGES CXX
)

# Yapılandırma tipi belirtilmemişse, Release olarak ayarla
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif()

# Qt yolu ve modülleri - Multimedia eklendi
set(CMAKE_PREFIX_PATH "C:/Qt/6.7.0/msvc2019_64" CACHE STRING "Qt6 path")
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets PrintSupport Multimedia)

# VkFFT ayarları
set(VKFFT_BACKEND 0 CACHE STRING "VkFFT backend selection")
set(VKFFT_ENABLE_DOUBLE_PRECISION ON CACHE BOOL "Enable double precision in VkFFT library build" FORCE)
set(VULKAN_SDK_PATH "C:/VulkanSDK/1.4.313.0" CACHE PATH "Vulkan SDK path")
set(GLSLANG_INCLUDE_DIR "${VULKAN_SDK_PATH}/Include/glslang/Include" CACHE PATH "Glslang include directory")

# Diğer ayarlar

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zc:__cplusplus")
if(DEFINED ENV{VCPKG_ROOT})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
endif()

# MSVC runtime library ayarları - BURASI EKLENDİ/DEĞİŞTİRİLDİ
if(MSVC)
    add_compile_options(/permissive-)
    
    # Runtime library uyumsuzluğunu düzeltme - glslang.lib ile uyumlu hale getir
    foreach(flag_var
            CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
            CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE)
        if(${flag_var} MATCHES "/MDd")
            string(REGEX REPLACE "/MDd" "/MD" ${flag_var} "${${flag_var}}")
        endif()
    endforeach()
    
    # Iterator debug level uyumsuzluğunu düzeltme
    add_compile_definitions(_ITERATOR_DEBUG_LEVEL=0)
    
    # Link uyarısını bastırmak için
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /NODEFAULTLIB:msvcrtd.lib")
endif()

# Qt için önemli ayarlar
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# MOC çift derleme sorununu çözmek için Unity Build kapatma
set(CMAKE_UNITY_BUILD OFF)

# Derleyici ayarları
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Kütüphaneleri bul
find_package(Vulkan REQUIRED)


# VkFFT dizini
set(VKFFT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/VkFFT" CACHE PATH "VkFFT library directory")

# Dizin tanımlamaları
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(THIRDPARTY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty)
set(UI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/ui)  

# Çıktı klasörleri
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# QCustomPlot - PrintSupport modülü eklendi
add_library(qcustomplot STATIC 
    ${THIRDPARTY_DIR}/qcustomplot/qcustomplot.cpp
)
target_include_directories(qcustomplot
    PUBLIC
        ${THIRDPARTY_DIR}/qcustomplot
)
target_link_libraries(qcustomplot PUBLIC 
    Qt6::Core 
    Qt6::Gui 
    Qt6::Widgets 
    Qt6::PrintSupport
Qt6::Multimedia
)
  
# Kaynak dosyalarını topla
file(GLOB_RECURSE MY_SOURCES CONFIGURE_DEPENDS ${SRC_DIR}/*.cpp)

list(FILTER MY_SOURCES EXCLUDE REGEX "moc_.*\\.cpp$")
list(FILTER MY_SOURCES EXCLUDE REGEX ".*mocs_compilation.*\\.cpp$")
file(GLOB_RECURSE MY_HEADERS CONFIGURE_DEPENDS ${INCLUDE_DIR}/*.h)

# UI dosyalarını topla
file(GLOB_RECURSE UI_FILES CONFIGURE_DEPENDS 
    ${UI_DIR}/*.ui  
    ${SRC_DIR}/*.ui
)

# QRC dosyalarını topla
file(GLOB_RECURSE QRC_FILES CONFIGURE_DEPENDS 
    ${SRC_DIR}/*.qrc
)

message(STATUS "UI files: ${UI_FILES}")
message(STATUS "Source files: ${MY_SOURCES}")
message(STATUS "QRC files: ${QRC_FILES}")

# Ana uygulamayı oluştur
add_executable(${PROJECT_NAME} 
    ${MY_SOURCES}
    ${MY_HEADERS}
    ${UI_FILES}
    ${QRC_FILES}


)

if(EXISTS "${CMAKE_SOURCE_DIR}/resources")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/resources"  
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/resources" 
        COMMENT "Copying application resources to executable directory"
    )
    message(STATUS "Resources directory will be copied to: $<TARGET_FILE_DIR:${PROJECT_NAME}>/resources")
else()
    message(WARNING "Resources directory not found at ${CMAKE_SOURCE_DIR}/resources. Skipping copy.")
endif()


target_include_directories(${PROJECT_NAME}
    PUBLIC 
        ${INCLUDE_DIR}
        ${VKFFT_DIR}
        ${VULKAN_SDK_PATH}/Include
        ${GLSLANG_INCLUDE_DIR}
        ${UI_DIR}
)

# Derleyici tanımları
target_compile_definitions(${PROJECT_NAME} PUBLIC 
    RESOURCES_PATH=\"${CMAKE_SOURCE_DIR}/resources/\"
    VKFFT_BACKEND=0
	
)

# MSVC runtime ve uyarılar - UTF-8 desteği eklendi - BURASI DEĞİŞTİRİLDİ
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Debug>:/W4 /Zi /utf-8>
        $<$<CONFIG:Release>:/O2 /DNDEBUG /utf-8>
    )
else()
    target_compile_options(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Debug>:-g -Wall -Wextra>
        $<$<CONFIG:Release>:-O3 -DNDEBUG>
    )
endif()

# Bağlantı ayarları - Multimedia eklendi
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::PrintSupport
    Qt6::Multimedia
    qcustomplot
    Vulkan::Vulkan
)

# Kurulum çıktısı
include(GNUInstallDirs)
set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install" CACHE PATH "Installation directory")

# Hedef kurulum kuralları
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

# Windows için gerekli Qt DLL'lerini kopyalama
if(WIN32)
    set(QT_INSTALL_PATH "${CMAKE_PREFIX_PATH}")
    if(NOT EXISTS "${QT_INSTALL_PATH}/bin")
        set(QT_INSTALL_PATH "${QT_INSTALL_PATH}/..")
        if(NOT EXISTS "${QT_INSTALL_PATH}/bin")
            set(QT_INSTALL_PATH "${QT_INSTALL_PATH}/..")
        endif()
    endif()
    
    # Platforms DLL kopyalama
    if(EXISTS "${QT_INSTALL_PATH}/plugins/platforms/qwindows.dll")
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>/plugins/platforms/")
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
                "${QT_INSTALL_PATH}/plugins/platforms/qwindows.dll"
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>/plugins/platforms/")
    endif()
    
    # Multimedia plugins kopyalama
    if(EXISTS "${QT_INSTALL_PATH}/plugins/multimedia")
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>/plugins/multimedia/")
        
        # Multimedia plugin dosyalarını kopyala
        file(GLOB MULTIMEDIA_PLUGINS "${QT_INSTALL_PATH}/plugins/multimedia/*.dll")
        foreach(PLUGIN ${MULTIMEDIA_PLUGINS})
            add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy
                    "${PLUGIN}"
                    "$<TARGET_FILE_DIR:${PROJECT_NAME}>/plugins/multimedia/")
        endforeach()
    endif()
    
    # Qt DLL'lerini kopyalama - Multimedia eklendi
    foreach(QT_LIB Core Gui Widgets PrintSupport Multimedia)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
                "${QT_INSTALL_PATH}/bin/Qt6${QT_LIB}.dll"
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>")
    endforeach(QT_LIB)
endif()