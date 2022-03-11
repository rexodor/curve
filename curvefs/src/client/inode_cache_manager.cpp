/*
 *  Copyright (c) 2021 NetEase Inc.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */


/*
 * Project: curve
 * Created Date: Thur May 27 2021
 * Author: xuchaojie
 */

#include "curvefs/src/client/inode_cache_manager.h"

#include <glog/logging.h>

#include <map>
#include <utility>

using ::curvefs::metaserver::Inode;
using ::curvefs::metaserver::MetaStatusCode_Name;

namespace curvefs {
namespace client {
namespace common {
DECLARE_bool(enableCto);
}  // namespace common
}  // namespace client
}  // namespace curvefs

namespace curvefs {
namespace client {

using NameLockGuard = ::curve::common::GenericNameLockGuard<Mutex>;

CURVEFS_ERROR InodeCacheManagerImpl::GetInode(uint64_t inodeid,
    std::shared_ptr<InodeWrapper> &out) {
    NameLockGuard lock(nameLock_, std::to_string(inodeid));
    bool ok = iCache_->Get(inodeid, &out);
    if (ok) {
        // if enableCto, we need and is unopen, we need reload from
        // metaserver
        if (curvefs::client::common::FLAGS_enableCto && !out->IsOpen()) {
            VLOG(6) << "InodeCacheManagerImpl, GetInode: enableCto and inode: "
                    << inodeid << " opencount is 0";
            iCache_->Remove(inodeid);
        } else {
            return CURVEFS_ERROR::OK;
        }
    }

    Inode inode;
    MetaStatusCode ret2 = metaClient_->GetInode(fsId_, inodeid, &inode);
    if (ret2 != MetaStatusCode::OK) {
        LOG_IF(ERROR, ret2 != MetaStatusCode::NOT_FOUND)
            << "metaClient_ GetInode failed, MetaStatusCode = " << ret2
            << ", MetaStatusCode_Name = " << MetaStatusCode_Name(ret2)
            << ", inodeid = " << inodeid;
        return MetaStatusCodeToCurvefsErrCode(ret2);
    }

    out = std::make_shared<InodeWrapper>(
        std::move(inode), metaClient_);

    std::shared_ptr<InodeWrapper> eliminatedOne;
    bool eliminated = iCache_->Put(inodeid, out, &eliminatedOne);
    if (eliminated) {
        eliminatedOne->FlushAsync();
    }
    return CURVEFS_ERROR::OK;
}

CURVEFS_ERROR InodeCacheManagerImpl::BatchGetInodeAttr(
    const std::set<uint64_t> &inodeIds,
    std::list<InodeAttr> *attr) {
    MetaStatusCode ret = metaClient_->BatchGetInodeAttr(fsId_, inodeIds, attr);
    if (MetaStatusCode::OK != ret) {
        LOG(ERROR) << "metaClient BatchGetInodeAttr failed, MetaStatusCode = "
                   << ret << ", MetaStatusCode_Name = "
                   << MetaStatusCode_Name(ret);
    }
    return MetaStatusCodeToCurvefsErrCode(ret);
}

CURVEFS_ERROR InodeCacheManagerImpl::BatchGetXAttr(
    const std::set<uint64_t> &inodeIds,
    std::list<XAttr> *xattr) {
    MetaStatusCode ret = metaClient_->BatchGetXAttr(fsId_, inodeIds, xattr);
    if (MetaStatusCode::OK != ret) {
        LOG(ERROR) << "metaClient BatchGetXAttr failed, MetaStatusCode = "
                   << ret << ", MetaStatusCode_Name = "
                   << MetaStatusCode_Name(ret);
    }
    return MetaStatusCodeToCurvefsErrCode(ret);
}

CURVEFS_ERROR InodeCacheManagerImpl::CreateInode(
    const InodeParam &param,
    std::shared_ptr<InodeWrapper> &out) {
    Inode inode;
    MetaStatusCode ret = metaClient_->CreateInode(param, &inode);
    if (ret != MetaStatusCode::OK) {
        LOG(ERROR) << "metaClient_ CreateInode failed, MetaStatusCode = " << ret
                   << ", MetaStatusCode_Name = " << MetaStatusCode_Name(ret);
        return MetaStatusCodeToCurvefsErrCode(ret);
    }
    uint64_t inodeid = inode.inodeid();
    out = std::make_shared<InodeWrapper>(
        std::move(inode), metaClient_);

    std::shared_ptr<InodeWrapper> eliminatedOne;
    bool eliminated = false;
    {
        NameLockGuard lock(nameLock_, std::to_string(inodeid));
        eliminated = iCache_->Put(inodeid, out, &eliminatedOne);
    }
    if (eliminated) {
        eliminatedOne->FlushAsync();
    }
    return CURVEFS_ERROR::OK;
}

CURVEFS_ERROR InodeCacheManagerImpl::DeleteInode(uint64_t inodeid) {
    NameLockGuard lock(nameLock_, std::to_string(inodeid));
    iCache_->Remove(inodeid);
    MetaStatusCode ret = metaClient_->DeleteInode(fsId_, inodeid);
    if (ret != MetaStatusCode::OK && ret != MetaStatusCode::NOT_FOUND) {
        LOG(ERROR) << "metaClient_ DeleteInode failed, MetaStatusCode = " << ret
                   << ", MetaStatusCode_Name = " << MetaStatusCode_Name(ret)
                   << ", inodeid = " << inodeid;
        return MetaStatusCodeToCurvefsErrCode(ret);
    }

    curve::common::LockGuard lg2(dirtyMapMutex_);
    dirtyMap_.erase(inodeid);
    return CURVEFS_ERROR::OK;
}

void InodeCacheManagerImpl::ClearInodeCache(uint64_t inodeid) {
    {
        NameLockGuard lock(nameLock_, std::to_string(inodeid));
        iCache_->Remove(inodeid);
    }
    curve::common::LockGuard lg2(dirtyMapMutex_);
    dirtyMap_.erase(inodeid);
}

void InodeCacheManagerImpl::ShipToFlush(
    const std::shared_ptr<InodeWrapper> &inodeWrapper) {
    curve::common::LockGuard lg(dirtyMapMutex_);
    dirtyMap_.emplace(inodeWrapper->GetInodeId(), inodeWrapper);
}

void InodeCacheManagerImpl::FlushAll() {
    while (!dirtyMap_.empty()) {
        FlushInodeOnce();
    }
}

void InodeCacheManagerImpl::FlushInodeOnce() {
    std::map<uint64_t, std::shared_ptr<InodeWrapper>> temp_;
    {
        curve::common::LockGuard lg(dirtyMapMutex_);
        temp_.swap(dirtyMap_);
    }
    for (auto it = temp_.begin(); it != temp_.end(); it++) {
        curve::common::UniqueLock ulk = it->second->GetUniqueLock();
        it->second->FlushAsync();
    }
}

void InodeCacheManagerImpl::AddParent(uint64_t inodeId, uint64_t parentId) {
    curve::common::LockGuard lg2(parentIdMapMutex_);
    if (parentIdMap_.count(inodeId)) {
        parentIdMap_[inodeId].emplace_back(parentId);
    } else {
        parentIdMap_.emplace(inodeId, std::list<uint64_t>({parentId}));
    }
}

void InodeCacheManagerImpl::RemoveParent(uint64_t inodeId, uint64_t parentId) {
    curve::common::LockGuard lg2(parentIdMapMutex_);
    if (parentIdMap_.count(inodeId)) {
        auto iter = std::find(parentIdMap_[inodeId].begin(),
            parentIdMap_[inodeId].end(), parentId);
        if (iter != parentIdMap_[inodeId].end()) {
            parentIdMap_[inodeId].erase(iter);
        }
    }
}

void InodeCacheManagerImpl::ClearParent(uint64_t inodeId) {
    curve::common::LockGuard lg2(parentIdMapMutex_);
    parentIdMap_.erase(inodeId);
}

bool InodeCacheManagerImpl::UpdateParent(uint64_t inodeId, uint64_t oldParentId,
    uint64_t newParentId) {
    curve::common::LockGuard lg2(parentIdMapMutex_);
    if (parentIdMap_.count(inodeId)) {
        auto iter = std::find(parentIdMap_[inodeId].begin(),
            parentIdMap_[inodeId].end(), oldParentId);
        if (iter != parentIdMap_[inodeId].end()) {
            *iter = newParentId;
            return true;
        }
    }
    return false;
}

bool InodeCacheManagerImpl::GetParent(uint64_t inodeId,
    std::list<uint64_t> *parentIds) {
    curve::common::LockGuard lg2(parentIdMapMutex_);
    if (parentIdMap_.count(inodeId)) {
        *parentIds = parentIdMap_[inodeId];
        return true;
    }
    return false;
}


}  // namespace client
}  // namespace curvefs