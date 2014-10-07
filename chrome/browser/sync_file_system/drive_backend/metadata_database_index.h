// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SYNC_FILE_SYSTEM_DRIVE_BACKEND_METADATA_DATABASE_INDEX_H_
#define CHROME_BROWSER_SYNC_FILE_SYSTEM_DRIVE_BACKEND_METADATA_DATABASE_INDEX_H_

#include <map>
#include <set>
#include <string>
#include <vector>

#include "base/containers/hash_tables.h"
#include "base/containers/scoped_ptr_hash_map.h"
#include "base/memory/scoped_vector.h"
#include "chrome/browser/sync_file_system/drive_backend/metadata_database_index_interface.h"
#include "chrome/browser/sync_file_system/drive_backend/tracker_id_set.h"

namespace sync_file_system {
namespace drive_backend {

class FileMetadata;
class FileTracker;
class LevelDBWrapper;
class ServiceMetadata;

}  // namespace drive_backend
}  // namespace sync_file_system

namespace BASE_HASH_NAMESPACE {

#if defined(COMPILER_GCC)
template<> struct hash<sync_file_system::drive_backend::ParentIDAndTitle> {
  std::size_t operator()(
      const sync_file_system::drive_backend::ParentIDAndTitle& v) const {
    return base::HashInts64(v.parent_id, hash<std::string>()(v.title));
  }
};
#elif defined(COMPILER_MSVC)
inline size_t hash_value(
    const sync_file_system::drive_backend::ParentIDAndTitle& v) {
  return base::HashInts64(v.parent_id, hash_value(v.title));
}
#endif  // COMPILER

}  // namespace BASE_HASH_NAMESPACE

namespace sync_file_system {
namespace drive_backend {

struct DatabaseContents {
  DatabaseContents();
  ~DatabaseContents();
  ScopedVector<FileMetadata> file_metadata;
  ScopedVector<FileTracker> file_trackers;
};

// Maintains indexes of MetadataDatabase on memory.
class MetadataDatabaseIndex : public MetadataDatabaseIndexInterface {
 public:
  virtual ~MetadataDatabaseIndex();

  static scoped_ptr<MetadataDatabaseIndex> Create(LevelDBWrapper* db);
  static scoped_ptr<MetadataDatabaseIndex> CreateForTesting(
      DatabaseContents* contents, LevelDBWrapper* db);

  // MetadataDatabaseIndexInterface overrides.
  virtual bool GetFileMetadata(
      const std::string& file_id, FileMetadata* metadata) const override;
  virtual bool GetFileTracker(
      int64 tracker_id, FileTracker* tracker) const override;
  virtual void StoreFileMetadata(scoped_ptr<FileMetadata> metadata) override;
  virtual void StoreFileTracker(scoped_ptr<FileTracker> tracker) override;
  virtual void RemoveFileMetadata(const std::string& file_id) override;
  virtual void RemoveFileTracker(int64 tracker_id) override;
  virtual TrackerIDSet GetFileTrackerIDsByFileID(
      const std::string& file_id) const override;
  virtual int64 GetAppRootTracker(const std::string& app_id) const override;
  virtual TrackerIDSet GetFileTrackerIDsByParentAndTitle(
      int64 parent_tracker_id,
      const std::string& title) const override;
  virtual std::vector<int64> GetFileTrackerIDsByParent(
      int64 parent_tracker_id) const override;
  virtual std::string PickMultiTrackerFileID() const override;
  virtual ParentIDAndTitle PickMultiBackingFilePath() const override;
  virtual int64 PickDirtyTracker() const override;
  virtual void DemoteDirtyTracker(int64 tracker_id) override;
  virtual bool HasDemotedDirtyTracker() const override;
  virtual bool IsDemotedDirtyTracker(int64 tracker_id) const override;
  virtual void PromoteDemotedDirtyTracker(int64 tracker_id) override;
  virtual bool PromoteDemotedDirtyTrackers() override;
  virtual size_t CountDirtyTracker() const override;
  virtual size_t CountFileMetadata() const override;
  virtual size_t CountFileTracker() const override;
  virtual void SetSyncRootTrackerID(int64 sync_root_id) const override;
  virtual void SetLargestChangeID(int64 largest_change_id) const override;
  virtual void SetNextTrackerID(int64 next_tracker_id) const override;
  virtual int64 GetSyncRootTrackerID() const override;
  virtual int64 GetLargestChangeID() const override;
  virtual int64 GetNextTrackerID() const override;
  virtual std::vector<std::string> GetRegisteredAppIDs() const override;
  virtual std::vector<int64> GetAllTrackerIDs() const override;
  virtual std::vector<std::string> GetAllMetadataIDs() const override;

