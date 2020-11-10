/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "core.h"
#include "../Client.h"

#include "Implementation.h"
#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#define Trace(fmt, args...) //fprintf(stderr, "[pid=%d][Client %s:%d] : " fmt, getpid(), __FILENAME__, __LINE__, ##args)
#define Debug(fmt, args...) fprintf(stderr, "[pid=%d][Client %s:%d] : " fmt, getpid(), __FILENAME__, __LINE__, ##args)

namespace WPEFramework {
namespace Wayland {
    /*static*/ Display::CriticalSection Display::_adminLock;
    /*static*/ std::string Display::_runtimeDir;
    /*static*/ Display::DisplayMap Display::_displays;

    static int gDisplayWidth, gDisplayHeight;

    static void terminatedCb( void * )
    {
        Debug("%s event\n", __FUNCTION__);
    }

    static EssTerminateListener terminateListener =
    {
       terminatedCb
    };

    static void displaySizeCb( void* userData, int width, int height )
    {
        EssCtx* ctx = (EssCtx*) userData;    // static_cast

        Debug("%s event\n", __FUNCTION__);

        if ( (gDisplayWidth != width) || (gDisplayHeight != height) )
        {
            Debug("display size changed: %dx%d\n", width, height);

            gDisplayWidth = width;
            gDisplayHeight = height;

            EssContextResizeWindow( ctx, width, height );
        }
    }

    static EssSettingsListener settingsListener =
    {
       displaySizeCb
    };

    static void keyPressedCb ( void*, unsigned int key )
    {
        Debug("%s event\n", __FUNCTION__);
    }

    static void keyReleasedCb( void*, unsigned int )
    {
        Debug("%s event\n", __FUNCTION__);
    }

    static EssKeyListener keyListener =
    {
       keyPressedCb,
       keyReleasedCb
    };



    Display::Display(const std::string& name)
        : _displayName(name)
        , _refCount(0)
    { TR();
    }

    Display::~Display()
    {
        ASSERT(_refCount == 0);
        DisplayMap::iterator index(_displays.find(_displayName));

        if (index != _displays.end()) {
            _displays.erase(index);
        }
    }

    void Display::Initialize()
    {
        bool success = false;
        bool useWayland = true;

        if (_displayName.compare("wayland-0") == 0) {
            Debug("Ignoring Display Name %s\n", _displayName.c_str() );
            return;
        }

        if (_runtimeDir.empty() == true) {
            const char* val = ::getenv("XDG_RUNTIME_DIR");
            if (val != nullptr)
                _runtimeDir = val;
        }
        const char* val = ::getenv("ESSOS_USE_WAYLAND");
        if (val != nullptr)
            useWayland = atoi(val);

        Debug("Initializing Wayland Display Name %s at %s useWayland=%d\n", _displayName.c_str(), _runtimeDir.c_str(), useWayland);

        _essCtx = EssContextCreate();
        if ( _essCtx ) {
            if (!EssContextSetUseWayland( _essCtx, useWayland ))
                Trace("EssContextSetUseWayland Failed\n");

            if ( !EssContextSetTerminateListener( _essCtx, 0, &terminateListener ))
                Trace("EssContextSetTerminateListener Failed\n");

            if (!EssContextSetSettingsListener( _essCtx, _essCtx, &settingsListener ))
                Trace("EssContextSetSettingsListener Failed\n");

            if (!EssContextSetKeyListener( _essCtx, 0, &keyListener ))
                Trace("EssContextSetKeyListener Failed\n");

            Debug("Essos Starting\n");
            if ( EssContextStart( _essCtx ) ) {
                Debug("Essos started\n");
                success = true;
            } else {
                Debug("Error Starting Essos\n");
            }
        }

        if ( !success )
        {
            const char *detail= EssContextGetLastErrorDetail( _essCtx );
            Debug("Essos error: %s\n", detail);
        }
    }

    void Display::Deinitialize()
    { TR();
        EssContextDestroy( _essCtx );
        _essCtx = nullptr;
    }

    int Display::Process(const uint32_t data)
    {
        signed int result(0);

        _adminLock.Lock();
        EssContextRunEventLoopOnce(_essCtx);
        _adminLock.Unlock();

        return result;
    }

    void Display::Process(IProcess* processLoop)
    {
        Trace("--> Entering %s \n", __FUNCTION__);
        while (processLoop->Dispatch()) {
            if (_essCtx) {
                EssContextRunEventLoopOnce(_essCtx);
            }
            //EssContextUpdateDisplay(_essCtx);
        }
        Trace("<-- Exiting %s\n", __FUNCTION__);
    }

    // IDisplay
    /* static */ Display& Display::Instance(const std::string& displayName)
    { TR();
        Display* result(nullptr);

        _adminLock.Lock();

        DisplayMap::iterator index(_displays.find(displayName));

        if (index == _displays.end()) {
            result = new Display(displayName);
            _displays.insert(std::pair<const std::string, Display*>(displayName, result));
        } else {
            result = index->second;
        }
        result->AddRef();
        _adminLock.Unlock();

        assert(result != nullptr);

        return (*result);
    }

    void Display::AddRef() const
    {
        if (Core::InterlockedIncrement(_refCount) == 1) {
            const_cast<Display*>(this)->Initialize();
        }
        return;
    }

    uint32_t Display::Release() const
    {
        if (Core::InterlockedDecrement(_refCount) == 0) {
            const_cast<Display*>(this)->Deinitialize();

            //Indicate Wayland connection is closed properly
            return (Core::ERROR_CONNECTION_CLOSED);
        }
        return (Core::ERROR_NONE);
    }

} // Wayland

/* static */ Compositor::IDisplay* Compositor::IDisplay::Instance(const std::string& displayName)
{
    return (&(Wayland::Display::Instance(displayName)));
}

} // WPEFramework
