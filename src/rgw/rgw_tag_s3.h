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

class RGWObjTagKey_S3: public XMLObj
{
public:
  RGWObjTagKey_S3() {};
  ~RGWObjTagKey_S3() {};
};

class RGWObjTagValue_S3: public XMLObj
{
public:
  RGWObjTagValue_S3() {};
  ~RGWObjTagValue_S3() {};
};

class RGWObjTagEntry_S3: public XMLObj
{
  string key;
  string val;
public:
  RGWObjTagEntry_S3() {}
  RGWObjTagEntry_S3(string k,string v):key(k),val(v) {};
  ~RGWObjTagEntry_S3() {}

  bool xml_end(const char* el);
  const string& get_key () { return key;}
  const string& get_val () { return val;}
  //void to_xml(CephContext *cct, ostream& out) const;
};

class RGWObjTags_S3: public RGWObjTags, public XMLObj
{
public:
  RGWObjTags_S3() {}
  ~RGWObjTags_S3() {}

  bool xml_end(const char* el);
  void to_xml(CephContext *cct, ostream& out) const;
};

class RGWObjTagSet_S3: public XMLObj
{
public:
  RGWObjTagSet_S3() {}
  ~RGWObjTagSet_S3() {}

  bool xml_end(const char* el);
  void to_xml(CephContext *cct, ostream& out) const;
};

// class RGWObjTags_S3 : public RGWObjTags, public XMLObj
// {
// public:
//   RGWObjTags_S3() {}
//   ~RGWObjTags_S3() {}

//   void to_xml(ostream& out);
//   void dump_xml(Formatter *f) const;

// };



// class RGWObjTagsParser : public RGWXMLParser
// {
//   XMLObj *alloc_obj(const char *el){
//     return new XMLObj;
//   }
// public:
//   RGWObjTagsParser() {}
//   ~RGWObjTagsParser() {}

//   int get_obj_tags(map <string,string>& tags);
// };

#endif /* RGW_TAG_S3_H */
