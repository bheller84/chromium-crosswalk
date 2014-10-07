// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SYNC_FILE_SYSTEM_DRIVE_BACKEND_SYNC_WORKER_H_
#define CHROME_BROWSER_SYNC_FILE_SYSTEM_DRIVE_BACKEND_SYNC_WORKER_H_

#include <string>

#include "base/files/file_path.h"
#include "base/memory/scoped_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "chrome/browser/sync_file_system/drive_backend/sync_task_manager.h"
#include "chrome/browser/sync_file_system/drive_backend/sync_worker_interface.h"
#include "chrome/browser/sync_file_system/remote_file_sync_service.h"
#include "chrome/browser/sync_file_system/sync_callbacks.h"
#include "chrome/browser/sync_file_system/task_logger.h"

class ExtensionServiceInterface;
class GURL;

namespace base {
class ListValue;
}

namespace drive {
class DriveServiceInterface;
class DriveUploaderInterface;
}

namespace storage {
class FileSystemURL;
}

namespace leveldb {
class Env;
}

namespace sync_file_system {

class FileChange;
class SyncFileMetadata;

namespace drive_backend {

class LocalToRemoteSyncer;
class MetadataDatabase;
class RemoteChangeProcessorOnWorker;
class RemoteToLocalSyncer;
class SyncEngineContext;
class SyncEngineInitializer;

class SyncWorker : public SyncWorkerInterface,
                   public SyncTaskManager::Client {
 public:
  SyncWorker(const base::FilePath& base_dir,
             const base::WeakPtr<ExtensionServiceInterface>& extension_service,
             leveldb::Env* env_override);

  virtual ~SyncWorker();

  virtual void Initialize(scoped_ptr<SyncEngineContext> context) override;

  // SyncTaskManager::Client overrides
  virtual void MaybeScheduleNextTask() override;
  virtual void NotifyLastOperationStatus(
      SyncStatusCode sync_status, bool used_network) override;
  virtual void RecordTaskLog(scoped_ptr<TaskLogger::TaskLog> task_log) override;

  // SyncWorkerInterface overrides
  virtual void RegisterOrigin(const GURL& origin,
                              const SyncStatusCallback& callback) override;
  virtual void EnableOrigin(const GURL& origin,
                            const SyncStatusCallback& callback) override;
  virtual void DisableOrigin(const GURL& origin,
                             const SyncStatusCallback& callback) override;
  virtual void UninstallOrigin(const GURL& origin,
                               RemoteFileSyncService::UninstallFlag flag,
                               const SyncStatusCallback& callback) override;
  virtual void ProcessRemoteChange(const SyncFileCallback& callback) override;
  virtual void SetRemoteChangeProcessor(
      RemoteChangeProcessorOnWorker* remote_change_processor_on_worker)
      override;
  virtual RemoteServiceState GetCurrentState() const override;
  virtual void GetOriginStatusMap(
      const RemoteFileSyncService::StatusMapCallback& callback) override;
  virtual scoped_ptr<base::ListValue> DumpFiles(const GURL& origin) override;
  virtual scoped_ptr<base::ListValue> DumpDatabase() override;
  virtual void SetSyncEnabled(bool enabled) override;
  virtual void PromoteDemotedChanges(const base::Closure& callback) override;
  virtual void ApplyLocalChange(const FileChange& local_change,
                                const base::FilePath& local_path,
                                const SyncFileMetadata& local_metadata,
                                const storage::FileSystemURL& url,
                                const SyncStatusCallback& callback) override;
  virtual void ActivateService(RemoteServiceState service_state,
                               const std::string& description) override;
  virtual void DeactivateService(const std::string& description) override;
  virtual void DetachFromSequence() override;
  virtual void AddObserver(Observer* observer) override;

 private:
  friend class DriveBackendSyncTest;
  friend class SyncWorkerTest;

  enum AppStatus {
    APP_STATUS_ENABLED,
    APP_STATUS_DISABLED,
    APP_STATUS_UNINSTALLED,
  };

  typedef base::hash_map<std::string, AppStatus> AppStatusMap;

  void DoDisableApp(const std::string& app_id,
                    const SyncStatusCallback& callback);
  void DoEnableApp(const std::string& app_id,
                   const SyncStatusCallback& callback);

  void PostInitializeTask();
  void DidInitialize(SyncEngineInitializer* initializer,
                     SyncStatusCode status);
  void UpdateRegisteredApps();
  static void QueryAppStatusOnUIThread(
      const base::WeakPtr<ExtensionServiceInterface>& extension_service_ptr,
      const std::vector<std::string>* app_ids,
      AppStatusMap* status,
      const base::Closure& callback);
  void DidQueryAppStatus(const AppStatusMap* app_status);
  void DidProcessRemoteChange(RemoteToLocalSyncer* syncer,
                              const SyncFileCallback& callback,
                              SyncStatusCode status);
  void DidApplyLocalChange(LocalToRemoteSyncer* syncer,
                           const SyncStatusCallback& callback,
                           SyncStatusCode status);

  // Returns true if a FetchChanges task is scheduled.
  bool MaybeStartFetchChanges();
  void DidResolveConflict(SyncStatusCode status);
  void DidFetchChanges(SyncStatusCode status);

  void UpdateServiceStateFromSyncStatusCode(SyncStatusCode state,
                                            bool used_network);
  void UpdateServiceState(RemoteServiceState state,
                          const std::string& description);

  void CallOnIdleForTesting(const base::Closure& callback);

  drive::DriveServiceInterface* GetDriveService();
  drive::DriveUploaderInterface* GetDriveUploader();
  MetadataDatabase* GetMetadataDatabase();

  base::FilePath base_dir_;

  leveldb::Env* env_override_;

  // Sync with SyncEngine.
  RemoteServiceState service_state_;

  bool should_check_conflict_;
  bool should_check_remote_change_;
  bool listing_remote_changes_;
  base::TimeTicks time_to_check_changes_;

  bool sync_enabled_;
  base::Closure call_on_idle_callback_;

  scoped_ptr<SyncTaskManager> task_manager_;

  base::WeakPtr<ExtensionServiceInterface> extension_service_;

  scoped_ptr<SyncEngineContext> context_;
  ObserverList<Observer> observers_;

  base::SequenceChecker sequence_checker_;

  base::WeakPtrFactory<SyncWorker> weak_ptr_factory_;
  DISALLOW_COPY_AND_ASSIGN(SyncWorker);
};

}  // namespace drive_backend
}  // namespace sync_file_system

#endif  // CHROME_BROWSER_SYNC_FILE_SYSTEM_DRIVE_BACKEND_SYNC_WORKER_H_
