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

#pragma once

#include "gpu/GPUBuffer.h"

namespace tgfx {
class GLBuffer : public GPUBuffer {
 public:
  unsigned bufferID() const {
    return _bufferID;
  }

 protected:
  void onReleaseGPU() override;

 private:
  unsigned _bufferID = 0;

  GLBuffer(BufferType bufferType, size_t size, unsigned bufferID)
      : GPUBuffer(bufferType, size), _bufferID(bufferID) {
  }

  friend class GPUBuffer;
};
}  // namespace tgfx
