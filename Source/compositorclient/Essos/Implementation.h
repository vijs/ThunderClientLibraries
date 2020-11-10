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

#pragma once

#include <algorithm>
#include <cassert>
#include <list>
#include <string>
#include <string.h>

#define EGL_EGLEXT_PROTOTYPES 1

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <EGL/eglext.h>

//#include <../core/core.h>
#include "essos.h"

#define TR() printf("%s:%d\n", __FUNCTION__, __LINE__);

namespace WPEFramework {
namespace Wayland {

    class Display : public Compositor::IDisplay {
    public:
        typedef std::map<const std::string, Display*> DisplayMap;
        struct ICallback {
            virtual ~ICallback() {}
            virtual void Attached(const uint32_t id) = 0;
            virtual void Detached(const uint32_t id) = 0;
        };

    private:
        Display() = delete;
        Display(const Display&) = delete;
        Display& operator=(const Display&) = delete;

        class CriticalSection {
        private:
            CriticalSection(const CriticalSection&);
            CriticalSection& operator=(const CriticalSection&);

        public:
            CriticalSection()
            {

                pthread_mutexattr_t structAttributes;

                // Create a recursive mutex for this process (no named version, use semaphore)
                if ((pthread_mutexattr_init(&structAttributes) != 0) || (pthread_mutexattr_settype(&structAttributes, PTHREAD_MUTEX_RECURSIVE) != 0) || (pthread_mutex_init(&_lock, &structAttributes) != 0)) {
                    // That will be the day, if this fails...
                    assert(false);
                }
            }
            ~CriticalSection()
            {
            }

        public:
            void Lock()
            {
                pthread_mutex_lock(&_lock);
            }
            void Unlock()
            {
                pthread_mutex_unlock(&_lock);
            }

        private:
            pthread_mutex_t _lock;
        };


        class SurfaceImplementation : public Compositor::IDisplay::ISurface {
        private:
            SurfaceImplementation() = delete;
            SurfaceImplementation(const SurfaceImplementation&) = delete;
            SurfaceImplementation& operator=(const SurfaceImplementation&) = delete;

        public:
            SurfaceImplementation(Display& display, const std::string& name, const uint32_t width, const uint32_t height)
                : _parent(display)
                , _refcount(1)
                , _name(name)
                , _width(width)
                , _height(height)
                , _nativeWindow(nullptr)
                , _keyboard(nullptr)
            {


               _parent.Register(this);
            }

            virtual ~SurfaceImplementation()
            {
                _parent.Unregister(this);
            }

        public:
            virtual void AddRef() const override
            {
                _refcount++;
            }
            virtual uint32_t Release() const override
            {
                if (--_refcount == 0) {
                    delete const_cast<SurfaceImplementation*>(this);
                }
                return (0);
            }
            virtual EGLNativeWindowType Native() const override
            {
                return (static_cast<EGLNativeWindowType>(_nativeWindow));
            }
            virtual std::string Name() const override
            {
                return _name;
            }
            virtual int32_t Height() const override
            {
                return (_height);
            }
            virtual int32_t Width() const override
            {
                return (_width);
            }
            inline uint32_t Id() const
            {
                return (_id);
            }
            inline void Position(const uint32_t X, const uint32_t Y, const uint32_t height, const uint32_t width) {}
            void Unlink() {}
            void Resize(const int x, const int y, const int width, const int height) {}
            //void Callback(wl_callback_listener* listener, void* data) {}
            void Visibility(const bool visible) {}
            void Opacity(const uint32_t opacity) {}
            void ZOrder(const uint32_t order) {}
            void BringToFront() {}
            inline const bool UpScale() const
            {
                return _upScale;
            }

            virtual void Keyboard(Compositor::IDisplay::IKeyboard* keyboard) override
            {
                assert((_keyboard == nullptr) ^ (keyboard == nullptr));
                _keyboard = keyboard;
            }
            inline void SendKey(const uint32_t key, const IKeyboard::state action, const uint32_t time)
            {

                if (_keyboard != nullptr) {
                    _keyboard->Direct(key, action);
                }
            }

        private:
            Display& _parent;
            mutable uint32_t _refcount;
            uint32_t _id;
            std::string _name;
            int32_t _width;
            int32_t _height;
            EGLSurface _nativeWindow;
            IKeyboard* _keyboard;
            const bool _upScale = false;
        };

        public:
            struct IProcess {
                virtual ~IProcess() {}

                virtual bool Dispatch() = 0;
            };

        class Surface {
        public:
            inline Surface()
                : _implementation(nullptr)
            {
            }
            inline Surface(SurfaceImplementation& impl)
                : _implementation(&impl)
            {
                _implementation->AddRef();
            }
            inline Surface(const Surface& copy)
                : _implementation(copy._implementation)
            {
                if (_implementation != nullptr) {
                    _implementation->AddRef();
                }
            }
            inline ~Surface()
            {
                if (_implementation != nullptr) {
                    _implementation->Release();
                }
            }

