#ifndef RGW_TAG_S3_H
#define RGW_TAG_S3_H

#include <map>
#include <string>
#include <iostream>
#include <include/types.h>

#include <expat.h>

#include "rgw_tag.h"
#include "rgw_xml.h"

using namespace std;

class RGWObjTags_S3 : public RGWObjTags, public XMLObj
{
public:
  RGWObjTags_S3() {}
  ~RGWObjTags_S3() {}

  void to_xml(ostream& out);
  void dump_xml(Formatter *f) const;

};

class RGWObjTagsParser : public RGWXMLParser
{
  XMLObj *alloc_obj(const char *el){
    return new XMLObj;
  }
public:
  RGWObjTagsParser() {}
  ~RGWObjTagsParser() {}

  int get_obj_tags(map <string,string>& tags);
};

#endif /* RGW_TAG_S3_H */
