############################################################################
# CMakeLists.txt file for building ROOT graf2d/x11ttf package
# @author Pere Mato, CERN
############################################################################
set(libname GX11TTF)

ROOT_USE_PACKAGE(graf2d/graf)
ROOT_USE_PACKAGE(graf2d/x11)
include_directories(${FREETYPE_INCLUDE_DIRS} ${X11_INCLUDE_DIR})

ROOT_GENERATE_DICTIONARY(G__${libname} *.h LINKDEF LinkDef.h)
ROOT_GENERATE_ROOTMAP(${libname} LINKDEF LinkDef.h DEPENDENCIES GX11 Graf)

ROOT_LINKER_LIBRARY(${libname} *.cxx G__${libname}.cxx 
                    LIBRARIES ${X11_LIBRARIES} ${X11_Xpm_LIB} ${X11_Xft_LIB} ${FREETYPE_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT}
                    DEPENDENCIES GX11 Graf)
ROOT_INSTALL_HEADERS()

