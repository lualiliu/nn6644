cmake_minimum_required(VERSION 3.10.2)
project(nn6644-Core)

# 设置C/C++标准
set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 11)

# 编译器特定选项
if(MSVC)
    add_compile_options(/W3)
else()
    add_compile_options(-Wall -Wno-multichar -Wno-unused-function -Wno-unused-variable)
endif()

# 全局定义
add_definitions(-D__LIBRETRO__)
add_definitions(-DM64P_PLUGIN_API)
add_definitions(-DM64P_CORE_PROTOTYPES)
add_definitions(-D_CRT_SECURE_NO_WARNINGS)
add_definitions(-DVIDEO_GLIDEN64)
add_definitions(-DMUPENPLUSAPI)
add_definitions(-DDYNAREC)
# 配置选项
option(SYSTEM_LIBPNG "Use system libpng" OFF)
option(SYSTEM_MINIZIP "Use system minizip" OFF)
option(SYSTEM_XXHASH "Use system xxhash" OFF)
option(SYSTEM_ZLIB "Use system zlib" OFF)
option(WITH_DYNAREC "Enable dynarec" ON)
option(HAVE_THR_AL "Enable Angrylion renderer" OFF)
option(LLE "Enable LLE RSP" OFF)
option(HAVE_PARALLEL_RDP "Enable Parallel RDP" OFF)
option(HAVE_PARALLEL_RSP "Enable Parallel RSP" OFF)
option(HAVE_NEON "Enable NEON optimizations" OFF)
option(GLES "Enable GLES" OFF)
option(GLES3 "Enable GLES3" OFF)
option(ANDROID "Build for Android" OFF)

# 设置源码根目录
set(ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})

# 目录定义
set(RSPDIR ${ROOT_DIR}/mupen64plus-rsp-hle)
set(CXD4DIR ${ROOT_DIR}/mupen64plus-rsp-cxd4)
set(CORE_DIR ${ROOT_DIR}/mupen64plus-core)
set(LIBRETRO_DIR ${ROOT_DIR}/libretro)
set(LIBRETRO_COMM_DIR ${ROOT_DIR}/libretro-common)
set(RSPDIR_PARALLEL ${ROOT_DIR}/mupen64plus-rsp-paraLLEl)
set(AUDIO_LIBRETRO_DIR ${ROOT_DIR}/custom/mupen64plus-core/plugin/audio_libretro)
set(VIDEODIR_GLIDEN64 ${ROOT_DIR}/GLideN64)
set(VIDEODIR_ANGRYLION ${ROOT_DIR}/mupen64plus-video-angrylion)
set(VIDEODIR_PARALLEL ${ROOT_DIR}/mupen64plus-video-paraLLEl)

set(MINIZIP_DIR ${CORE_DIR}/subprojects/minizip)
set(LIBPNG_DIR ${ROOT_DIR}/custom/dependencies/libpng)
set(XXHASH_DIR ${ROOT_DIR}/xxHash)
set(ZLIB_DIR ${ROOT_DIR}/custom/dependencies/libzlib)

# 包含目录
include_directories(
    ${ROOT_DIR}/custom
    ${ROOT_DIR}/custom/mupen64plus-core
    ${ROOT_DIR}/custom/android/include
    ${ROOT_DIR}/custom/GLideN64
    ${VIDEODIR_GLIDEN64}/src
    ${VIDEODIR_GLIDEN64}/src/osal
    ${CORE_DIR}/src
    ${CORE_DIR}/src/api
    ${AUDIO_LIBRETRO_DIR}
    ${LIBRETRO_COMM_DIR}/include
    ${LIBRETRO_COMM_DIR}
    ${VIDEODIR_GLIDEN64}/src/inc
    ${CORE_DIR}/subprojects/md5
    ${LIBRETRO_DIR}
)

