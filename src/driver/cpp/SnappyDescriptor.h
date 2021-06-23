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
 * SnappyDescriptor.hpp
 */

#ifndef SNAPPYDESCRIPTOR_H_
#define SNAPPYDESCRIPTOR_H_

#include "DriverBase.h"

namespace io {
namespace snappydata {

  /**
   * Contains the ODBC descriptor handles for
   * Application Parameter Descriptor(APD),
   * Implementation Parameter Descriptor(IPD),
   * Application Row Descriptor(ARD),
   * Implementation Row Descriptor(IPD).
   * Encapsulates a JDBC parameter metadata and the resultsetmetadata.
   */
  class SnappyDescriptor final : public SnappyHandleBase {
  private:
    int m_descType;

    /** let SnappyStatement access the private fields */
    friend class SnappyStatement;

  public:

    SnappyDescriptor(int descType);

    ~SnappyDescriptor();
  };

} /* namespace snappydata */
} /* namespace io */

#endif /* SNAPPYDESCRIPTOR_H_ */
