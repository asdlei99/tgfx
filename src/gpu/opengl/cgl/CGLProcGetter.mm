/////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Tencent is pleased to support the open source community by making tgfx available.
//
//  Copyright (C) 2023 Tencent. All rights reserved.
//
//  Licensed under the BSD 3-Clause License (the "License"); you may not use this file except
//  in compliance with the License. You may obtain a copy of the License at
//
//      https://opensource.org/licenses/BSD-3-Clause
//
//  unless required by applicable law or agreed to in writing, software distributed under the
//  license is distributed on an "as is" basis, without warranties or conditions of any kind,
//  either express or implied. see the license for the specific language governing permissions
//  and limitations under the license.
//
/////////////////////////////////////////////////////////////////////////////////////////////////

#include "CGLProcGetter.h"
#include <dlfcn.h>

namespace tgfx {
CGLProcGetter::CGLProcGetter() {
  fLibrary = dlopen("/System/Library/Frameworks/OpenGL.framework/Versions/A/Libraries/libGL.dylib",
                    RTLD_LAZY);
}

CGLProcGetter::~CGLProcGetter() {
  if (fLibrary) {
    dlclose(fLibrary);
  }
}

void* CGLProcGetter::getProcAddress(const char* name) const {
  auto handle = fLibrary ? fLibrary : RTLD_DEFAULT;
  return dlsym(handle, name);
}

std::unique_ptr<GLProcGetter> GLProcGetter::Make() {
  return std::make_unique<CGLProcGetter>();
}
}  // namespace tgfx
