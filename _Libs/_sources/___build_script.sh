#!/bin/bash

#Install Builds into _builds/linux directory
#InstallTo=~0/../_builds/linux

#Install Builds into _builds/macos directory
#InstallTo=~0/../_builds/macos

#Install Builds into /usr/ directory globally
#InstallTo = /usr/

CURRENT_TARBALL=""
CACHE_DIR="_build_cache"

OurOS="linux_defaut"

if [[ "$OSTYPE" == "darwin"* ]]; then
    OurOS="macos"
elif [[ "$OSTYPE" == "linux-gnu" || "$OSTYPE" == "linux" ]]; then
    OurOS="linux"
elif [[ "$OSTYPE" == "freebsd"* ]]; then
    OurOS="freebsd"
fi

if [[ "$OurOS" != "macos" ]]; then
    Sed=sed
else
    Sed=gsed
fi

#=======================================================================
errorofbuild()
{
    printf "\n\n=========\E[37;41mAN ERROR OCCURED!\E[0m==========\n"
    printf "Build failed in the $CURRENT_TARBALL component\n\n"
	exit 1
}
#=======================================================================

LatestSDL=$(find . -maxdepth 1 -name "SDL-*.tar.gz" | sed "s/\.tar\.gz//;s/\.\///");
echo "=====Latest SDL is $LatestSDL====="

UnArch()
{
# $1 - archive name
    if [ ! -d $1 ]
	    then
        printf "tar -xf ../$1.tar.*z* ..."
	    tar -xf ../$1.tar.*z*
        if [ $? -eq 0 ];
        then
            printf "OK!\n"
        else
            printf "FAILED!\n"
        fi
    fi
}

BuildSrc()
{
# $1 - dir name   #2 additional props

    cd $1
    #Build debug version of SDL
    #CFLAGS='-O0 -g' ./configure $2

    echo ----------------------------------------------------------------------
    echo "./configure $2"
    echo ----------------------------------------------------------------------

    ./configure $2

    if [ $? -eq 0 ]
    then
        printf "\n[Configure completed]\n\n"
    else
        errorofbuild
    fi

    make --jobs=2
    if [ $? -eq 0 ]
    then
        printf "\n[Make completed]\n\n"
    else
        errorofbuild
    fi

    echo "Installing..."
    make -s install
    if [ $? -eq 0 ]
    then
        printf "\n[Install completed]\n\n"
    else
        errorofbuild
    fi

    cd ..
}

#############################Build libraries#####################

BuildSampleRate()
{
    CURRENT_TARBALL="libSampleRate"
    UnArch 'libsamplerate-0.1.9'
    printf "=========\E[37;42mlibSampleRate\E[0m===========\n"
    lSRC_ARGS="${lSRC_ARGS} --prefix=${InstallTo}"
    lSRC_ARGS="${lSRC_ARGS} --includedir=${InstallTo}/include"
    lSRC_ARGS="${lSRC_ARGS} --libdir=${InstallTo}/lib"
    lSRC_ARGS="${lSRC_ARGS} CFLAGS=-fPIC CXXFLAGS=-fPIC"
    lSRC_ARGS="${lSRC_ARGS} --enable-static=yes --enable-shared=no"
    BuildSrc 'libsamplerate-0.1.9' "${lSRC_ARGS}"
    # use libSampleRate with SDL:
    SDL_ARGS="${SDL_ARGS} --enable-libsamplerate=yes --disable-libsamplerate-shared "
}