# 源文件列表
set(SOURCES_C
    ${CORE_DIR}/src/asm_defines/asm_defines.c
    ${CORE_DIR}/src/api/callbacks.c
    ${ROOT_DIR}/custom/mupen64plus-core/api/config.c
    ${CORE_DIR}/src/api/debugger.c
    ${CORE_DIR}/src/api/frontend.c
    ${CORE_DIR}/src/backends/plugins_compat/audio_plugin_compat.c
    ${CORE_DIR}/src/backends/api/video_capture_backend.c
    ${CORE_DIR}/src/backends/plugins_compat/input_plugin_compat.c
    ${CORE_DIR}/src/backends/clock_ctime_plus_delta.c
    ${CORE_DIR}/src/backends/dummy_video_capture.c
    ${CORE_DIR}/src/backends/file_storage.c
    ${CORE_DIR}/src/device/cart/cart.c
    ${CORE_DIR}/src/device/cart/af_rtc.c
    ${CORE_DIR}/src/device/cart/cart_rom.c
    ${CORE_DIR}/src/device/cart/eeprom.c
    ${CORE_DIR}/src/device/cart/flashram.c
    ${CORE_DIR}/src/device/cart/is_viewer.c
    ${CORE_DIR}/src/device/cart/sram.c
    ${CORE_DIR}/src/device/controllers/game_controller.c
    ${CORE_DIR}/src/device/controllers/vru_controller.c
    ${CORE_DIR}/src/device/controllers/paks/biopak.c
    ${CORE_DIR}/src/device/controllers/paks/mempak.c
    ${CORE_DIR}/src/device/controllers/paks/rumblepak.c
    ${CORE_DIR}/src/device/controllers/paks/transferpak.c
    ${CORE_DIR}/src/device/dd/dd_controller.c
    ${CORE_DIR}/src/device/dd/disk.c
    ${CORE_DIR}/src/device/device.c
    ${CORE_DIR}/src/device/gb/gb_cart.c
    ${CORE_DIR}/src/device/gb/mbc3_rtc.c
    ${CORE_DIR}/src/device/gb/m64282fp.c
    ${CORE_DIR}/src/device/memory/memory.c
    ${CORE_DIR}/src/device/pif/bootrom_hle.c
    ${CORE_DIR}/src/device/pif/cic.c
    ${CORE_DIR}/src/device/pif/n64_cic_nus_6105.c
    ${CORE_DIR}/src/device/pif/pif.c
    ${CORE_DIR}/src/device/r4300/cached_interp.c
    ${CORE_DIR}/src/device/r4300/cp0.c
    ${CORE_DIR}/src/device/r4300/cp1.c
    ${CORE_DIR}/src/device/r4300/cp2.c
    ${CORE_DIR}/src/device/r4300/idec.c
    ${CORE_DIR}/src/device/r4300/interrupt.c
    ${CORE_DIR}/src/device/r4300/pure_interp.c
    ${CORE_DIR}/src/device/r4300/r4300_core.c
    ${CORE_DIR}/src/device/r4300/tlb.c
    ${CORE_DIR}/src/device/rcp/ai/ai_controller.c
    ${CORE_DIR}/src/device/rcp/mi/mi_controller.c
    ${CORE_DIR}/src/device/rcp/pi/pi_controller.c
    ${CORE_DIR}/src/device/rcp/rdp/fb.c
    ${CORE_DIR}/src/device/rcp/rdp/rdp_core.c
    ${CORE_DIR}/src/device/rcp/ri/ri_controller.c
    ${CORE_DIR}/src/device/rcp/rsp/rsp_core.c
    ${CORE_DIR}/src/device/rcp/si/si_controller.c
    ${CORE_DIR}/src/device/rcp/vi/vi_controller.c
    ${CORE_DIR}/src/device/rdram/rdram.c
    ${CORE_DIR}/src/main/main.c
    ${CORE_DIR}/src/main/util.c
    ${CORE_DIR}/src/main/cheat.c
    ${CORE_DIR}/src/main/rom.c
    ${CORE_DIR}/src/main/savestates.c
    ${CORE_DIR}/src/plugin/plugin.c
    ${CORE_DIR}/src/plugin/dummy_audio.c
    ${CORE_DIR}/src/plugin/dummy_input.c
    ${CORE_DIR}/subprojects/md5/md5.c
)

# 平台特定的文件
if(WIN32)
    list(APPEND SOURCES_C ${VIDEODIR_GLIDEN64}/src/osal/osal_files_win32.c)
else()
    list(APPEND SOURCES_C ${VIDEODIR_GLIDEN64}/src/osal/osal_files_unix.c)
endif()

# 依赖库处理
if(SYSTEM_LIBPNG)
    find_package(PNG REQUIRED)
else()
    include_directories(${LIBPNG_DIR})
    set(LIBPNG_SOURCES_C
        ${LIBPNG_DIR}/png.c
        ${LIBPNG_DIR}/pngerror.c
        ${LIBPNG_DIR}/pngget.c
        ${LIBPNG_DIR}/pngmem.c
        ${LIBPNG_DIR}/pngpread.c
        ${LIBPNG_DIR}/pngread.c
        ${LIBPNG_DIR}/pngrio.c
        ${LIBPNG_DIR}/pngrtran.c
        ${LIBPNG_DIR}/pngrutil.c
        ${LIBPNG_DIR}/pngset.c
        ${LIBPNG_DIR}/pngtrans.c
        ${LIBPNG_DIR}/pngwio.c
        ${LIBPNG_DIR}/pngwrite.c
        ${LIBPNG_DIR}/pngwtran.c
        ${LIBPNG_DIR}/pngwutil.c
    )
    list(APPEND SOURCES_C ${LIBPNG_SOURCES_C})
