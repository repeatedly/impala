// Copyright (c) 2012 Cloudera, Inc. All rights reserved.

#ifndef IMPALA_EXEC_HDFS_BYTE_STREAM_H_
#define IMPALA_EXEC_HDFS_BYTE_STREAM_H_

#include "exec/byte-stream.h"
#include "exec/hdfs-scan-node.h"

#include <string>
#include <hdfs.h>

namespace impala {

class Status;

// A ByteStream implementation that is backed by an HDFS file
class HdfsByteStream : public ByteStream {
 public:
  // hdfs_connection: connection to the hadoop file system
  // scan_node: scan node which uses this byte stream, passed to record statistics.
  HdfsByteStream(hdfsFS hdfs_connection, HdfsScanNode* scan_node);

  virtual Status Open(const std::string& location);
  virtual Status Close();
  virtual Status Read(uint8_t *buf, int64_t req_length, int64_t* actual_length);
  virtual Status Seek(int64_t offset);
  virtual Status SeekRelative(int64_t offset);
  virtual Status GetPosition(int64_t* position);
  virtual Status Eof(bool* eof);

 private:
  hdfsFS hdfs_connection_;
  hdfsFile hdfs_file_;
  HdfsScanNode* scan_node_;
};

}

#endif
