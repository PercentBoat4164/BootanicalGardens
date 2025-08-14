#pragma once

#include "efsw/efsw.hpp"
#include <filesystem>
#include <functional>

class UpdateListener : public efsw::FileWatchListener {
public:
  using Func = std::function<void()>;

private:
  using Funcs = std::tuple<Func, Func, Func, Func>;
  static std::unordered_map<std::filesystem::path, Funcs> directoryFuncMap;
  static std::unordered_map<std::filesystem::path, Funcs> fileFuncMap;
  static efsw::FileWatcher watcher;

  UpdateListener() = default;

  /**
   * Call functions based on fileActions that have occurred
   * @param watchid WatchId that has detected the action
   * @param dir The directory being watched
   * @param filename Name of the affected file
   * @param action The type of action that was performed on the file
   * @param oldFilename The filename before the action
   */
  void handleFileAction(efsw::WatchID watchid, const std::string& dir, const std::string& filename, efsw::Action action, std::string oldFilename) override;

public:
  static UpdateListener* getInstance();

  /**
   * Watch a directory recursively for changes
   * @param directory The directory to watch
   * @param onAdd Function to be called when a file is added to the directory
   * @param onDelete Function to be called when a file is deleted from the directory
   * @param onModify Function to be called when a file is modified in the directory
   * @param onMove Function to be called when a file is moved in the directory
   */
  static void addDirectoryWatch(const std::filesystem::path& directory, const Func& onAdd, const Func& onDelete, const Func& onModify, const Func& onMove);

  /**
   * @param file The file to watch
  * @param onAdd Function to be called when a file is added to the directory
  * @param onDelete Function to be called when a file is deleted from the directory
  * @param onModify Function to be called when a file is modified in the directory
  * @param onMove Function to be called when a file is moved in the directory
   */
  static void addFileWatch(const std::filesystem::path& file, const Func& onAdd, const Func& onDelete, const Func& onModify, const Func& onMove);

  /**
   * Remove a directory from the watch list
   * @param directory The directory to remove the watch for
   */
  static void removeDirectoryWatch(const std::filesystem::path& directory);

  /**
   * Remove a file from the watch list
   * @param file The file to remove the watch for
   */
  static void removeFileWatch(const std::filesystem::path& file) {
    watcher.removeWatch(file.string());
    fileFuncMap.erase(file);
  }

  /**
   * Start watching for file changes
   */
   static void startWatching() {
     watcher.watch();
   }
};