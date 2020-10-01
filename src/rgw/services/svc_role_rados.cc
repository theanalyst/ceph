#include "svc_role_rados.h"
#include "svc_meta_be_sobj.h"
#include "rgw_role.h"
#include "rgw_zone.h"

#define dout_subsys ceph_subsys_rgw

RGWSI_MetaBackend_Handler* RGWSI_Role_RADOS::get_be_handler()
{
  return be_handler;
}

void RGWSI_Role_RADOS::init(RGWSI_Zone *_zone_svc,
                            RGWSI_Meta *_meta_svc,
                            RGWSI_MetaBackend *_meta_be_svc,
                            RGWSI_SysObj *_sysobj_svc)
{
  svc.zone = _zone_svc;
  svc.meta = _meta_svc;
  svc.meta_be = _meta_be_svc;
  svc.sysobj = _sysobj_svc;
}

int RGWSI_Role_RADOS::store_info(RGWSI_MetaBackend::Context *ctx,
                                 const RGWRoleInfo& role,
                                 RGWObjVersionTracker * const objv_tracker,
                                 const real_time& mtime,
                                 bool exclusive,
                                 map<std::string, bufferlist> * pattrs,
                                 optional_yield y)
{
  bufferlist data_bl;
  encode(role, data_bl);
  RGWSI_MBSObj_PutParams params(data_bl, pattrs, mtime, exclusive);

  return svc.meta_be->put(ctx, get_role_meta_key(role.id),
                          params, objv_tracker, y);
}

int RGWSI_Role_RADOS::store_name(RGWSI_MetaBackend::Context *ctx,
                                 const std::string& role_id,
                                 const std::string& name,
                                 const std::string& tenant,
                                 RGWObjVersionTracker * const objv_tracker,
                                 const real_time& mtime,
                                 bool exclusive,
                                 optional_yield y)
{
  RGWNameToId nameToId;
  nameToId.obj_id = role_id;

  bufferlist data_bl;
  encode(nameToId, data_bl);
  RGWSI_MBSObj_PutParams params(data_bl, nullptr, mtime, exclusive);

  return svc.meta_be->put(ctx, get_role_name_meta_key(name, tenant),
                          params, objv_tracker, y);
}


int RGWSI_Role_RADOS::store_path(RGWSI_MetaBackend::Context *ctx,
                                 const std::string& role_id,
                                 const std::string& path,
                                 const std::string& tenant,
                                 RGWObjVersionTracker * const objv_tracker,
                                 const real_time& mtime,
                                 bool exclusive,
                                 optional_yield y)
{
  bufferlist data_bl;
  RGWSI_MBSObj_PutParams params(data_bl, nullptr, mtime, exclusive);
  return svc.meta_be->put(ctx, get_role_path_meta_key(path, role_id, tenant),
                          params, objv_tracker, y);

}


int RGWSI_Role_RADOS::read_info(RGWSI_MetaBackend::Context *ctx,
                                const std::string& role_id,
                                RGWRoleInfo *role,
                                RGWObjVersionTracker * const objv_tracker,
                                real_time * const pmtime,
                                map<std::string, bufferlist> * pattrs,
                                optional_yield y)
{
  bufferlist data_bl;
  RGWSI_MBSObj_GetParams params(&data_bl, pattrs, pmtime);

  int r = svc.meta_be->get_entry(ctx, get_role_meta_key(role_id), params, objv_tracker, y);
  if (r < 0)
    return r;

  auto bl_iter = data_bl.cbegin();
  try  {
    decode(*role, bl_iter);
  } catch (buffer::error& err) {
    ldout(svc.meta_be->ctx(),0) << "ERROR: failed to decode RGWRoleInfo, caught buffer::err " << dendl;
    return -EIO;
  }

  return 0;
}

int RGWSI_Role_RADOS::read_name(RGWSI_MetaBackend::Context *ctx,
                                const std::string& name,
                                const std::string& tenant,
                                std::string& role_id,
                                RGWObjVersionTracker * const objv_tracker,
                                real_time * const pmtime,
                                optional_yield y)
{
  bufferlist data_bl;
  RGWSI_MBSObj_GetParams params(&data_bl, nullptr, pmtime);

  int r = svc.meta_be->get_entry(ctx, get_role_name_meta_key(name, tenant),
                                 params, objv_tracker, y);
  if (r < 0)
    return r;

  auto bl_iter = data_bl.cbegin();
  RGWNameToId nameToId;
  try  {
    decode(nameToId, bl_iter);
  } catch (buffer::error& err) {
    ldout(svc.meta_be->ctx(),0) << "ERROR: failed to decode RGWRoleInfo name, caught buffer::err " << dendl;
    return -EIO;
  }

  role_id = nameToId.obj_id;
  return 0;
}

static int delete_oid(RGWSI_MetaBackend::Context *ctx,
                      RGWSI_MetaBackend* meta_be,
                      const std::string& oid,
                      RGWObjVersionTracker * const objv_tracker,
                      optional_yield y)
{
  RGWSI_MBSObj_RemoveParams params;
  int r = meta_be->remove(ctx, oid, params, objv_tracker, y);
  if (r < 0 && r != -ENOENT && r != -ECANCELED) {
    ldout(meta_be->ctx(),0) << "ERROR: RGWSI_Role: could not remove oid = "
                                << oid << " r = "<< r << dendl;
    return r;
  }
  return 0;
}

int RGWSI_Role_RADOS::delete_info(RGWSI_MetaBackend::Context *ctx,
                                  const std::string& role_id,
                                  RGWObjVersionTracker * const objv_tracker,
                                  optional_yield y)
{

  return delete_oid(ctx, svc.meta_be, get_role_meta_key(role_id),
                    objv_tracker, y);
}

int RGWSI_Role_RADOS::delete_name(RGWSI_MetaBackend::Context *ctx,
                                  const std::string& name,
                                  const std::string& tenant,
                                  RGWObjVersionTracker * const objv_tracker,
                                  optional_yield y)
{
  return delete_oid(ctx, svc.meta_be, get_role_name_meta_key(name, tenant),
                    objv_tracker, y);

}

int RGWSI_Role_RADOS::delete_path(RGWSI_MetaBackend::Context *ctx,
                                  const std::string& role_id,
                                  const std::string& path,
                                  const std::string& tenant,
                                  RGWObjVersionTracker * const objv_tracker,
                                  optional_yield y)
{
  return delete_oid(ctx, svc.meta_be, get_role_path_meta_key(path, role_id, tenant),
                    objv_tracker, y);

}