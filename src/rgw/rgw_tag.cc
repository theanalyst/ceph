
#include <map>
#include <string>

#include <common/errno.h>

#include "rgw_tag.h"
#define MAX_OBJ_TAGS 10
#define MAX_TAG_KEY_SIZE 128
#define MAX_TAG_VAL_SIZE 256

int RGWObjTags::add_tag(const string&key, const string& val){
  if (tags.size() == MAX_OBJ_TAGS || key.size() > MAX_TAG_KEY_SIZE || val.size() > MAX_TAG_VAL_SIZE){
    // amz doesn't seem to support a modify op, so we don't have to check for
    // the key existence
    return -EINVAL;
  }
  tags[key] = val;
  return 0;
}
