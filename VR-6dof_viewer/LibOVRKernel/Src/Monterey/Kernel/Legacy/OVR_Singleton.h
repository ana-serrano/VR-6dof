/************************************************************************************

PublicHeader:   OVR
Filename    :   OVR_System.h
Content     :   General kernel initialization/cleanup, including that
                of the memory allocator.
Created     :   September 19, 2012
Notes       :

Copyright   :   Copyright 2014-2016 Oculus VR, LLC All Rights reserved.

Licensed under the Oculus VR Rift SDK License Version 3.3 (the "License");
you may not use the Oculus VR Rift SDK except in compliance with the License,
which is provided at the time of installation or download, or which
otherwise accompanies this software in either electronic or hard copy form.

You may obtain a copy of the License at

http://www.oculusvr.com/licenses/LICENSE-3.3

Unless required by applicable law or agreed to in writing, the Oculus VR SDK
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

************************************************************************************/

#ifndef OVR_Singleton_h

#include "../OVR_Allocator.h"
#include "../OVR_Log.h"
#include "../OVR_Atomic.h"

#include <atomic>

namespace OVR {

//-----------------------------------------------------------------------------
// Lightweight version of PC code.
template <class T>
class SystemSingletonBase {
 public:
  // TODO: implement a more self controlled singleton based on thread lifecycle
  virtual void OnThreadDestroy() {}

  // must use GetInstance to access the global singleton instance
  static T* GetInstance() {
    // A internal singleton instance used globally
    static T SingletonInstance;
    return &SingletonInstance;
  }

  bool IsInitialized() const {
    return Initialized;
  }

  virtual void Shutdown() = 0;

 protected:
  bool Initialized = false;
};

} // namespace OVR

#endif
