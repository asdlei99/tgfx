/////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Tencent is pleased to support the open source community by making tgfx available.
//
//  Copyright (C) 2024 Tencent. All rights reserved.
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

#include "CodecImage.h"
#include <memory>

namespace tgfx {
CodecImage::CodecImage(UniqueKey uniqueKey, std::shared_ptr<ImageCodec> codec)
    : GeneratorImage(std::move(uniqueKey), std::move(codec)) {
}

std::shared_ptr<ImageCodec> CodecImage::getCodec() const {
  return std::static_pointer_cast<ImageCodec>(generator);
}

}  // namespace tgfx