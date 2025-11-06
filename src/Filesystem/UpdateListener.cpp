#include "UpdateListener.hpp"

#include <cassert>

std::unordered_map<std::filesystem::path, UpdateListener::Funcs> UpdateListener::directoryFuncMap;
std::unordered_map<std::filesystem::path, UpdateListener::Funcs> UpdateListener::fileFuncMap;
efsw::FileWatcher UpdateListener::watcher;

void UpdateListener::handleFileAction(efsw::WatchID watchid, const std::string& dir, const std::string& filename, efsw::Action action, std::string oldFilename) {
  const Funcs funcs = directoryFuncMap.at(std::filesystem::canonical(dir));
  switch (action) {
    case efsw::Actions::Add: if (std::get<0>(funcs)) std::get<0>(funcs)(); break;
    case efsw::Actions::Delete: if (std::get<1>(funcs)) std::get<1>(funcs)(); break;
    case efsw::Actions::Modified: if (std::get<2>(funcs)) std::get<2>(funcs)(); break;
    case efsw::Actions::Moved: if (std::get<3>(funcs)) std::get<3>(funcs)(); break;
    default: assert(false);
  }
}

UpdateListener* UpdateListener::getInstance() {
  static UpdateListener listener = UpdateListener();
  return &listener;
}

void UpdateListener::addDirectoryWatch(const std::filesystem::path& directory, const Func& onAdd, const Func& onDelete, const Func& onModify, const Func& onMove) {
  watcher.addWatch(directory.string(), getInstance(), true);
  directoryFuncMap.insert({std::filesystem::canonical(directory), {onAdd, onDelete, onModify, onMove}});
}

void UpdateListener::addFileWatch(const std::filesystem::path& file, const Func& onAdd, const Func& onDelete, const Func& onModify, const Func& onMove) {
  watcher.addWatch(file.string(), getInstance(), true);
  fileFuncMap.insert({std::filesystem::canonical(file), {onAdd, onDelete, onModify, onMove}});
}

void UpdateListener::removeDirectoryWatch(const std::filesystem::path& directory) {
  watcher.removeWatch(directory.string());
  directoryFuncMap.erase(std::filesystem::canonical(directory));
}