endif()

if(SYSTEM_MINIZIP)
    find_package(minizip REQUIRED)
else()
    include_directories(${MINIZIP_DIR})
    set(MINIZIP_SOURCES_C
        ${MINIZIP_DIR}/zip.c
        ${MINIZIP_DIR}/unzip.c
        ${MINIZIP_DIR}/ioapi.c
    )
    list(APPEND SOURCES_C ${MINIZIP_SOURCES_C})
endif()

if(SYSTEM_ZLIB)
    find_package(ZLIB REQUIRED)
else()
    include_directories(${ZLIB_DIR})
    set(ZLIB_SOURCES_C
        ${ZLIB_DIR}/adler32.c
        ${ZLIB_DIR}/compress.c
        ${ZLIB_DIR}/crc32.c
        ${ZLIB_DIR}/deflate.c
        ${ZLIB_DIR}/gzclose.c
        ${ZLIB_DIR}/gzlib.c
        ${ZLIB_DIR}/gzread.c
        ${ZLIB_DIR}/gzwrite.c
        ${ZLIB_DIR}/infback.c
        ${ZLIB_DIR}/inffast.c
        ${ZLIB_DIR}/inflate.c
        ${ZLIB_DIR}/inftrees.c
        ${ZLIB_DIR}/trees.c
        ${ZLIB_DIR}/uncompr.c
        ${ZLIB_DIR}/zutil.c
    )
    list(APPEND SOURCES_C ${ZLIB_SOURCES_C})
endif()

if(SYSTEM_XXHASH)
    find_package(xxhash REQUIRED)
else()
    include_directories(${XXHASH_DIR})
endif()

# Libretro 源文件
list(APPEND SOURCES_C
    ${LIBRETRO_DIR}/libretro.c
    ${LIBRETRO_COMM_DIR}/memmap/memalign.c
    ${ROOT_DIR}/custom/mupen64plus-core/plugin/emulate_game_controller_via_libretro.c
    ${LIBRETRO_COMM_DIR}/audio/resampler/drivers/sinc_resampler.c
    ${LIBRETRO_COMM_DIR}/audio/resampler/drivers/nearest_resampler.c
    ${LIBRETRO_COMM_DIR}/audio/resampler/audio_resampler.c
    ${AUDIO_LIBRETRO_DIR}/audio_backend_libretro.c
    ${LIBRETRO_COMM_DIR}/file/config_file.c
    ${LIBRETRO_COMM_DIR}/file/config_file_userdata.c
    ${LIBRETRO_COMM_DIR}/file/file_path.c
    ${LIBRETRO_COMM_DIR}/file/file_path_io.c
    ${LIBRETRO_COMM_DIR}/time/rtime.c
    ${LIBRETRO_COMM_DIR}/compat/compat_strl.c
    ${LIBRETRO_COMM_DIR}/compat/compat_posix_string.c
    ${LIBRETRO_COMM_DIR}/compat/compat_strcasestr.c
    ${LIBRETRO_COMM_DIR}/audio/conversion/float_to_s16.c
    ${LIBRETRO_COMM_DIR}/audio/conversion/s16_to_float.c
    ${LIBRETRO_COMM_DIR}/features/features_cpu.c
    ${LIBRETRO_COMM_DIR}/lists/string_list.c
    ${LIBRETRO_COMM_DIR}/encodings/encoding_utf.c
    ${LIBRETRO_COMM_DIR}/string/stdstring.c
    ${LIBRETRO_COMM_DIR}/vfs/vfs_implementation.c
    ${LIBRETRO_COMM_DIR}/streams/file_stream.c
    ${LIBRETRO_COMM_DIR}/compat/fopen_utf8.c
    ${ROOT_DIR}/custom/mupen64plus-core/api/vidext_libretro.c
    ${LIBRETRO_COMM_DIR}/glsm/glsm.c
    ${LIBRETRO_COMM_DIR}/libco/libco.c
)

