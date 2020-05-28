/************************************************************************************

Filename    :   OVR_Types.h
Content     :   Standard library defines and simple types
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

#ifndef OVR_Compiler_Extra_h
#define OVR_Compiler_Extra_h

// -----------------------------------------------------------------------------------
// ***** OVR_NON_COPYABLE
//
// Allows you to specify a class as being neither copy-constructible nor assignable,
// which is a commonly needed pattern in C++ programming. Classes with this declaration
// are required to be default constructible (as are most classes). For pre-C++11
// compilers this macro declares a private section for the class, which will be
// inherited by whatever code is directly below the macro invocation by default.
//
// Example usage:
//    struct Test {
//       Test();
//       ...
//       OVR_NON_COPYABLE(Test)
//    };

#if !defined(OVR_NON_COPYABLE)
#if defined(OVR_CPP_NO_DELETED_FUNCTIONS)
#define OVR_NON_COPYABLE(Type) \
 private:                      \
  Type(const Type&);           \
  void operator=(const Type&);
#else
#define OVR_NON_COPYABLE(Type) \
  Type(const Type&) = delete;  \
  void operator=(const Type&) = delete;
#endif
#endif

#endif
