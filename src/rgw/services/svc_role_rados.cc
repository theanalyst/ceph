#include "svc_role_rados.h"
#include "svc_meta_be_sobj.h"
#include "rgw_role.h"

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
                                 const RGWRole& role,
                                 RGWObjVersionTracker * const objv_tracker,
                                 const real_time& mtime,
                                 bool exclusive,
                                 map<std::string, bufferlist> * pattrs,
                                 optional_yield y)
{
  bufferlist data_bl;
  encode(role, data_bl);
  RGWSI_MBSObj_PutParams params(data_bl, pattrs, mtime, exclusive);

  return svc.meta_be->put(ctx, get_role_meta_key(role.get_id()), params, objv_tracker, y);
}

int RGWSI_Role_RADOS::read_info(RGWSI_MetaBackend::Context *ctx,
                                const std::string& role_id,
                                RGWRole *role,
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
    ldout(svc.meta_be->ctx(),0) << "ERROR: failed to decode RGWRole, caught buffer::err " << dendl;
    return -EIO;
  }

  return 0;
}

int RGWSI_Role_RADOS::delete_info(RGWSI_MetaBackend::Context *ctx,
                                  const std::string& role_id,
                                  RGWObjVersionTracker * const objv_tracker,
                                  optional_yield y)
{
  RGWSI_MBSObj_RemoveParams params;

  int r = svc.meta_be->remove(ctx, get_role_meta_key(role_id), params, objv_tracker, y);
  if (r < 0 && r != -ENOENT && r != -ECANCELED) {
    ldout(svc.meta_be->ctx(),0) << "ERROR: could not remove RGWRole, id = "
                                << role_id << " r = "<< r << dendl;
    return r;
  }
  return 0;
}