# GLideN64 C++ 源文件
set(SOURCES_CXX
    ${VIDEODIR_GLIDEN64}/src/Combiner.cpp
    ${VIDEODIR_GLIDEN64}/src/CombinerKey.cpp
    ${VIDEODIR_GLIDEN64}/src/CommonPluginAPI.cpp
    ${VIDEODIR_GLIDEN64}/src/Config.cpp
    ${VIDEODIR_GLIDEN64}/src/convert.cpp
    ${VIDEODIR_GLIDEN64}/src/DebugDump.cpp
    ${VIDEODIR_GLIDEN64}/src/Debugger.cpp
    ${VIDEODIR_GLIDEN64}/src/DepthBuffer.cpp
    ${VIDEODIR_GLIDEN64}/src/DisplayWindow.cpp
    ${VIDEODIR_GLIDEN64}/src/Graphics/OpenGLContext/mupen64plus/mupen64plus_DisplayWindow.cpp
    ${VIDEODIR_GLIDEN64}/src/DisplayLoadProgress.cpp
    ${VIDEODIR_GLIDEN64}/src/FrameBuffer.cpp
    ${VIDEODIR_GLIDEN64}/src/FrameBufferInfo.cpp
    ${VIDEODIR_GLIDEN64}/src/GBI.cpp
    ${VIDEODIR_GLIDEN64}/src/gDP.cpp
    ${VIDEODIR_GLIDEN64}/src/GLideN64.cpp
    ${VIDEODIR_GLIDEN64}/src/gSP.cpp
    ${VIDEODIR_GLIDEN64}/src/N64.cpp
    ${VIDEODIR_GLIDEN64}/src/TextDrawer.cpp
    ${VIDEODIR_GLIDEN64}/src/PaletteTexture.cpp
    ${VIDEODIR_GLIDEN64}/src/Performance.cpp
    ${VIDEODIR_GLIDEN64}/src/PostProcessor.cpp
    ${VIDEODIR_GLIDEN64}/src/RDP.cpp
    ${VIDEODIR_GLIDEN64}/src/RSP.cpp
    ${VIDEODIR_GLIDEN64}/src/SoftwareRender.cpp
    ${VIDEODIR_GLIDEN64}/src/TexrectDrawer.cpp
    ${VIDEODIR_GLIDEN64}/src/TextureFilterHandler.cpp
    ${VIDEODIR_GLIDEN64}/src/Textures.cpp
    ${VIDEODIR_GLIDEN64}/src/VI.cpp
    ${VIDEODIR_GLIDEN64}/src/ZlutTexture.cpp
    ${VIDEODIR_GLIDEN64}/src/common/CommonAPIImpl_common.cpp
    ${VIDEODIR_GLIDEN64}/src/DepthBufferRender/ClipPolygon.cpp
    ${VIDEODIR_GLIDEN64}/src/DepthBufferRender/DepthBufferRender.cpp
    ${VIDEODIR_GLIDEN64}/src/BufferCopy/BlueNoiseTexture.cpp
    ${VIDEODIR_GLIDEN64}/src/BufferCopy/ColorBufferToRDRAM.cpp
    ${VIDEODIR_GLIDEN64}/src/BufferCopy/DepthBufferToRDRAM.cpp
    ${VIDEODIR_GLIDEN64}/src/BufferCopy/RDRAMtoColorBuffer.cpp
    ${VIDEODIR_GLIDEN64}/src/GraphicsDrawer.cpp
    ${VIDEODIR_GLIDEN64}/src/Graphics/Context.cpp
    ${VIDEODIR_GLIDEN64}/src/Graphics/ColorBufferReader.cpp
    ${VIDEODIR_GLIDEN64}/src/Graphics/CombinerProgram.cpp
    ${VIDEODIR_GLIDEN64}/src/Graphics/ObjectHandle.cpp
    ${VIDEODIR_GLIDEN64}/src/Graphics/OpenGLContext/GLFunctions.cpp
    ${VIDEODIR_GLIDEN64}/src/Graphics/OpenGLContext/ThreadedOpenGl/opengl_Wrapper.cpp
    ${VIDEODIR_GLIDEN64}/src/Graphics/OpenGLContext/ThreadedOpenGl/opengl_WrappedFunctions.cpp
    ${VIDEODIR_GLIDEN64}/src/Graphics/OpenGLContext/ThreadedOpenGl/opengl_Command.cpp
    ${VIDEODIR_GLIDEN64}/src/Graphics/OpenGLContext/ThreadedOpenGl/opengl_ObjectPool.cpp
    ${VIDEODIR_GLIDEN64}/src/Graphics/OpenGLContext/ThreadedOpenGl/RingBufferPool.cpp
    ${VIDEODIR_GLIDEN64}/src/Graphics/OpenGLContext/opengl_Attributes.cpp
    ${VIDEODIR_GLIDEN64}/src/Graphics/OpenGLContext/opengl_BufferedDrawer.cpp
    ${VIDEODIR_GLIDEN64}/src/Graphics/OpenGLContext/opengl_BufferManipulationObjectFactory.cpp
    ${VIDEODIR_GLIDEN64}/src/Graphics/OpenGLContext/opengl_CachedFunctions.cpp
    ${VIDEODIR_GLIDEN64}/src/Graphics/OpenGLContext/opengl_ColorBufferReaderWithBufferStorage.cpp
    ${VIDEODIR_GLIDEN64}/src/Graphics/OpenGLContext/opengl_ColorBufferReaderWithPixelBuffer.cpp
    ${VIDEODIR_GLIDEN64}/src/Graphics/OpenGLContext/opengl_ColorBufferReaderWithReadPixels.cpp
    ${VIDEODIR_GLIDEN64}/src/Graphics/OpenGLContext/opengl_ColorBufferReaderWithEGLImage.cpp
    ${VIDEODIR_GLIDEN64}/src/Graphics/OpenGLContext/opengl_ContextImpl.cpp
    ${VIDEODIR_GLIDEN64}/src/Graphics/OpenGLContext/opengl_GLInfo.cpp
    ${VIDEODIR_GLIDEN64}/src/Graphics/OpenGLContext/opengl_Parameters.cpp
    ${VIDEODIR_GLIDEN64}/src/Graphics/OpenGLContext/opengl_TextureManipulationObjectFactory.cpp
    ${VIDEODIR_GLIDEN64}/src/Graphics/OpenGLContext/opengl_UnbufferedDrawer.cpp
    ${VIDEODIR_GLIDEN64}/src/Graphics/OpenGLContext/opengl_Utils.cpp
    ${VIDEODIR_GLIDEN64}/src/Graphics/OpenGLContext/GLSL/glsl_CombinerInputs.cpp
    ${VIDEODIR_GLIDEN64}/src/Graphics/OpenGLContext/GLSL/glsl_CombinerProgramBuilder.cpp
    ${VIDEODIR_GLIDEN64}/src/Graphics/OpenGLContext/GLSL/glsl_CombinerProgramImpl.cpp
    ${VIDEODIR_GLIDEN64}/src/Graphics/OpenGLContext/GLSL/glsl_CombinerProgramUniformFactory.cpp
    ${VIDEODIR_GLIDEN64}/src/Graphics/OpenGLContext/GLSL/glsl_CombinerProgramUniformFactoryAccurate.cpp
    ${VIDEODIR_GLIDEN64}/src/Graphics/OpenGLContext/GLSL/glsl_CombinerProgramUniformFactoryFast.cpp
    ${VIDEODIR_GLIDEN64}/src/Graphics/OpenGLContext/GLSL/glsl_CombinerProgramUniformFactoryCommon.cpp
    ${VIDEODIR_GLIDEN64}/src/Graphics/OpenGLContext/GLSL/glsl_CombinerProgramBuilderCommon.cpp
    ${VIDEODIR_GLIDEN64}/src/Graphics/OpenGLContext/GLSL/glsl_CombinerProgramBuilderAccurate.cpp
    ${VIDEODIR_GLIDEN64}/src/Graphics/OpenGLContext/GLSL/glsl_CombinerProgramBuilderFast.cpp
    ${VIDEODIR_GLIDEN64}/src/Graphics/OpenGLContext/GLSL/glsl_FXAA.cpp
    ${VIDEODIR_GLIDEN64}/src/Graphics/OpenGLContext/GLSL/glsl_ShaderStorage.cpp
    ${VIDEODIR_GLIDEN64}/src/Graphics/OpenGLContext/GLSL/glsl_SpecialShadersFactory.cpp
    ${VIDEODIR_GLIDEN64}/src/Graphics/OpenGLContext/GLSL/glsl_Utils.cpp
    ${VIDEODIR_GLIDEN64}/src/Graphics/OpenGLContext/GraphicBuffer/PrivateApi/GraphicBuffer.cpp
    ${VIDEODIR_GLIDEN64}/src/mupenplus/MemoryStatus_mupenplus.cpp
    ${VIDEODIR_GLIDEN64}/src/uCodes/F3D.cpp
    ${VIDEODIR_GLIDEN64}/src/uCodes/F3DAM.cpp
    ${VIDEODIR_GLIDEN64}/src/uCodes/F3DBETA.cpp
    ${VIDEODIR_GLIDEN64}/src/uCodes/F3DDKR.cpp
    ${VIDEODIR_GLIDEN64}/src/uCodes/F3DEX.cpp
    ${VIDEODIR_GLIDEN64}/src/uCodes/F3DEX2.cpp
    ${VIDEODIR_GLIDEN64}/src/uCodes/F3DEX3.cpp
    ${VIDEODIR_GLIDEN64}/src/uCodes/F3DEX095.cpp
    ${VIDEODIR_GLIDEN64}/src/uCodes/F3DEX2ACCLAIM.cpp
    ${VIDEODIR_GLIDEN64}/src/uCodes/F3DEX2CBFD.cpp
    ${VIDEODIR_GLIDEN64}/src/uCodes/F3DZEX2.cpp
    ${VIDEODIR_GLIDEN64}/src/uCodes/F3DFLX2.cpp
    ${VIDEODIR_GLIDEN64}/src/uCodes/F3DGOLDEN.cpp
    ${VIDEODIR_GLIDEN64}/src/uCodes/F3DPD.cpp
    ${VIDEODIR_GLIDEN64}/src/uCodes/F3DSETA.cpp
    ${VIDEODIR_GLIDEN64}/src/uCodes/F5Indi_Naboo.cpp
    ${VIDEODIR_GLIDEN64}/src/uCodes/F5Rogue.cpp
    ${VIDEODIR_GLIDEN64}/src/uCodes/F3DTEXA.cpp
    ${VIDEODIR_GLIDEN64}/src/uCodes/L3D.cpp
    ${VIDEODIR_GLIDEN64}/src/uCodes/L3DEX2.cpp
    ${VIDEODIR_GLIDEN64}/src/uCodes/L3DEX.cpp
    ${VIDEODIR_GLIDEN64}/src/uCodes/S2DEX2.cpp
    ${VIDEODIR_GLIDEN64}/src/uCodes/S2DEX.cpp
    ${VIDEODIR_GLIDEN64}/src/uCodes/T3DUX.cpp
    ${VIDEODIR_GLIDEN64}/src/uCodes/Turbo3D.cpp
    ${VIDEODIR_GLIDEN64}/src/uCodes/ZSort.cpp
    ${VIDEODIR_GLIDEN64}/src/uCodes/ZSortBOSS.cpp
    ${VIDEODIR_GLIDEN64}/src/MupenPlusPluginAPI.cpp
    ${VIDEODIR_GLIDEN64}/src/mupenplus/MupenPlusAPIImpl.cpp
    ${ROOT_DIR}/custom/GLideN64/mupenplus/Config_mupenplus.cpp
    ${ROOT_DIR}/custom/GLideN64/mupenplus/CommonAPIImpl_mupenplus.cpp
    ${VIDEODIR_GLIDEN64}/src/Log.cpp
)

