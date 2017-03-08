#ifndef RGW_TAG_H
#define RGW_TAG_H

#include <map>
#include <string>
#include <include/types.h>

#include "rgw_common.h"

using std::string;
using std::map;

class RGWObjTags
{
 protected:
  map <string, string> tags;
 public:
  RGWObjTags() {}
  ~RGWObjTags() {}

  void encode(bufferlist& bl) const {
    ENCODE_START(1,1,bl);
    ::encode(tags, bl);
    ENCODE_FINISH(bl);
  }

  void decode(bufferlist::iterator &bl) {
    DECODE_START_LEGACY_COMPAT_LEN(1, 1, 1, bl);
    ::decode(tags,bl);
    DECODE_FINISH(bl);
  }

  void dump(Formatter *f) const;
  int add_tag(const string& key, const string& val="");
  size_t count() const {return tags.size();}
  int set_from_string(const string& input); // implement me!
  const map <string,string>& get_tags() const {return tags;}
};
WRITE_CLASS_ENCODER(RGWObjTags)

#endif /* RGW_TAG_H */
