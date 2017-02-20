#include <map>
#include <string>
#include <iostream>

#include "include/types.h"

#include "rgw_tag_s3.h"

using namespace std;

void RGWObjTags_S3::to_xml(ostream& out){
  for (const auto& tag : tags){
    out << "<Key>" << tag->first;
    if (!tag->second.empty())
      out << "<Value>" << tag->second;
  }
}

void RGWObjTags_S3::dump_xml(Formatter *f) const {
  f->open_object_section_in_ns("Tagging", XMLNS_AWS_S3);
  f->open_section("TagSet");
  for (const auto& tag : tags){
    f->open_section("Tag");
    out << "<Key>" << tag->first;
    if (!tag->second.empty())
      out << "<Value>" << tag->second;
    f->close_section();
  }
  f->close_section();
  f->close_section();
}

int RGWObjTagsParser::get_obj_tags(map <string, string>& tags){
  // validating based on http://docs.aws.amazon.com/AmazonS3/latest/API/RESTObjectPUTtagging.html
  XMLObj *tagging = find_first("Tagging");
  if (!tagging)
    return -EINVAL;
  XMLObj *tagset = tagging->find_first("TagSet");
  if (!tagset)
    return -EINVAL;


}