# GLideNHQ 源文件
list(APPEND SOURCES_CXX
    ${VIDEODIR_GLIDEN64}/src/GLideNHQ/TextureFilters.cpp
    ${VIDEODIR_GLIDEN64}/src/GLideNHQ/TextureFilters_2xsai.cpp
    ${VIDEODIR_GLIDEN64}/src/GLideNHQ/TextureFilters_hq2x.cpp
    ${VIDEODIR_GLIDEN64}/src/GLideNHQ/TextureFilters_hq4x.cpp
    ${VIDEODIR_GLIDEN64}/src/GLideNHQ/TextureFilters_xbrz.cpp
    ${VIDEODIR_GLIDEN64}/src/GLideNHQ/TxCache.cpp
    ${VIDEODIR_GLIDEN64}/src/GLideNHQ/TxDbg.cpp
    ${VIDEODIR_GLIDEN64}/src/GLideNHQ/TxFilter.cpp
    ${VIDEODIR_GLIDEN64}/src/GLideNHQ/TxFilterExport.cpp
    ${VIDEODIR_GLIDEN64}/src/GLideNHQ/TxHiResCache.cpp
    ${VIDEODIR_GLIDEN64}/src/GLideNHQ/TxHiResNoCache.cpp
    ${VIDEODIR_GLIDEN64}/src/GLideNHQ/TxHiResLoader.cpp
    ${VIDEODIR_GLIDEN64}/src/GLideNHQ/TxImage.cpp
    ${VIDEODIR_GLIDEN64}/src/GLideNHQ/TxQuantize.cpp
    ${VIDEODIR_GLIDEN64}/src/GLideNHQ/TxReSample.cpp
    ${VIDEODIR_GLIDEN64}/src/GLideNHQ/TxTexCache.cpp
    ${VIDEODIR_GLIDEN64}/src/GLideNHQ/TxUtil.cpp
    ${VIDEODIR_GLIDEN64}/src/RSP_LoadMatrix.cpp
)

