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

#include "Quad.h"
#include "tgfx/core/Buffer.h"

namespace tgfx {
Quad Quad::MakeFrom(const Rect& rect, const Matrix* matrix) {
  return Quad(rect, matrix);
}

Quad::Quad(const Rect& rect, const Matrix* matrix) {
  points[0] = Point::Make(rect.left, rect.top);
  points[1] = Point::Make(rect.left, rect.bottom);
  points[2] = Point::Make(rect.right, rect.top);
  points[3] = Point::Make(rect.right, rect.bottom);
  if (matrix) {
    matrix->mapPoints(points, 4);
  }
}
}  // namespace tgfx