 private:
  typedef base::ScopedPtrHashMap<std::string, FileMetadata> MetadataByID;
  typedef base::ScopedPtrHashMap<int64, FileTracker> TrackerByID;
  typedef base::hash_map<std::string, TrackerIDSet> TrackerIDsByFileID;
  typedef base::hash_map<std::string, TrackerIDSet> TrackerIDsByTitle;
  typedef std::map<int64, TrackerIDsByTitle> TrackerIDsByParentAndTitle;
  typedef base::hash_map<std::string, int64> TrackerIDByAppID;
  typedef base::hash_set<std::string> FileIDSet;
  typedef base::hash_set<ParentIDAndTitle> PathSet;
  typedef std::set<int64> DirtyTrackers;

  friend class MetadataDatabaseTest;

  explicit MetadataDatabaseIndex(LevelDBWrapper* db);
  void Initialize(scoped_ptr<ServiceMetadata> service_metadata,
                  DatabaseContents* contents);

  // Maintains |app_root_by_app_id_|.
  void AddToAppIDIndex(const FileTracker& new_tracker);
  void UpdateInAppIDIndex(const FileTracker& old_tracker,
                          const FileTracker& new_tracker);
  void RemoveFromAppIDIndex(const FileTracker& tracker);

  // Maintains |trackers_by_file_id_| and |multi_tracker_file_ids_|.
  void AddToFileIDIndexes(const FileTracker& new_tracker);
  void UpdateInFileIDIndexes(const FileTracker& old_tracker,
                             const FileTracker& new_tracker);
  void RemoveFromFileIDIndexes(const FileTracker& tracker);

  // Maintains |trackers_by_parent_and_title_| and |multi_backing_file_paths_|.
  void AddToPathIndexes(const FileTracker& new_tracker);
  void UpdateInPathIndexes(const FileTracker& old_tracker,
                           const FileTracker& new_tracker1);
  void RemoveFromPathIndexes(const FileTracker& tracker);

  // Maintains |dirty_trackers_| and |demoted_dirty_trackers_|.
  void AddToDirtyTrackerIndexes(const FileTracker& new_tracker);
  void UpdateInDirtyTrackerIndexes(const FileTracker& old_tracker,
                                   const FileTracker& new_tracker);
  void RemoveFromDirtyTrackerIndexes(const FileTracker& tracker);

  scoped_ptr<ServiceMetadata> service_metadata_;
  LevelDBWrapper* db_;  // Not owned

  MetadataByID metadata_by_id_;
  TrackerByID tracker_by_id_;

  TrackerIDByAppID app_root_by_app_id_;

  TrackerIDsByFileID trackers_by_file_id_;
  FileIDSet multi_tracker_file_ids_;

  TrackerIDsByParentAndTitle trackers_by_parent_and_title_;
  PathSet multi_backing_file_paths_;

  DirtyTrackers dirty_trackers_;
  DirtyTrackers demoted_dirty_trackers_;

  DISALLOW_COPY_AND_ASSIGN(MetadataDatabaseIndex);
};

}  // namespace drive_backend
}  // namespace sync_file_system

#endif  // CHROME_BROWSER_SYNC_FILE_SYSTEM_DRIVE_BACKEND_METADATA_DATABASE_INDEX_H_
