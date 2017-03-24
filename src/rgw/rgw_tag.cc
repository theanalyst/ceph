
#include <map>
#include <string>

#include <common/errno.h>

#include "rgw_tag.h"

static constexpr uint32_t MAX_OBJ_TAGS=10;
static constexpr uint32_t MAX_TAG_KEY_SIZE=128;
static constexpr uint32_t MAX_TAG_VAL_SIZE=256;

void RGWObjTags::add_tag(const string&key, const string& val){
  tags[key] = val;
}

int RGWObjTags::check_and_add_tag(const string&key, const string& val){
  if (tags.size() == MAX_OBJ_TAGS || key.size() > MAX_TAG_KEY_SIZE || val.size() > MAX_TAG_VAL_SIZE){
    // amz doesn't seem to support a modify op, so we don't have to check for
    // the key existence
    return -ERR_INVALID_TAG;
  }
  tags[key] = val;
  return 0;
}

int RGWObjTags::set_from_string(const string& input){
  string url_decoded_input;
  bool decoded=url_decode(input, url_decoded_input);
  if (!decoded)
    return -EINVAL;

  int ret=0;
  for (const auto& kv: split(url_decoded_input, '&')){
    auto p = kv.find_first_of("=");
    if (p != string::npos) {
      // p+1 will be size() in worst case, so this will not raise an exception
      ret = check_and_add_tag(kv.substr(0,p), kv.substr(p+1));
    } else {
      ret = check_and_add_tag(kv);
    }
    if (ret < 0)
      return ret;
  }
  return ret;
}
