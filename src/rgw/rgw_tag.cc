
#include <map>
#include <string>

#include <common/errno.h>
#include <boost/algorithm/string.hpp>

#include "rgw_tag.h"

static constexpr uint32_t MAX_OBJ_TAGS=10;
static constexpr uint32_t MAX_TAG_KEY_SIZE=128;
static constexpr uint32_t MAX_TAG_VAL_SIZE=256;

bool RGWObjTags::add_tag(const string&key, const string& val){
  return tags.emplace(std::make_pair(key,val)).second;
}

int RGWObjTags::check_and_add_tag(const string&key, const string& val){
  if (tags.size() == MAX_OBJ_TAGS ||
      key.size() > MAX_TAG_KEY_SIZE ||
      val.size() > MAX_TAG_VAL_SIZE ||
      key.size() == 0){
    return -ERR_INVALID_TAG;
  }

  // if we get a conflicting key, either the XML is malformed or the user
  // supplied an invalid string
  if (!add_tag(key,val))
    return -EINVAL;

  return 0;
}

int RGWObjTags::set_from_string(const string& input){
  int ret=0;
  vector <string> kvs;
  boost::split(kvs, input, boost::is_any_of("&"));
  for (const auto& kv: kvs){
    auto p = kv.find("=");
    string key,val;
    if (p != string::npos) {
      // TODO: actually handle if url_decode errors
      url_decode(kv.substr(0,p),key);
      url_decode(kv.substr(p+1),val);
      ret = check_and_add_tag(key,val);
    } else {
      url_decode(kv,key);
      ret = check_and_add_tag(key);
    }

    if (ret < 0)
      return ret;
  }
  return ret;
}
