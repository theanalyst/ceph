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

bool RGWObjTagging_S3::xml_end(const char* el){
  RGWObjTagSet_S3 *tagset = static_cast<RGWObjTagSet_S3 *> (find_first_of("TagSet"));
  if (tagset){
      XMLObjIter iter = find("Tag");
      RGWObjTagEntry_S3 *tagentry = static_cast<RGWObjTagEntry_S3 *>(iter.get_next());
      while (tagentry) {
	  const string& key = tagentry->get_key();
	  const string& val = tagentry->get_val();
	  add_tag(key,val);
	  tagentry = static_cast<RGWObjTagEntry_S3 *>(iter.get_next());
      }
  }

}

void RGWObjTagging_S3::dump_xml(Formatter &f){
    f.open_section_in_ns("Tagging");
    f.open_object_section("TagSet");
    for (const auto& tag: tags){
	f.open_section("Tags");
	f.open_section("Key");
	f.dump_string(tag->first);
	if (tag->second){
	    f.open_section("Value");
	    f.dump_string(tag->second);
	}
    }
}

XMLObj *RGWObjTagsXMLParser::alloc_obj(const char *el){
    XMLObj* obj = nullptr;
    if(strcmp(el,"Tagging") == 0) {
	obj = new RGWObjTagging_S3();
    } else if (strcmp(el,"TagSet") == 0) {
	obj = new RGWTagSet_S3();
    } else if (strcmp(el,"Tag") == 0) {
	obj = new RGWTag_S3();
    } else if (strcmp(el,"Key") == 0) {
	obj = new RGWTagKey_S3();
    } else if (strcmp(el,"Value") == 0) {
	obj = new RGWTagValue_S3();
    }

    return obj;
}

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