            inline Surface& operator=(const Surface& rhs)
            {
                if (_implementation != nullptr) {
                    _implementation->Release();
                }
                _implementation = rhs._implementation;
                if (_implementation != nullptr) {
                    _implementation->AddRef();
                }
                return (*this);
            }

        public:
            inline bool IsValid() const
            {
                return (_implementation != nullptr);
            }
            inline uint32_t Id() const
            {
                assert(IsValid() == true);
                return (_implementation->Id());
            }
            inline const std::string Name() const
            {
                assert(IsValid() == true);
                return (_implementation->Name());
            }
            inline uint32_t Height() const
            {
                assert(IsValid() == true);
                return (_implementation->Height());
            }
            inline uint32_t Width() const
            {
                assert(IsValid() == true);
                return (_implementation->Width());
            }
            inline void Visibility(const bool visible)
            {
                assert(IsValid() == true);
                return (_implementation->Visibility(visible));
            }
            inline void Opacity(const uint32_t opacity)
            {
                assert(IsValid() == true);
                return (_implementation->Opacity(opacity));
            }
            inline void ZOrder(const uint32_t order)
            {
                assert(IsValid() == true);
                return (_implementation->ZOrder(order));
            }
            inline const bool UpScale()
            {
                assert(IsValid() == true);
                return (_implementation->UpScale());
            }

            inline void Position(const uint32_t X, const uint32_t Y, const uint32_t height, const uint32_t width)
            {
                assert(IsValid() == true);
                _implementation->Position(X, Y, height, width);
            }
            inline void Keyboard(IKeyboard* keyboard)
            {
                assert(IsValid() == true);
                return (_implementation->Keyboard(keyboard));
            }
            inline void Pointer(IPointer* pointer)
            {
                assert(IsValid() == true);
                return (_implementation->Pointer(pointer));
            }
            inline void AddRef()
            {
                if (_implementation != nullptr) {
                    _implementation->AddRef();
                    _implementation = nullptr;
                }
            }
            inline void Release()
            {
                if (_implementation != nullptr) {
                    _implementation->Release();
                    _implementation = nullptr;
                }
            }
            //inline void Callback(wl_callback_listener* listener, void* data = nullptr)
            //{
            //    assert(IsValid() == true);
            //    _implementation->Callback(listener, data);
            //}
            inline void Resize(const int x, const int y, const int width, const int height)
            {
                assert(IsValid() == true);
                _implementation->Resize(x, y, width, height);
            }
            inline void BringToFront()
            {
                assert(IsValid() == true);
                _implementation->BringToFront();
            }
            inline EGLNativeWindowType Native() const
            {
                assert(IsValid() == true);
                return (_implementation->Native());
            }
            inline void Unlink()
            {
                assert(IsValid() == true);
                return _implementation->Unlink();
            }

        private:
            SurfaceImplementation* _implementation;
        };

    private:
        Display(const std::string& name);
        void Initialize();
        void Deinitialize();

    public:
        virtual ~Display();
        static Display& Instance(const std::string& displayName);


    public:
        // Lifetime management
        virtual void AddRef() const;
        virtual uint32_t Release() const;


        // IDisplay Methods
        EGLNativeDisplayType Native() const override
        { TR();
            return (static_cast<EGLNativeDisplayType>(EGL_DEFAULT_DISPLAY));
        }

        const std::string& Name() const override
        { TR();
            return (_displayName);
        }

        int Process(const uint32_t data) override;
        virtual ISurface* Create(const std::string& name, const uint32_t width, const uint32_t height) override
        { TR();
            return (new SurfaceImplementation(*this, name, width, height));
        }
        virtual int FileDescriptor() const override
        { TR();
            return  0;
        }

        //
        inline void Callback(ICallback* callback) const
        { TR();
        }

        void Get(const uint32_t id, Surface& surface)
        { TR();
        }
        void LoadSurfaces()
        { TR();
        }
        void Process(IProcess* processLoop);
        void Signal()
        { TR();
        }


    private:
        inline void Register(SurfaceImplementation* surface)
        {
            std::list<SurfaceImplementation*>::iterator index(std::find(_surfaces.begin(), _surfaces.end(), surface));

            if (index == _surfaces.end()) {
                _surfaces.push_back(surface);
            }
        }
        inline void Unregister(SurfaceImplementation* surface)
        {
            std::list<SurfaceImplementation*>::iterator index(std::find(_surfaces.begin(), _surfaces.end(), surface));

            if (index != _surfaces.end()) {
                _surfaces.erase(index);
            }
        }

    private:
        const std::string _displayName;
        EssCtx* _essCtx;
        std::list<SurfaceImplementation*> _surfaces;


        mutable uint32_t _refCount;

        // Process wide singleton
        static CriticalSection _adminLock;
        static std::string _runtimeDir;
        static DisplayMap _displays;
    }; // Display

} // Wayland
} // WPEFramework