# RSP HLE 源文件
list(APPEND SOURCES_C
    ${RSPDIR}/src/alist.c
    ${RSPDIR}/src/alist_audio.c
    ${RSPDIR}/src/alist_naudio.c
    ${RSPDIR}/src/alist_nead.c
    ${RSPDIR}/src/audio.c
    ${RSPDIR}/src/cicx105.c
    ${RSPDIR}/src/hle.c
    ${RSPDIR}/src/hvqm.c
    ${RSPDIR}/src/jpeg.c
    ${RSPDIR}/src/memory.c
    ${RSPDIR}/src/mp3.c
    ${RSPDIR}/src/musyx.c
    ${RSPDIR}/src/re2.c
    ${RSPDIR}/src/plugin.c
)

# 动态重编译配置
if(WITH_DYNAREC)
    add_definitions(-DDYNAREC)
    
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm" OR CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64")
        if(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64")
            add_definitions(-DNEW_DYNAREC=4)
            list(APPEND SOURCES_C
                ${CORE_DIR}/src/device/r4300/new_dynarec/new_dynarec.c
            )
            # 注意：需要处理 ARM64 汇编文件
        else()
            add_definitions(-DNEW_DYNAREC=3)
            list(APPEND SOURCES_C
                ${CORE_DIR}/src/device/r4300/new_dynarec/new_dynarec.c
                ${CORE_DIR}/src/device/r4300/new_dynarec/arm/arm_cpu_features.c
            )
            # 注意：需要处理 ARM 汇编文件
        endif()
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64")
        add_definitions(-DNEW_DYNAREC=2 -DARCH_MIN_SSE2)
        add_compile_options(-msse -msse2)
        list(APPEND SOURCES_C
            ${CORE_DIR}/src/device/r4300/new_dynarec/new_dynarec.c
        )
        # 注意：需要处理 x64 汇编文件
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "i386")
        add_definitions(-DNEW_DYNAREC=1 -DARCH_MIN_SSE2)
        add_compile_options(-msse -msse2)
        list(APPEND SOURCES_C
            ${CORE_DIR}/src/device/r4300/new_dynarec/new_dynarec.c
        )
        # 注意：需要处理 x86 汇编文件
    endif()
