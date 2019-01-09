/*
 * Project: curve
 * Created Date: Friday September 7th 2018
 * Author: hzsunjianliang
 * Copyright (c) 2018 netease
 */
#ifndef SRC_MDS_NAMESERVER2_NAMESPACE_STORAGE_H_
#define SRC_MDS_NAMESERVER2_NAMESPACE_STORAGE_H_

#include <string>
#include <tuple>
#include <vector>
#include <iostream>
#include <map>

#include "proto/nameserver2.pb.h"

#include "src/common/encode.h"
#include "src/mds/nameserver2/define.h"

namespace curve {
namespace mds {

enum class StoreStatus {
    OK = 0,
    KeyNotExist,
    InternalError,
};
std::ostream& operator << (std::ostream & os, StoreStatus &s);

const char FILEINFOKEYPREFIX[] = "01";
const char SEGMENTINFOKEYPREFIX[] = "02";
const char SNAPSHOTFILEINFOKEYPREFIX[] = "03";
// TODO(hzsunjianliang): if use single prefix for snapshot file?
const int PREFIX_LENGTH = 2;

std::string EncodeFileStoreKey(uint64_t parentID, const std::string &fileName);
std::string EncodeSnapShotFileStoreKey(uint64_t parentID,
                const std::string &fileName);
std::string EncodeSegmentStoreKey(uint64_t inodeID, offset_t offset);


// TODO(hzsunjianliang): may be storage need high level abstruction
// put the encoding internal, not external


// kv value storage for namespace and segment
class NameServerStorage {
 public:
    virtual ~NameServerStorage(void) {}

    virtual StoreStatus PutFile(const std::string & storeKey,
                const FileInfo & fileInfo) = 0;

    virtual StoreStatus GetFile(const std::string & storeKey,
                FileInfo * fileInfo) = 0;

    virtual StoreStatus DeleteFile(const std::string & storekey) = 0;

    // TODO(lixiaocui1): need transaction here
    virtual StoreStatus RenameFile(const std::string & oldStoreKey,
                                   const FileInfo &oldfileInfo,
                                   const std::string & newStoreKey,
                                   const FileInfo &newfileInfo) = 0;

    virtual StoreStatus ListFile(const std::string & startStoreKey,
                                 const std::string & endStoreKey,
                                 std::vector<FileInfo> * files) = 0;


    virtual StoreStatus GetSegment(const std::string & storeKey,
                                   PageFileSegment *segment) = 0;

    virtual StoreStatus PutSegment(const std::string & storeKey,
                                   const PageFileSegment * segment) = 0;

    virtual StoreStatus DeleteSegment(const std::string &storeKey) = 0;

    // TODO(lixiaocui1): need transaction here
    virtual StoreStatus SnapShotFile(const std::string & originalFileKey,
                                    const FileInfo *originalFileInfo,
                                    const std::string & snapshotFileKey,
                                    const FileInfo * snapshotFileInfo) = 0;
    virtual StoreStatus LoadSnapShotFile(
                                std::vector<FileInfo> *snapShotFiles) = 0;
};
}  // namespace mds
}  // namespace curve

#endif   // SRC_MDS_NAMESERVER2_NAMESPACE_STORAGE_H_