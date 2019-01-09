/*
 * Project: curve
 * Created Date: Tuesday September 25th 2018
 * Author: hzsunjianliang
 * Copyright (c) 2018 netease
 */

#ifndef  SRC_MDS_NAMESERVER2_NAMESPACE_SERVICE_H_
#define  SRC_MDS_NAMESERVER2_NAMESPACE_SERVICE_H_

#include <brpc/closure_guard.h>
#include <brpc/controller.h>
#include "proto/nameserver2.pb.h"


namespace curve {
namespace mds {

class NameSpaceService: public CurveFSService {
 public:
    NameSpaceService() {}

    virtual ~NameSpaceService() {}

    void CreateFile(::google::protobuf::RpcController* controller,
                       const ::curve::mds::CreateFileRequest* request,
                       ::curve::mds::CreateFileResponse* response,
                       ::google::protobuf::Closure* done) override;

    void GetFileInfo(::google::protobuf::RpcController* controller,
                       const ::curve::mds::GetFileInfoRequest* request,
                       ::curve::mds::GetFileInfoResponse* response,
                       ::google::protobuf::Closure* done) override;

    void GetOrAllocateSegment(::google::protobuf::RpcController* controller,
                       const ::curve::mds::GetOrAllocateSegmentRequest* request,
                       ::curve::mds::GetOrAllocateSegmentResponse* response,
                       ::google::protobuf::Closure* done) override;

    void DeleteSegment(::google::protobuf::RpcController* controller,
                         const ::curve::mds::DeleteSegmentRequest* request,
                         ::curve::mds::DeleteSegmentResponse* response,
                         ::google::protobuf::Closure* done) override;

    void RenameFile(::google::protobuf::RpcController* controller,
                       const ::curve::mds::RenameFileRequest* request,
                       ::curve::mds::RenameFileResponse* response,
                       ::google::protobuf::Closure* done) override;

    void ExtendFile(::google::protobuf::RpcController* controller,
                       const ::curve::mds::ExtendFileRequest* request,
                       ::curve::mds::ExtendFileResponse* response,
                       ::google::protobuf::Closure* done) override;

    void CreateSnapShot(::google::protobuf::RpcController* controller,
                       const ::curve::mds::CreateSnapShotRequest* request,
                       ::curve::mds::CreateSnapShotResponse* response,
                       ::google::protobuf::Closure* done) override;
    void ListSnapShot(::google::protobuf::RpcController* controller,
                       const ::curve::mds::ListSnapShotFileInfoRequest* request,
                       ::curve::mds::ListSnapShotFileInfoResponse* response,
                       ::google::protobuf::Closure* done) override;
    void DeleteSnapShot(::google::protobuf::RpcController* controller,
                       const ::curve::mds::DeleteSnapShotRequest* request,
                       ::curve::mds::DeleteSnapShotResponse* response,
                       ::google::protobuf::Closure* done) override;
    void CheckSnapShotStatus(::google::protobuf::RpcController* controller,
                       const ::curve::mds::CheckSnapShotStatusRequest* request,
                       ::curve::mds::CheckSnapShotStatusResponse* response,
                       ::google::protobuf::Closure* done) override;
    void GetSnapShotFileSegment(::google::protobuf::RpcController* controller,
                       const ::curve::mds::GetOrAllocateSegmentRequest* request,
                       ::curve::mds::GetOrAllocateSegmentResponse* response,
                       ::google::protobuf::Closure* done);
};
}  // namespace mds
}  // namespace curve
#endif   // SRC_MDS_NAMESERVER2_NAMESPACE_SERVICE_H_