endif()

# Angrylion 渲染器
if(HAVE_THR_AL)
    add_definitions(-DHAVE_THR_AL)
    list(APPEND SOURCES_CXX ${VIDEODIR_ANGRYLION}/parallel_al.cpp)
    list(APPEND SOURCES_C
        ${VIDEODIR_ANGRYLION}/interface.c
        ${VIDEODIR_ANGRYLION}/n64video.c
    )
endif()

# LLE RSP
if(LLE)
    list(APPEND SOURCES_C ${CXD4DIR}/rsp.c)
endif()

# 并行 RDP
if(HAVE_PARALLEL_RDP)
    add_definitions(-DHAVE_PARALLEL_RDP)
    # 这里需要包含 parallel-rdp 的配置
    # 由于 parallel-rdp 有自己的构建系统，可能需要作为子项目处理
endif()

# 并行 RSP  
if(HAVE_PARALLEL_RSP)
    add_definitions(-DHAVE_PARALLEL_RSP -DPARALLEL_INTEGRATION)
    # 这里需要添加 Parallel RSP 的源文件
endif()

# NEON 优化
if(HAVE_NEON)
    add_definitions(-DHAVE_NEON)
    list(APPEND SOURCES_CXX
        ${VIDEODIR_GLIDEN64}/src/Neon/3DMathNeon.cpp
        ${VIDEODIR_GLIDEN64}/src/Neon/gSPNeon.cpp
    )
else()
    list(APPEND SOURCES_CXX ${VIDEODIR_GLIDEN64}/src/3DMath.cpp)
endif()

# CRC 优化
if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm" OR CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64")
    list(APPEND SOURCES_CXX ${VIDEODIR_GLIDEN64}/src/CRC32_ARMV8.cpp)
else()
    list(APPEND SOURCES_CXX ${VIDEODIR_GLIDEN64}/src/CRC_OPT.cpp)
endif()

# OpenGL/GLES 配置
if(GLES3)
    add_definitions(-DHAVE_OPENGLES -DHAVE_OPENGLES3 -DGLES3)
    if(NOT ANDROID)
        add_definitions(-DEGL)
    endif()
    find_package(OpenGLES3 REQUIRED)
    list(APPEND SOURCES_C ${LIBRETRO_COMM_DIR}/glsym/glsym_es3.c)
