/*
 * Copyright (c) 2010-2015 Pivotal Software, Inc. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you
 * may not use this file except in compliance with the License. You
 * may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * permissions and limitations under the License. See accompanying
 * LICENSE file.
 */
/*
 * Changes for SnappyData data platform.
 *
 * Portions Copyright (c) 2018 SnappyData, Inc. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you
 * may not use this file except in compliance with the License. You
 * may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * permissions and limitations under the License. See accompanying
 * LICENSE file.
 */

/**
 * PropertyReader.h
 */

#ifndef PROPERTYREADER_H_
#define PROPERTYREADER_H_

#include "DriverBase.h"

namespace io {
namespace snappydata {
namespace impl {

  template<typename CHAR_TYPE>
  class PropertyReader {
  public:
    virtual void init(const CHAR_TYPE* inputRef, SQLLEN inputLen,
        void* propData) = 0;

    virtual SQLRETURN read(std::string& outPropName,
        std::string& outPropValue, SnappyHandleBase* handle) = 0;

    virtual const std::string& getDSN() const noexcept = 0;

    virtual ~PropertyReader() {
    }
  };

} /* namespace impl */
} /* namespace snappydata */
} /* namespace io */

#endif /* PROPERTYREADER_H_ */