BuildSDL()
{
    CURRENT_TARBALL="SDL2"

    UnArch $LatestSDL

    #--------------Apply some patches--------------
    #++++Fix build on MinGW where are missing tagWAVEINCAPS2W and tagWAVEOUTCAPS2W structures declarations
    #patch -t -N $LatestSDL/src/audio/winmm/SDL_winmm.c < ../patches/SDL_winmm.c.patch
    # FIXED OFFICIALLY: https://hg.libsdl.org/SDL/rev/7e06b0e4dbe0
    #----------------------------------------------

    ###########SDL2###########
    printf "=========\E[37;42mSDL2\E[0m===========\n"
    #sed  -i 's/-version-info [^ ]\+/-avoid-version /g' $LatestSDL'/src/Makefile.am'
    $Sed -i 's/-version-info [^ ]\+/-avoid-version /g' $LatestSDL/Makefile.in
    $Sed -i 's/libSDL2-2\.0\.so\.0/libSDL2\.so/g' $LatestSDL/SDL2.spec.in
    #cd $LatestSDL
    #autoreconf -vfi
    #cd ..
    if [[ "$OurOS" != "macos" ]]; then
        #on any other OS'es build via autotools
        export CFLAGS="-I${InstallTo}/include"
        export LDFLAGS="-L${InstallTo}/lib"
        SDL_ARGS="${SDL_ARGS} --prefix=${InstallTo}"
        SDL_ARGS="${SDL_ARGS} --includedir=${InstallTo}/include"
        SDL_ARGS="${SDL_ARGS} --libdir=${InstallTo}/lib"
        BuildSrc $LatestSDL "${SDL_ARGS}"
    else
        #on Mac OS X build via X-Code
        cd $LatestSDL
            UNIVERSAL_OUTPUTFOLDER=$InstallTo/frameworks
            if [ -d $UNIVERSAL_OUTPUTFOLDER ]; then
                #Deletion of old builds
                rm -Rf $UNIVERSAL_OUTPUTFOLDER
            fi
            mkdir -p -- "$UNIVERSAL_OUTPUTFOLDER"

            xcodebuild -target Framework -project Xcode/SDL/SDL.xcodeproj -configuration Release BUILD_DIR="${InstallTo}/frameworks"

            if [ ! $? -eq 0 ]
            then
                errorofbuild
            fi

            #move out built framework from "Release" folder
            mv -f $InstallTo/frameworks/Release/SDL2.framework $InstallTo/frameworks/
            rm -Rf $InstallTo/frameworks/Release

            #make RIGHT headers organization in the SDL Framework
            mkdir -p -- ${InstallTo}/frameworks/SDL2.framework/Headers/SDL2
            mv ${InstallTo}/frameworks/SDL2.framework/Headers/*.h ${InstallTo}/frameworks/SDL2.framework/Headers/SDL2

        cd ..
    fi
}

BuildFluidSynth()
{
    CURRENT_TARBALL="FluidSynth"
    UnArch 'fluidsynth-1.1.6'
    printf "=========\E[37;42mFLUIDSYNTH\E[0m===========\n"
    #Build minimalistic FluidSynth version to just generate raw audio output to handle in the SDL Mixer X
    FLUID_ARGS="${FLUID_ARGS} --prefix=${InstallTo}"
    FLUID_ARGS="${FLUID_ARGS} --includedir=${InstallTo}/include"
    FLUID_ARGS="${FLUID_ARGS} --libdir=${InstallTo}/lib"
    FLUID_ARGS="${FLUID_ARGS} CFLAGS=-fPIC CXXFLAGS=-fPIC"
    FLUID_ARGS="${FLUID_ARGS} --disable-dbus-support"
    FLUID_ARGS="${FLUID_ARGS} --disable-pulse-support"
    FLUID_ARGS="${FLUID_ARGS} --disable-alsa-support"
    FLUID_ARGS="${FLUID_ARGS} --disable-portaudio-support"
    FLUID_ARGS="${FLUID_ARGS} --disable-oss-support"
    FLUID_ARGS="${FLUID_ARGS} --disable-jack-support"
    FLUID_ARGS="${FLUID_ARGS} --disable-midishare"
    FLUID_ARGS="${FLUID_ARGS} --disable-coreaudio"
    FLUID_ARGS="${FLUID_ARGS} --disable-coremidi"
    FLUID_ARGS="${FLUID_ARGS} --disable-dart"
    FLUID_ARGS="${FLUID_ARGS} --disable-lash"
    FLUID_ARGS="${FLUID_ARGS} --disable-ladcca"
    FLUID_ARGS="${FLUID_ARGS} --without-readline"
    FLUID_ARGS="${FLUID_ARGS} --enable-static=yes --enable-shared=no"
    BuildSrc 'fluidsynth-1.1.6' "${FLUID_ARGS}"
}

BuildLUAJIT()
{
    CURRENT_TARBALL="LuaJIT"
    UnArch 'luajit'

    ###########LuaJIT###########
    printf "=========\E[37;42mLuaJIT\E[0m===========\n"
    cd LuaJIT
    echo "Building..."
    make -s --jobs=2 PREFIX="${InstallTo}" BUILDMODE=static
    if [ $? -eq 0 ]
    then
        echo "[good]"
    else
        errorofbuild
    fi

    echo "Installing..."
    make -s install PREFIX="${InstallTo}" BUILDMODE=static
    if [ $? -eq 0 ]
    then
        echo "[good]"
    else
        errorofbuild
    fi

    if [[ "$OurOS" == "macos" ]]; then
        cp -a ./src/libluajit.a "${InstallTo}/lib/libluajit.a"
        cp -a ./src/libluajit.a "${InstallTo}/lib/libluajit-5.1.a"
    fi
    cd ..
}

BuildGLEW()
{
    CURRENT_TARBALL="GLEW"
    UnArch 'glew-2.0.0'

    ###########GLEW###########
    printf "=========\E[37;42mGLEW\E[0m===========\n"
    cd glew-2.0.0
    GLEW_ARGS="${GLEW_ARGS} GLEW_PREFIX=\"${InstallTo}\""
    GLEW_ARGS="${GLEW_ARGS} LIBDIR=\"${InstallTo}/lib\""
    GLEW_ARGS="${GLEW_ARGS} GLEW_DEST=\"${InstallTo}\""
    GLEW_ARGS="${GLEW_ARGS} GLEW_NO_GLU=\"-DGLEW_NO_GLU\""
    echo ------------------------------------------------------------
    echo make ${GLEW_ARGS} CFLAGS.EXTRA="-fPIC"
    echo ------------------------------------------------------------
    make ${GLEW_ARGS} CFLAGS.EXTRA="-fPIC"
    if [ $? -eq 0 ]
    then
        echo "[good]"
    else
        errorofbuild
    fi
        make install $GLEW_ARGS
        if [ $? -eq 0 ]
        then
            echo "[good]"
        else
            errorofbuild
        fi
    cd ..
}

########################Build & Install libraries##################################

# in-archives
if [ ! -d $CACHE_DIR ]
then
	mkdir $CACHE_DIR
fi
cd $CACHE_DIR

BuildLUAJIT
#BuildSampleRate
BuildSDL
#BuildFluidSynth
#BuildGLEW

echo Libraries installed into $InstallTo