elseif(GLES)
    add_definitions(-DHAVE_OPENGLES -DHAVE_OPENGLES2 -DGLES2)
    if(NOT ANDROID)
        add_definitions(-DEGL)
    endif()
    find_package(OpenGLES2 REQUIRED)
    list(APPEND SOURCES_C ${LIBRETRO_COMM_DIR}/glsym/glsym_es2.c)
else()
    add_definitions(-DHAVE_OPENGL)
    if(NOT WIN32)
        add_definitions(-DCORE)
    endif()
    find_package(OpenGL REQUIRED)
    list(APPEND SOURCES_C ${LIBRETRO_COMM_DIR}/glsym/glsym_gl.c)
endif()

list(APPEND SOURCES_C
    ${LIBRETRO_COMM_DIR}/glsym/rglgen.c
)

# iOS/tvOS 特定文件
if(IOS OR TVOS)
    list(APPEND SOURCES_C ${ROOT_DIR}/custom/ios/compat.c)
endif()

# Android 特定文件
if(ANDROID)
    if(GLES OR GLES3)
        list(APPEND SOURCES_CXX
            ${VIDEODIR_GLIDEN64}/src/Graphics/OpenGLContext/GraphicBuffer/GraphicBufferWrapper.cpp
            ${VIDEODIR_GLIDEN64}/src/Graphics/OpenGLContext/GraphicBuffer/PublicApi/android_hardware_buffer_compat.cpp
        )
    endif()
endif()

# 创建库
enable_language(ASM_NASM)
find_program(NASM nasm REQUIRED)

# x64 动态重编译汇编文件
if(WITH_DYNAREC AND CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64")
    set(X64_ASM_SOURCES
        ${CORE_DIR}/src/device/r4300/new_dynarec/x64/linkage_x64.asm
    )
    
    set(ASM_INCLUDE_DIR ${CORE_DIR}/src/device/r4300/new_dynarec/x64)
    foreach(asm_file ${X64_ASM_SOURCES})
        get_filename_component(asm_name ${asm_file} NAME_WE)
        set(obj_file ${CMAKE_CURRENT_BINARY_DIR}/${asm_name}_asm.o)
        
    add_custom_command(
        OUTPUT ${obj_file}
        COMMAND ${NASM} -I${ASM_INCLUDE_DIR}/ -f elf64 -o ${obj_file} ${asm_file}
        DEPENDS ${asm_file}
        COMMENT "Assembling ${asm_name}.asm"
        VERBATIM
    )
        
        list(APPEND SOURCE_ASM ${obj_file})
    endforeach()
    
    add_custom_target(nasm_objects DEPENDS ${SOURCE_ASM})
endif()

add_library(${PROJECT_NAME} STATIC ${SOURCES_C} ${SOURCES_CXX} ${SOURCE_ASM})


# 设置目标属性
set_target_properties(${PROJECT_NAME} PROPERTIES
    C_STANDARD 99
    CXX_STANDARD 11
    POSITION_INDEPENDENT_CODE ON
)

# 链接库
if(SYSTEM_LIBPNG)
    target_link_libraries(${PROJECT_NAME} PNG::PNG)
endif()

if(SYSTEM_MINIZIP)
    target_link_libraries(${PROJECT_NAME} minizip)
endif()

if(SYSTEM_ZLIB)
    target_link_libraries(${PROJECT_NAME} ZLIB::ZLIB)
endif()

if(SYSTEM_XXHASH)
    target_link_libraries(${PROJECT_NAME} xxhash::xxhash)
endif()

if(GLES3)
    target_link_libraries(${PROJECT_NAME} OpenGL::GLES3)
    if(NOT ANDROID)
        target_link_libraries(${PROJECT_NAME} EGL)
    endif()
elseif(GLES)
    target_link_libraries(${PROJECT_NAME} OpenGL::GLES2)
    if(NOT ANDROID)
        target_link_libraries(${PROJECT_NAME} EGL)
    endif()
else()
    target_link_libraries(${PROJECT_NAME} OpenGL::GL)
endif()

# 安装规则（如果需要）
# install(TARGETS ${PROJECT_NAME} DESTINATION lib)

# 输出信息
message(STATUS "Building ${PROJECT_NAME}")
message(STATUS "System libpng: ${SYSTEM_LIBPNG}")
message(STATUS "System minizip: ${SYSTEM_MINIZIP}")
message(STATUS "System zlib: ${SYSTEM_ZLIB}")
message(STATUS "System xxhash: ${SYSTEM_XXHASH}")
message(STATUS "Dynarec enabled: ${WITH_DYNAREC}")
message(STATUS "GLES: ${GLES}")
message(STATUS "GLES3: ${GLES3}")
