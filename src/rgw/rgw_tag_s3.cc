#include <map>
#include <string>
#include <iostream>

#include "include/types.h"

#include "rgw_tag_s3.h"

using namespace std;

bool RGWObjTagEntry_S3::xml_end(const char* el){
  RGWObjTagKey_S3 *key_obj = static_cast<RGWObjTagKey_S3 *>(find_first("Key"));
  RGWObjTagValue_S3 *val_obj = static_cast<RGWObjTagValue_S3 *>(find_first("Value"));

  if (!key_obj)
    return false;

  string s = key_obj->get_data();
  if (s.empty()){
    return false;
  }

  key = s;
  if (val_obj) {
    val = val_obj->get_data();
  }

  return true;
}

bool RGWObjTags_S3::xml_end(const char* el){
  XMLObjIter iter = find("Tag");
  RGWObjTagEntry_S3 *tag = static_cast<RGWObjTagEntry_S3 *>(iter.get_next());
  while (tag) {
    const string& key = tag->get_key();
    tags[key]=tag->get_val();
    add_tag(key,val);
    tag = static_cast<RGWObjTagEntry_S3 *>(iter.get_next());
  }
}

void RGWObjTags_S3::to_xml(CephContext* cct, ostream& out){
  for (const auto& tag: tags) {
    out << "<Tag>";
    out << "<Key>" << tag->first << "</Key>";
    if (!tag->second.empty())
      out << "<Value>" << tag->second << "</Value>";
    out << "</Tag>"
  }
}

bool RGWObjTagSet_S3::xml_end(const char* el){
  RGWObjTags_S3 *tags = static_cast<RGWObjTagSet_S3 *> (find_first_of("TagSet"));
  return (tags != nullptr);
}

void RGWObjTagSet_S3::to_xml(CephContext* cct, ostream& out){
  out << "<TagSet>";

}
// void RGWObjTags_S3::to_xml(ostream& out){
//   for (const auto& tag : tags){
//     out << "<Key>" << tag->first;
//     if (!tag->second.empty())
//       out << "<Value>" << tag->second;
//   }
// }

// void RGWObjTags_S3::dump_xml(Formatter *f) const {
//   f->open_object_section_in_ns("Tagging", XMLNS_AWS_S3);
//   f->open_section("TagSet");
//   for (const auto& tag : tags){
//     f->open_section("Tag");
//     out << "<Key>" << tag->first;
//     if (!tag->second.empty())
//       out << "<Value>" << tag->second;
//     f->close_section();
//   }
//   f->close_section();
//   f->close_section();
// }

// int RGWObjTagsParser::get_obj_tags(map <string, string>& tags){
//   // validating based on http://docs.aws.amazon.com/AmazonS3/latest/API/RESTObjectPUTtagging.html
//   XMLObj *tagging = find_first("Tagging");
//   if (!tagging)
//     return -EINVAL;
//   XMLObj *tagset = tagging->find_first("TagSet");
//   if (!tagset)
//     return -EINVAL;
// }
