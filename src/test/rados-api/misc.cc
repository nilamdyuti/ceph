#include "include/buffer.h"
#include "include/rados/librados.h"
#include "include/rados/librados.hpp"
#include "mds/mdstypes.h"
#include "test/rados-api/test.h"

#include "gtest/gtest.h"
#include <errno.h>
#include <map>
#include <sstream>
#include <string>

using namespace librados;
using ceph::buffer;
using std::map;
using std::ostringstream;
using std::string;

TEST(LibRadosMisc, Version) {
  int major, minor, extra;
  rados_version(&major, &minor, &extra);
}

TEST(LibRadosMisc, VersionPP) {
  int major, minor, extra;
  Rados::version(&major, &minor, &extra);
}

static std::string read_key_from_tmap(IoCtx& ioctx, const std::string &obj,
				      const std::string &key)
{
  bufferlist bl;
  int r = ioctx.read(obj, bl, 0, 0);
  if (r <= 0) {
    ostringstream oss;
    oss << "ioctx.read(" << obj << ", bl, 0, 0) returned " << r;
    return oss.str();
  }
  bufferlist::iterator p = bl.begin();
  bufferlist header;
  map<string, bufferlist> m;
  ::decode(header, p);
  ::decode(m, p);
  map<string, bufferlist>::iterator i = m.find(key);
  if (i == m.end())
    return "";
  std::string retstring;
  ::decode(retstring, i->second);
  return retstring;
}

static std::string add_key_to_tmap(IoCtx &ioctx, const std::string &obj,
	  const std::string &key, const std::string &val)
{
  __u8 c = CEPH_OSD_TMAP_SET;

  bufferlist tmbl;
  ::encode(c, tmbl);
  ::encode(key, tmbl);
  bufferlist blbl;
  ::encode(val, blbl);
  ::encode(blbl, tmbl);
  int ret = ioctx.tmap_update(obj, tmbl);
  if (ret) {
    ostringstream oss;
    oss << "ioctx.tmap_update(obj=" << obj << ", key="
	<< key << ", val=" << val << ") failed with error " << ret;
    return oss.str();
  }
  return "";
}

TEST(LibRadosMisc, TmapUpdatePP) {
  Rados cluster;
  std::string pool_name = get_temp_pool_name();
  ASSERT_EQ("", create_one_pool_pp(pool_name, cluster));
  IoCtx ioctx;
  cluster.ioctx_create(pool_name.c_str(), ioctx);

  // create tmap
  {
    __u8 c = CEPH_OSD_TMAP_CREATE;
    std::string my_tmap("my_tmap");
    bufferlist emptybl;

    bufferlist tmbl;
    ::encode(c, tmbl);
    ::encode(my_tmap, tmbl);
    ::encode(emptybl, tmbl);
    ASSERT_EQ(0, ioctx.tmap_update("foo", tmbl));
  }

  ASSERT_EQ(string(""), add_key_to_tmap(ioctx, "foo", "key1", "val1"));

  ASSERT_EQ(string(""), add_key_to_tmap(ioctx, "foo", "key2", "val2"));

  // read key1 from the tmap
  ASSERT_EQ(string("val1"), read_key_from_tmap(ioctx, "foo", "key1"));

  // remove key1 from tmap
  {
    __u8 c = CEPH_OSD_TMAP_RM;
    std::string key1("key1");
    bufferlist tmbl;
    ::encode(c, tmbl);
    ::encode(key1, tmbl);
    ASSERT_EQ(0, ioctx.tmap_update("foo", tmbl));
  }

  // key should be removed
  ASSERT_EQ(string(""), read_key_from_tmap(ioctx, "foo", "key1"));

  ioctx.close();
  ASSERT_EQ(0, destroy_one_pool_pp(pool_name, cluster));
}

//TEST(LibRadosMisc, Exec) {
//  char buf[128];
//  char buf2[sizeof(buf)];
//  char buf3[sizeof(buf)];
//  rados_t cluster;
//  rados_ioctx_t ioctx;
//  std::string pool_name = get_temp_pool_name();
//  ASSERT_EQ("", create_one_pool(pool_name, &cluster));
//  rados_ioctx_create(cluster, pool_name.c_str(), &ioctx);
//  memset(buf, 0xcc, sizeof(buf));
//  ASSERT_EQ((int)sizeof(buf), rados_write(ioctx, "foo", buf, sizeof(buf), 0));
//  strncpy(buf2, "abracadabra", sizeof(buf2));
//  memset(buf3, 0, sizeof(buf3));
//  ASSERT_EQ(0, rados_exec(ioctx, "foo", "crypto", "md5",
//	  buf2, strlen(buf2) + 1, buf3, sizeof(buf3)));
//  ASSERT_EQ(string("06fce6115b1efc638e0cc2026f69ec43"), string(buf3));
//  rados_ioctx_destroy(ioctx);
//  ASSERT_EQ(0, destroy_one_pool(pool_name, &cluster));
//}

//int rados_tmap_update(rados_ioctx_t io, const char *o, const char *cmdbuf, size_t cmdbuflen);

//TEST(LibRadosMisc, CloneRange) {
//  char buf[128];
//  rados_t cluster;
//  rados_ioctx_t ioctx;
//  std::string pool_name = get_temp_pool_name();
//  ASSERT_EQ("", create_one_pool(pool_name, &cluster));
//  rados_ioctx_create(cluster, pool_name.c_str(), &ioctx);
//  memset(buf, 0xcc, sizeof(buf));
//  ASSERT_EQ((int)sizeof(buf), rados_write(ioctx, "src", buf, sizeof(buf), 0));
//  rados_ioctx_locator_set_key(ioctx, "src");
//  ASSERT_EQ(0, rados_clone_range(ioctx, "dst", 0, "src", 0, sizeof(buf)));
//  rados_ioctx_locator_set_key(ioctx, NULL);
//  char buf2[sizeof(buf)];
//  memset(buf2, 0, sizeof(buf2));
//  ASSERT_EQ((int)sizeof(buf2), rados_read(ioctx, "dst", buf2, sizeof(buf2), 0));
//  ASSERT_EQ(0, memcmp(buf, buf2, sizeof(buf)));
//  rados_ioctx_destroy(ioctx);
//  ASSERT_EQ(0, destroy_one_pool(pool_name, &cluster));
//}
