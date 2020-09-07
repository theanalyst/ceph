// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*-
// vim: ts=8 sw=2 smarttab ft=cpp

/*
 * Ceph - scalable distributed file system
 *
 * Copyright (C) 2020 SUSE LLC
 *
 * This is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1, as published by the Free Software
 * Foundation. See file COPYING.
 *
 */

#pragma once

#include "rgw/rgw_service.h"
#include "svc_meta_be.h"

class RGWRole;

template <typename T>
struct RGWSI_Read_Ret {
  T info;
  map<std::string, bufferlist> attrs;
  real_time mtime;

  RGWSI_Read_Ret(T&& _info, map<std::string, bufferlist>&& _attrs, real_time&& _mtime) :
    info(std::move(_info)), attrs(std::move(_attrs)), mtime(std::move(_mtime)) {}
};

using RGWSI_Read_bl = RGWSI_Read_Ret<ceph::bufferlist>;
using RGWSI_Read_Role = RGWSI_Read_Ret<RGWRole>;
using RGWSI_Read_String = RGWSI_Read_Ret<std::string>;

using RoleReadRet = std::tuple<int,std::optional<RGWSI_Read_Role>>;
using StringReadRet = std::tuple<int,std::optional<RGWSI_Read_String>>;

class RGWSI_Role: public RGWServiceInstance
{
 public:
  RGWSI_Role(CephContext *cct) : RGWServiceInstance(cct) {}
  virtual ~RGWSI_Role() {}

  virtual RGWSI_MetaBackend_Handler* get_be_handler() = 0;
  static std::string get_role_meta_key(const std::string& role_id);
  static std::string get_role_name_meta_key(const std::string& role_name, const std::string& tenant);
  static std::string get_role_path_meta_key(const std::string& path, const std::string& role_id, const std::string& tenant);

  virtual int store_info(RGWSI_MetaBackend::Context *ctx,
			 const RGWRole& role,
			 RGWObjVersionTracker * const objv_tracker,
			 const real_time& mtime,
			 bool exclusive,
			 map<std::string, bufferlist> * pattrs,
			 optional_yield y) = 0;

  virtual int store_name(RGWSI_MetaBackend::Context *ctx,
			 const std::string& role_id,
			 const std::string& name,
			 const std::string& tenant,
			 RGWObjVersionTracker * const objv_tracker,
			 const real_time& mtime,
			 bool exclusive,
			 optional_yield y) = 0;

  virtual int store_path(RGWSI_MetaBackend::Context *ctx,
			 const std::string& role_id,
			 const std::string& path,
			 const std::string& tenant,
			 RGWObjVersionTracker * const objv_tracker,
			 const real_time &mtime,
			 bool exclusive,
			 optional_yield y) = 0;

  virtual RoleReadRet read_info(RGWSI_MetaBackend::Context *ctx,
				  const std::string& role_id,
				  RGWRole *role,
				  RGWObjVersionTracker * const objv_tracker,
				  real_time * const pmtime,
				  map<std::string, bufferlist> * pattrs,
				  optional_yield y) = 0;

  virtual StringReadRet read_name(RGWSI_MetaBackend::Context *ctx,
				    const std::string& name,
				    const std::string& tenant,
				    std::string& role_id,
				    RGWObjVersionTracker * const objv_tracker,
				    real_time * const pmtime,
				    optional_yield y) = 0;

  virtual StringReadRet read_path(RGWSI_MetaBackend::Context *ctx,
				    std::string& path,
				    RGWObjVersionTracker * const objv_tracker,
				    real_time * const pmtime,
				    optional_yield y) = 0;

  virtual int delete_info(RGWSI_MetaBackend::Context *ctx,
			  const std::string& name,
			  RGWObjVersionTracker * const objv_tracker,
			  optional_yield y) = 0;

  virtual int delete_name(RGWSI_MetaBackend::Context *ctx,
			  const std::string& name,
			  const std::string& tenant,
			  RGWObjVersionTracker * const objv_tracker,
			  optional_yield y) = 0;

  virtual int delete_path(RGWSI_MetaBackend::Context *ctx,
			  const std::string& role_id,
			  const std::string& path,
			  const std::string& tenant,
			  RGWObjVersionTracker * const objv_tracker,
			  optional_yield y) = 0;

};
