
#include "CustomJSONDataHooks.h"

#include "HookUtils.hpp"
#include "CustomBeatmapData.h"

#include "BeatmapSaveDataVersion2_6_0AndEarlier/zzzz__EventData_def.hpp"
#include "BeatmapSaveDataVersion2_6_0AndEarlier/BeatmapSaveDataItem.hpp"
#include "BeatmapDataLoaderVersion2_6_0AndEarlier/BeatmapDataLoader.hpp"

#include "BeatmapDataLoaderVersion3/BeatmapDataLoader.hpp"
#include "BeatmapSaveDataVersion3/BeatmapSaveDataItem.hpp"

#include "BeatmapDataLoaderVersion4/BeatmapDataLoader.hpp"
#include "BeatmapSaveDataVersion4/LightshowSaveData.hpp"

#include "GlobalNamespace/BpmTimeProcessor.hpp"
#include "GlobalNamespace/EnvironmentKeywords.hpp"
#include "GlobalNamespace/IEnvironmentInfo.hpp"
#include "GlobalNamespace/IEnvironmentLightGroups.hpp"
#include "GlobalNamespace/EnvironmentLightGroups.hpp"
#include "GlobalNamespace/DefaultEnvironmentEvents.hpp"
#include "GlobalNamespace/BeatmapObjectData.hpp"
#include "GlobalNamespace/NoteData.hpp"
#include "GlobalNamespace/BeatmapDataSortedListForTypeAndIds_1.hpp"
#include "GlobalNamespace/BasicBeatmapEventDataProcessor.hpp"
#include "GlobalNamespace/BeatmapDataStrobeFilterTransform.hpp"
#include "GlobalNamespace/LightColorBeatmapEventData.hpp"
#include "GlobalNamespace/EnvironmentIntensityReductionOptions.hpp"
#include "GlobalNamespace/CallbacksInTime.hpp"
#include "GlobalNamespace/IReadonlyBeatmapData.hpp"
#include "GlobalNamespace/BeatmapEventDataLightsExtensions.hpp"

#include "System/Action.hpp"

#include "UnityEngine/JsonUtility.hpp"

#include "System/Reflection/MemberInfo.hpp"
#include "System/Collections/Generic/InsertionBehavior.hpp"

#include "beatsaber-hook/shared/utils/typedefs-list.hpp"
#include "beatsaber-hook/shared/utils/hooking.hpp"
#include "beatsaber-hook/shared/config/rapidjson-utils.hpp"
// for rapidjson error parsing
#include "beatsaber-hook/shared/rapidjson/include/rapidjson/error/en.h"

#include "cpp-semver/shared/cpp-semver.hpp"

#include "paper/shared/Profiler.hpp"

#include "sombrero/shared/linq_functional.hpp"
#include "sombrero/shared/Vector2Utils.hpp"
#include "sombrero/shared/Vector3Utils.hpp"

#include "custom-types/shared/register.hpp"

#include "songcore/shared/CustomJSONData.hpp"

#include "JSONWrapper.h"
#include "CustomBeatmapSaveDatav2.h"
#include "CustomBeatmapSaveDatav3.h"
#include "CustomBeatmapData.h"
#include "BeatmapFieldUtils.hpp"
#include "BeatmapDataLoaderUtils.hpp"
#include "CustomJSONDataHooks.h"
#include "CJDLogger.h"
#include "VList.h"

#include <regex>
#include <chrono>
#include <codecvt>
#include <locale>

using namespace System;
using namespace System::Collections::Generic;
using namespace GlobalNamespace;
using namespace CustomJSONData;
using namespace BeatmapSaveDataVersion3;
using namespace BeatmapSaveDataVersion4;

MAKE_HOOK_FIND_INSTANCE(CustomBeatmapDataSortedListForTypes_InsertItem,
                        classof(BeatmapDataSortedListForTypeAndIds_1<BeatmapDataItem*>*), "InsertItem",
                        System::Collections::Generic::LinkedListNode_1<BeatmapDataItem*>*,
                        BeatmapDataSortedListForTypeAndIds_1<BeatmapDataItem*>* self, BeatmapDataItem* item) {
  auto* list = self->GetList(CustomBeatmapData::GetCustomType(item), item->get_subtypeGroupIdentifier());

  auto* node = list->Insert(item);
  // Remove to avoid exception
  self->_itemToNodeMap->TryInsert(item, node, InsertionBehavior::OverwriteExisting);

  return node;
}

MAKE_HOOK_FIND_INSTANCE(CustomBeatmapDataSortedListForTypes_RemoveItem,
                        classof(BeatmapDataSortedListForTypeAndIds_1<BeatmapDataItem*>*), "RemoveItem", void,
                        BeatmapDataSortedListForTypeAndIds_1<BeatmapDataItem*>* self, BeatmapDataItem* item) {
  auto* list = self->GetList(CustomBeatmapData::GetCustomType(item), item->get_subtypeGroupIdentifier());
  System::Collections::Generic::LinkedListNode_1<BeatmapDataItem*>* node = nullptr;
  if (self->_itemToNodeMap->TryGetValue(item, byref(node))) {
    list->Remove(node);
  }
}

MAKE_PAPER_HOOK_MATCH(BeatmapData_GetCopy, &CustomBeatmapData::GetCopy, BeatmapData*, BeatmapData* self) {
  static auto* CustomBeatmapDataKlass = classof(CustomBeatmapData*);

  if (self->klass == CustomBeatmapDataKlass) {
    return reinterpret_cast<CustomBeatmapData*>(self)->GetCopyOverride();
  }

  return BeatmapData_GetCopy(self);
}

MAKE_PAPER_HOOK_MATCH(
    BeatmapData_GetFilteredCopy, &CustomBeatmapData::GetFilteredCopy, BeatmapData*, BeatmapData* self,
    System::Func_2<::GlobalNamespace::BeatmapDataItem*, ::GlobalNamespace::BeatmapDataItem*>* processDataItem) {
  static auto* CustomBeatmapDataKlass = classof(CustomBeatmapData*);

  if (self->klass == CustomBeatmapDataKlass) {
    return reinterpret_cast<CustomBeatmapData*>(self)->GetFilteredCopyOverride(
        [&](auto i) constexpr { return processDataItem->Invoke(i); });
  }

  return BeatmapData_GetFilteredCopy(self, processDataItem);
}

void CustomJSONData::InstallBeatmapHooks() {
  INSTALL_HOOK_ORIG(CJDLogger::Logger, CustomBeatmapDataSortedListForTypes_InsertItem);
  INSTALL_HOOK_ORIG(CJDLogger::Logger, CustomBeatmapDataSortedListForTypes_RemoveItem);
  INSTALL_HOOK_ORIG(CJDLogger::Logger, BeatmapData_GetFilteredCopy);
  INSTALL_HOOK_ORIG(CJDLogger::Logger, BeatmapData_GetCopy);
}