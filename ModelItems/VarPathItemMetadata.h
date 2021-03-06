//
// Copyright (c) 2010-2016, Fabric Software Inc. All rights reserved.
//

#ifndef FABRICUI_MODELITEMS_VARPATHITEMMETADATA_H
#define FABRICUI_MODELITEMS_VARPATHITEMMETADATA_H

#include "DFGModelItemMetadata.h"

#include <FabricUI/ModelItems/VarPathModelItem.h>

namespace FabricUI {
namespace ModelItems {

class VarPathItemMetadata : public DFGModelItemMetadata
{
public:

  VarPathItemMetadata( VarPathModelItem *varPathModelItem ) {}

  virtual const char* getString( const char* key ) const /*override*/
  {
    if ( key == VENotInspectableKey )
    {
      return FTL_STR("").c_str();
    }

    if ( key == VEPortTypeKey )
    {
      return FTL_STR("In").c_str();
    }

    return 0;
  }
};

} // namespace ModelItems
} // namespace FabricUI

#endif // FABRICUI_MODELITEMS_VARPATHITEMMETADATA_H
